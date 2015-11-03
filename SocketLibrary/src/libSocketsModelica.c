#include <libSocketsModelica.h>

#include <Debug.h>
#include <Sockets.h>
#include <Timer.h>
#include <GeneralBuffer.h>
#include <ControlBuffer.h>
#include <House.h>
#include <Fifo.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <math.h>

/************************************************************
* Local enums and defines
************************************************************/

typedef enum _comms_status {
	COMMS_MEAS_WAIT = 0,
	COMMS_MEAS_SEND,
	COMMS_CMDS_WAIT,
	COMMS_CMDS_RECV
} CommsStatus;

/************************************************************
* Local functions declaration
************************************************************/

static void send_meas(const char * const name, const double value, const int32_t ctrl);
static double get_cmds(const char * const name, const int32_t ctrl);

static void advance(const int32_t ctrl, const int step);
static int recv_MEAS_ctrl(CommsStatus *status, Timer t, const int step);
static int send_MEAS_buffer(CommsStatus *status, const int step);
static int recv_CMDS_ctrl(CommsStatus *status, Timer t, const int step);
static int recv_CMDS_buffer(CommsStatus *status, Timer t, const int step);

static int server_is_running(void);

static void print_MEAS_buffer(void);
static void print_CMDS_buffer(void);

/************************************************************
* Local variables
************************************************************/

static int32_t current_hour;
static CommsStatus communication_status;

static ControlBuffer meas_buffer;
static ControlBuffer cmds_buffer;

static Timer comms_timer;

static FIFO out_meas_buffer;

static struct socket_singleton sockets[SOCKET_NUMBER] = {{0}};

/************************************************************
* Function definition
************************************************************/

/**
 * Listens on MEAS and CMDS ports and accepts a connection on each one.
 */
void startServers(const double t, const unsigned long queries_per_int, const unsigned long speed)
{
	if(server_is_running()) {
		ERROR("startServers: connections already started.\n");
	}

	OPEN_DEBUG("houseServer");

	int fds_left = SOCKET_NUMBER;
	struct pollfd fds[SOCKET_NUMBER] = {{0}};

	/* Create sockets. */
	if(0 > (sockets[SOCKET_CMDS].listen_fd = socketBuilder(CMDS_LISTEN_PORT, 1))) {
		ERROR("startServers: unable to create CMDS socket.\n");
	}

	if(0 > (sockets[SOCKET_MEAS].listen_fd = socketBuilder(MEAS_LISTEN_PORT, 1))) {
		ERROR("startServers: unable to create MEAS socket.\n");
	}

	/* Wait until both (all) connections are accepted. */
	do {
		buildPoll(fds, fds_left, sockets);

		if(0 < poll(fds, fds_left, -1)) {
			fds_left = acceptConnections(fds, fds_left, sockets);
		}
		else { /* Poll failure. */
			ERROR("startServers: poll failure.\n");
		}

	} while (0 < fds_left);

	DEBUG_PRINT("startServers: all connection accepted at simulation time %.2f.\n", t);

	/* Initialize timer */
	comms_timer = create_timer(speed, queries_per_int);
	if (NULL == comms_timer) {
		ERROR("startServers: unable to create timer.\n");
	}

	/* Initialize MEAS buffer FIFO */
	out_meas_buffer = fifo_init();
	if (NULL == out_meas_buffer) {
		ERROR("startServers: unable to create FIFO.\n");
	}

	/* Initialize buffers */
	meas_buffer = CB_init(MEAS_NUMBER);
	if (NULL == meas_buffer) {
		ERROR("startServers: unable to create MEAS control buffer.\n");
	}
	cmds_buffer = CB_init(CMDS_NUMBER);
	if (NULL == cmds_buffer) {
		ERROR("startServers: unable to create CMDS control buffer.\n");
	}

	current_hour = 1;
	if (CB_setControl(meas_buffer, &current_hour)) {
		ERROR("startServers: unable to set MEAS control.\n");
	}
	if (CB_setControl(cmds_buffer, &current_hour)) {
		ERROR("startServers: unable to set CMDS contro.\n");
	}

	communication_status = COMMS_MEAS_WAIT;
}

/**
 * Buffers the [val] in the [name] slot. Sends all values if the MEAS
 * buffer is full. Throws econnectrror if [name] variable has already been
 * buffered.
 */
double sendOM(const double val, const char * const name, const double t, const int32_t ctrl)
{
//	DEBUG_PRINT("sendOM: called with val: %f, name: %s, t: %f\n", val, name, t);
	if(!server_is_running()) {
		WARNING("sendOM: server not started or connection closed.\n");
		return 0.0;
	}

	if(NULL == name) {
		ERROR("sendOM: NULL pointer argument.\n");
	}

	int step = ((int) fmod(t, 60.0));

	send_meas(name, val, ctrl);
	advance(ctrl, step);

	return val;

}

