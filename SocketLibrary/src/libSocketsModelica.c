#include <libSocketsModelica.h>

#include <Debug.h>
#include <Sockets.h>
#include <Timer.h>
#include <Buffers.h>
#include <House.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>

/************************************************************
* Local functions declaration
************************************************************/

static void send_all_vars(void);
static void recv_next_var(void);

static void send_CMDS_ack(void);

static void reset_MEAS_buffer(const double reset_time);
static void reset_CMDS_buffer(const double reset_time);

static int isfull_MEAS_buffer(void);
static int isfull_CMDS_buffer(void);
static int isdelivered_CMDS_buffer(void);

static int server_is_running(void);

static void print_MEAS_buffer(void);
static void print_CMDS_buffer(void);

/************************************************************
* Local variables
************************************************************/

GBuffer meas_buffer;
GBuffer cmds_buffer;

long int cmds_control;

struct socket_singleton sockets[SOCKET_NUMBER] = {{0}};

double last_meas_time;
double last_cmds_time;
double granularity;

/************************************************************
* Function definition
************************************************************/

/**
 * Listens on MEAS and CMDS ports and accepts a connection on each one.
 */
void startServers(const double t, const double comms_time_int, const unsigned long queries_per_int, const unsigned long speed)
{
    if(0.0 >= comms_time_int) {
        ERROR("startServers: \"%f seconds\" is not a valid time interval.\n", comms_time_int);
    }

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
        else {
            /* Poll failure. */
            ERROR("startServers: poll failure.\n");
        }

    } while (0 < fds_left);

    DEBUG_PRINT("startServers: all connection accepted at simulation time %.2f.\n", t);

    /* Initialize buffers */
    meas_buffer = GB_initDouble(MEAS_NUMBER);
    cmds_buffer = GB_initDouble(CMDS_NUMBER);

    cmds_control = -1;

    granularity = comms_time_int;
    setup_timer(speed, queries_per_int);

    reset_MEAS_buffer(t - comms_time_int);
    reset_CMDS_buffer(t);

}

/**
 * Buffers the [val] in the [name] slot. Sends all values if the MEAS
 * buffer is full. Throws econnectrror if [name] variable has already been
 * buffered.
 */
double sendOM(const double val, const char * const name, const double t)
{
//    DEBUG_PRINT("sendOM: called with val: %f, name: %s, t: %f\n", val, name, t);
    if(!server_is_running()) {
        WARNING("sendOM: server has not been started yet.\n");
        return val;
    }

    if(NULL == name) {
        ERROR("sendOM: NULL pointer argument.\n");
    }

    Measures n = get_MEAS_num_from_name(name);
    if((-1 == n) || (n >= MEAS_NUMBER)) {
        ERROR("sendOM: unkwon name \"%s\".\n", name);
    }

    if(t >= (last_meas_time + granularity)) {
        /* Last time we sent was more than an hour ago */

        if(GB_isSet(meas_buffer, n)) {
            WARNING("sendOM: \"%s\" already set.\n", name);
        }

        GB_setDoubleValue(meas_buffer, n, val);

        if(isfull_MEAS_buffer() &&
           data_available(sockets[SOCKET_MEAS].accept_fd)) 
        {
            /* The buffer is full, send it */
            DEBUG_PRINT("sendOM: server asked for measurements at time %.2f.\n", t);
            send_all_vars();
            print_MEAS_buffer();
            /* Empty the MEAS buffer and control variable */
            reset_MEAS_buffer(t);
        }
    }

    return val;
}

/**
 * Returns the [name] variable received from CMDS socket. If the
 * [name] variable has already been taken, throws an error. If the 
 * CMDS buffer is empty, loads data from the socket.
 */
double getOM(const double val, const char * const name, const double t)
{
//    DEBUG_PRINT("getOM: called with parameters (val: %f, name: \"%s\", t: %f)\n", val, name, t);
    if(!server_is_running()) {
        WARNING("getOM: server has not been started yet.\n");
        return val;
    }

    if(NULL == name) {
        ERROR("getOM: NULL pointer argument.\n");
    }

    double ret;
    Commands n = get_CMDS_num_from_name(name);

    if((-1 == n) || (n >= CMDS_NUMBER)) {
        ERROR("getOM: unkwon name \"%s\".\n", name);
    }

    if(t >= (last_cmds_time + granularity)) 
    {
        /* One hour has passed, get all commands */

        /* If CMDS buffers are not full... */
        while(!isfull_CMDS_buffer()) 
        {
            /* ... wait and receive */
            if(!wait_for_answer(sockets[SOCKET_CMDS].accept_fd)) {
                ERROR("getOM: controller timed out.\n");
            }

            recv_next_var();

            /* Debug: print buffer when received all */
            if(isfull_CMDS_buffer()) {
                DEBUG_PRINT("getOM: received commands after %.2f time units.\n", fmod(t, granularity));
                print_CMDS_buffer();
            }
        }

        ret = GB_getDoubleValue(cmds_buffer, n);
        /* Set current var as delivered */
        GB_markAsDelivered(cmds_buffer, n);

        /* If all have been delivered, reset buffer */
        if(isdelivered_CMDS_buffer()) 
        {
            send_CMDS_ack();
            reset_CMDS_buffer(t);
        }
    }
    else {
        /* We can still wait */

        /* Receive only if data available and buffer not full */
        while(!isfull_CMDS_buffer() &&
              data_available(sockets[SOCKET_CMDS].accept_fd)) 
        {
            recv_next_var();

            /* Debug: print buffer when received all */
            if(isfull_CMDS_buffer()) {
                DEBUG_PRINT("getOM: received commands after %.2f time units.\n", fmod(t, granularity) );
                print_CMDS_buffer();
            }
        }

        /* If we already have new data, deliver it. 
         * Otherwise, deliver the old value */
        if(GB_isSet(cmds_buffer, n)) {
            ret = GB_getDoubleValue(cmds_buffer, n);
        }
        else {
            ret = val;
        }

    }

    return ret;
}