/**
 * Returns the [name] variable received from CMDS socket. If the
 * [name] variable has already been taken, throws an error. If the
 * CMDS buffer is empty, loads data from the socket.
 */
double getOM(const double val, const char * const name, const double t, const int32_t ctrl)
{
//	DEBUG_PRINT("getOM: called with parameters (val: %f, name: \"%s\", t: %f)\n", val, name, t);
	if(!server_is_running()) {
		WARNING("getOM: server not started or connection closed.\n");
		return 0.0;
	}

	if(NULL == name) {
		ERROR("getOM: NULL pointer argument.\n");
	}

	int step = ((int) fmod(t, 60.0));

	advance(ctrl, step);

	return get_cmds(name, ctrl);
}

/************************************************************
* Communication functions
************************************************************/

static void advance(const int32_t ctrl, const int step)
{
	while (current_hour <= ctrl) {
		switch(communication_status) {
		case COMMS_MEAS_WAIT:
			if (recv_MEAS_ctrl(&communication_status, comms_timer, step)) {
				return;
			}
			break;

		case COMMS_MEAS_SEND:
			if (send_MEAS_buffer(&communication_status, step)) {
				return;
			}
			break;

		case COMMS_CMDS_WAIT:
			if (recv_CMDS_ctrl(&communication_status, comms_timer, step)) {
				return;
			}
			break;

		case COMMS_CMDS_RECV:
			if (recv_CMDS_buffer(&communication_status, comms_timer, step)) {
				return;
			}
			++current_hour;
			break;

		default:
			ERROR("advance: status %d is invalid.\n", communication_status);

		} /* Switch */
	} /* While */
}

static int recv_MEAS_ctrl(CommsStatus *status, Timer t, const int step)
{
	if (!read_possible(t, step, sockets[SOCKET_MEAS].accept_fd)) {
		return 1;
	}
	int32_t control_in;

	recv_complete(&sockets[SOCKET_MEAS], (char*) &control_in, sizeof (int32_t));

	DEBUG_PRINT("recv_MEAS_ctrl: MEAS control message from server is \"%d\".\n", control_in);

	*status = COMMS_MEAS_SEND;

	return 0;
}

static int send_MEAS_buffer(CommsStatus *status, const int step)
{
	if (NULL == fifo_peek(out_meas_buffer)) {
		return 1;
	}

	ControlBuffer extracted_meas_buffer;
	int32_t control_out;
	Measures meas_index;
	double value;

	extracted_meas_buffer = fifo_pop(out_meas_buffer);
	/* Holds: extracted_meas_buffer  is not NULL ! */

	if (CB_getControl(extracted_meas_buffer, &control_out)) {
		ERROR("advance: unable to extract control from MEAS buffer.\n");
	}
	send_complete(&sockets[SOCKET_MEAS], (char *) &control_out, sizeof (int32_t));
	DEBUG_PRINT("advance: sent MEAS control message \"%d\" back.\n", control_out);
	for (meas_index = 0; meas_index < MEAS_NUMBER; ++meas_index) {
		if (GB_getValue(CB_getBuffer(extracted_meas_buffer), meas_index, &value)) {
			ERROR("advance: unable to extract MEAS %d from MEAS buffer.\n", meas_index);
		}
		send_complete(&sockets[SOCKET_MEAS], (char *) &value, sizeof(double));
	}
	DEBUG_PRINT("advance: sent MEAS buffer.\n");
	if (CB_destroy(extracted_meas_buffer)) {
		ERROR("advance: unable to free MEAS buffer.\n");
	}
	*status = COMMS_CMDS_WAIT;

	return 0;
}

static int recv_CMDS_ctrl(CommsStatus *status, Timer t, const int step)
{
	if (!read_possible(t, step, sockets[SOCKET_CMDS].accept_fd)) {
		return 1;
	}
	int32_t control_in;

	recv_complete(&sockets[SOCKET_CMDS], (char *) &control_in, sizeof(int32_t));
	DEBUG_PRINT("advance: received CMDS control message \"%d\".\n", control_in);
	if (CB_setControl(cmds_buffer, &control_in)) {
		ERROR("advance: unable to set CMDS control.\n");
	}
	if (GB_empty(CB_getBuffer(cmds_buffer))) {
		ERROR("advance: unable to empty CMDS buffer.\n");
	}
	*status = COMMS_CMDS_RECV;

	return 0;
}

static int recv_CMDS_buffer(CommsStatus *status, Timer t, const int step)
{
	if (!read_possible(t, step, sockets[SOCKET_CMDS].accept_fd)) {
		return 1;
	}
	Commands cmds_index;
	double value;
	int32_t control_out;

	for(cmds_index = 0; cmds_index < CMDS_NUMBER; ++cmds_index) {
		recv_complete(&sockets[SOCKET_CMDS], (char *) &value, sizeof(double));
		if (GB_setValue(CB_getBuffer(cmds_buffer), cmds_index, &value)) {
			ERROR("advance: unable to set CMDS value.\n");
		}
	}
	print_CMDS_buffer();
	send_complete(&sockets[SOCKET_CMDS], (char *) &control_out, sizeof (int32_t));
	DEBUG_PRINT("advance: CMDS ack sent back to server.\n");
	*status = COMMS_MEAS_WAIT;

	return 0;
}

/************************************************************
* Get CMDS and send MEAS functions
************************************************************/

static double get_cmds(const char * const name, const int32_t ctrl)
{
	if(NULL == name) {
		ERROR("get_cmds: NULL pointer argument.\n");
	}

	double ret = 0.0;
	Commands index = get_CMDS_num_from_name(name);

	if((-1 == index) || (index >= CMDS_NUMBER)) {
		ERROR("get_cmds: unkwon name \"%s\".\n", name);
	}

	int32_t tmp_control = 0;
	if (CB_getControl(cmds_buffer, &tmp_control)) {
		ERROR("get_cmds: unable to get CMDS contorl.\n");
	}
	if (GB_isFull(CB_getBuffer(cmds_buffer)) && (ctrl == tmp_control)) {
		if (GB_getValue(CB_getBuffer(cmds_buffer), index, &ret)) {
			ERROR("get_cmds: unable to get CMDS %d.\n", index);
		}
	}

	return ret;
}

/**
 * @prec: must be called once per MEAS name, per time slot.
 */
static void send_meas(const char * const name, const double value, const int32_t ctrl)
{
	if (NULL == name) {
		ERROR("send_meas: NULL pointer argument.\n");
	}

	int32_t current_meas_control = 0, zero_ctrl = 0;
	Measures index;

	/* If the current ctrl differs from the control on the MEAS buffer,
	 * a new hour has begun. */
	if (CB_getControl(meas_buffer, &current_meas_control)) {
		ERROR("send_meas: unable to get MEAS control.\n");
	}
	if (ctrl != current_meas_control) {
		if (CB_setControl(meas_buffer, &ctrl)) {
			ERROR("send_meas: unable to set MEAS control.\n");
		}
		if (reset_timer(comms_timer)) {
			ERROR("send_meas: unable to reset timer.\n");
		}
	}

	/* Set value in MEAS buffer. */
	index = get_MEAS_num_from_name(name);
	if ((-1 == index) || (index >= MEAS_NUMBER)) {
		ERROR("send_meas: unkwon name \"%s\".\n", name);
	}
	if (GB_isSet(CB_getBuffer(meas_buffer), index)) {
		WARNING("send_meas: \"%s\" already set.\n", name);
	}
	if (GB_setValue(CB_getBuffer(meas_buffer), index, &value)) {
		ERROR("send_meas: unable to set MEAS buffer value.\n");
	}

	/* If the MEAS buffer is full, add it to the FIFO,
	 * create a new one, and set its control to 0 */
	if (GB_isFull(CB_getBuffer(meas_buffer))) {
		print_MEAS_buffer(); // Debug
		if (fifo_insert(out_meas_buffer, meas_buffer)) {
			ERROR("send_meas: unable to insert MEAS buffer in FIFO.\n");
		}
		meas_buffer = CB_init(MEAS_NUMBER);
		if (NULL == meas_buffer) {
			ERROR("send_meas: unable to create new MEAS control buffer.\n");
		}
		if (CB_setControl(meas_buffer, &zero_ctrl)) {
			ERROR("send_meas: unable to reset MEAS control.\n");
		}
	}
}

/************************************************************
* Local buffer utilities
************************************************************/

static int server_is_running(void)
{
	return (0 < sockets[SOCKET_CMDS].accept_fd) && (0 < sockets[SOCKET_MEAS].accept_fd);
}

static void print_MEAS_buffer(void)
{
	int32_t control = 0;
	if(CB_getControl(meas_buffer, &control)) {
		ERROR("print_MEAS_buffer: unable to get MEAS control.\n");
	}
	DEBUG_PRINT("MEAS buffer control is set to %d.\n", control);
	GB_print(CB_getBuffer(meas_buffer), (PrintFunction) get_MEAS_name_from_num);
}

static void print_CMDS_buffer(void)
{
	int32_t control = 0;
	if(CB_getControl(meas_buffer, &control)) {
		ERROR("print_CMDS_buffer: unable to get CMDS control.\n");
	}
	DEBUG_PRINT("CMDS buffer control is set to %d.\n", control);
	GB_print(CB_getBuffer(cmds_buffer), (PrintFunction) get_CMDS_name_from_num);
}