/************************************************************
* Local function definition
************************************************************/

/**
 * Sends all MEAS buffer values + the MEAS control through the MEAS
 * socket.
 */
static void send_all_vars(void)
{
    if(!server_is_running()) {
        ERROR("send_all_vars: sockets not started.\n");
    }

    if(!isfull_MEAS_buffer()) {
        ERROR("send_all_vars: MEAS buffer is not full.\n");
    }

    Measures i;
    long int message_in;
    double meas_out;

    /* Receive control message from server */
    recv_complete(sockets[SOCKET_MEAS].accept_fd, (char*)&message_in, sizeof (long int));
    DEBUG_PRINT("send_all_vars: control message from server is \"%ld\".\n", message_in);

    /* Send back control message */
    send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&message_in, sizeof (long int));
    DEBUG_PRINT("send_all_vars: sent control message \"%ld\" back.\n", message_in);

    /* Send all values in the MEAS buffer */
    for (i = 0; i < MEAS_NUMBER; i++) {
        meas_out = GB_getDoubleValue(meas_buffer, i);
        send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&meas_out, sizeof(double));
    }

}

/**
 * If the CMDS buffer is not full, receives a value from the CMDS socket.
 * The expected order is CMDS_CONTROL, CMDS_BATTERY, CMDS_PHEV.
 */
static void recv_next_var(void)
{
    if(!server_is_running()) {
        ERROR("recv_next_var: sockets not started.\n");
    }

    if(isfull_CMDS_buffer()) {
        ERROR("recv_next_var: CMDS buffer is full.\n");
    }

    if(0 > cmds_control) {
        /* Control is missing, get it */
        recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&cmds_control, sizeof(long int));
        DEBUG_PRINT("recv_next_var: received control message \"%ld\".\n", cmds_control);
        return;
    }
    else {
        /* Find the missing command and get it */
        Commands i;
        double cmds_in;

        for(i = 0; i < CMDS_NUMBER; ++i) {
            if(!GB_isSet(cmds_buffer, i)) {
                DEBUG_PRINT("recv_next_var: received command \"%s\".\n", get_CMDS_name_from_num(i));
                recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&cmds_in, sizeof(double));
                GB_setDoubleValue(cmds_buffer, i, cmds_in);

                return;
            }
        }

    }

    /* Oops! The buffer was full! */
    ERROR("recv_next_var: CMDS buffer was full!\n");
}

/************************************************************
* Local buffer utilities
************************************************************/

static void reset_MEAS_buffer(const double reset_time)
{
    GB_empty(meas_buffer);

    last_meas_time = reset_time;
}

/**
 * This should be sent after each received command block. A command block
 * is the triplet (CONTROL, CMDS_BATTERY, CMDS_PHEV).
 */
static void send_CMDS_ack(void)
{
    long int ctrl_back = 0;
    send_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&ctrl_back, sizeof (long int));    
    DEBUG_PRINT("send_CMDS_ack: ack sent back to server.\n");
}

static void reset_CMDS_buffer(const double reset_time)
{
    // DEBUG_PRINT("reset_CMDS_buffer: resetting at time %f\n", reset_time);

    cmds_control = -1;
    GB_empty(cmds_buffer);

    /* Restart controller timer */
    reset_timer();
    last_cmds_time = reset_time;

}

static int server_is_running(void) 
{
    return (0 != sockets[SOCKET_CMDS].accept_fd) || (0 != sockets[SOCKET_MEAS].accept_fd);
}

static int isfull_MEAS_buffer(void)
{
    return GB_isFull(meas_buffer);
}

static int isfull_CMDS_buffer(void)
{
    return (0 <= cmds_control) && GB_isFull(cmds_buffer);
}

static int isdelivered_CMDS_buffer(void)
{
    return GB_isAllDelivered(cmds_buffer);
}

static void print_MEAS_buffer(void)
{
    GB_printDecorated(meas_buffer, (PrintFunction) get_MEAS_name_from_num);
}

static void print_CMDS_buffer(void)
{
    GB_printDecorated(cmds_buffer, (PrintFunction) get_CMDS_name_from_num);
}

