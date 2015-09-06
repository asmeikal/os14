#include <libSocketsModelica.h>

#include <Debug.h>
#include <Sockets.h>
#include <Timer.h>
#include <House.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

/************************************************************
* Local structs
************************************************************/

struct buffer_val_double {
    double value;
    unsigned char isset;
};

struct buffer_val_int {
    int value;
    unsigned char isset;
};

/************************************************************
* Local functions declaration
************************************************************/

static void send_all(void);
static void get_all(void);

static void print_buffer(struct buffer_val_double b[]);
static void clean_MEAS_buffer(void);
static unsigned char isempty_CMDS_buffer(void);
static unsigned char isfull_MEAS_buffer(void);

/************************************************************
* Local variables
************************************************************/

struct buffer_val_int control_buffer[SOCKET_NUMBER];

struct buffer_val_double meas_buffer[MEAS_NUMBER];
struct buffer_val_double cmds_buffer[CMDS_NUMBER];

struct socket_singleton sockets[SOCKET_NUMBER] = {{0}};

/************************************************************
* Function definition
************************************************************/

/**
 * Listens on MEAS and CMDS ports and accepts a connection on each one.
 */
void startServers(void)
{
    if((0 != sockets[SOCKET_CMDS].accept_fd) || (0 != sockets[SOCKET_MEAS].accept_fd)) {
        DEBUG_PRINT("startServers: connections already started\n");
        return;
    }

    int fds_left = SOCKET_NUMBER;
    struct pollfd fds[SOCKET_NUMBER] = {{0}};

    /* Create sockets. */
    if(0 > (sockets[SOCKET_CMDS].listen_fd = socketBuilder(CMDS_LISTEN_PORT, 1))) {
        ERROR("startServers: unable to create CMDS socket\n");
    }

    if(0 > (sockets[SOCKET_MEAS].listen_fd = socketBuilder(MEAS_LISTEN_PORT, 1))) {
        ERROR("startServers: unable to create MEAS socket\n");
    }

    /* Wait until both (all) connections are accepted. */
    do {
        buildPoll(fds, fds_left, sockets);

        if(0 < poll(fds, fds_left, -1)) {
            fds_left = acceptConnections(fds, fds_left, sockets);
        }
        else {
            /* Poll failure. */
            ERROR("startServers: poll failure\n");
        }

    } while (0 < fds_left);

    PRINT_MSG("startServers: all connection accepted\n");
    // accept both connections

    clean_MEAS_buffer();

#ifndef USE_PHEV
    meas_buffer[MEAS_PHEV].isset = meas_buffer[MEAS_PHEV_READY_HOURS].isset = 1;
#endif

    set_timer();
}

/**
 * Buffers the control message [val]. Sends all values if MEAS buffer
 * is full.
 * Throws error if control message has already been buffered.
 */
void sendOMcontrol(long int val)
{
    if(0 != control_buffer[SOCKET_MEAS].isset) {
        ERROR("sendOMcontrol: control already set\n");
    }

    control_buffer[SOCKET_MEAS].value = val;
    control_buffer[SOCKET_MEAS].isset = 1;

    if(0 != isfull_MEAS_buffer()) {
        send_all();
    }
}

/**
 * Buffers the [val] in the [name] slot. Sends all values if the MEAS
 * buffer is full. Throws error if [name] variable has already been
 * buffered.
 */
void sendOM(double val, char *name)
{
    if(NULL == name) {
        ERROR("sendOM: NULL pointer argument\n");
    }

    Measures n = get_MEAS_num_from_name(name);
    if((-1 == n) || (n >= MEAS_NUMBER)) {
        ERROR("sendOM: unkwon name '%s'\n", name);
    }

    if(0 != meas_buffer[n].isset) {
        ERROR("sendOM: '%s' already set\n", name);
    }

    meas_buffer[n].value = val;
    meas_buffer[n].isset = 1;

    if(0 != isfull_MEAS_buffer()) {
        send_all();
    }
}

/**
 * Returns the control message received from CMDS socket. If the
 * control message has already been taken, throws an error. If the 
 * CMDS buffer is empty, loads data from the socket.
 */
long int getOMcontrol(void)
{
    if(0 != isempty_CMDS_buffer()) {
        get_all();
    }

    if(0 == control_buffer[SOCKET_CMDS].isset) {
        ERROR("getOMcontrol: no value available\n");
    }

    long int ret = control_buffer[SOCKET_CMDS].value;
    control_buffer[SOCKET_CMDS].isset = 0;

    return ret;
}

/**
 * Returns the [name] variable received from CMDS socket. If the
 * [name] variable has already been taken, throws an error. If the 
 * CMDS buffer is empty, loads data from the socket.
 */
double getOM(char *name)
{
    if(NULL == name) {
        ERROR("getOM: NULL pointer argument\n");
    }

    if(0 != isempty_CMDS_buffer()) {
        get_all();
    }

    Commands n = get_CMDS_num_from_name(name);
    if((-1 == n) || (n >= CMDS_NUMBER)) {
        ERROR("getOM: unkwon name '%s'\n", name);
    }

    if(0 == cmds_buffer[n].isset) {
        ERROR("getOM: no value available for '%s'\n", name);
    }

    double ret = cmds_buffer[n].value;
    cmds_buffer[n].isset = 0;

    return ret;

}

/************************************************************
* Local function definition
************************************************************/

/**
 * Sends all MEAS buffer values + the MEAS control through the MEAS
 * socket.
 */
static void send_all(void)
{
    if((0 == sockets[SOCKET_CMDS].accept_fd) || (0 == sockets[SOCKET_MEAS].accept_fd)) {
        ERROR("send_all: sockets not started\n");
    }

    int n;
    Measures i;

    /* Send control message */
    n = send(sockets[SOCKET_MEAS].accept_fd, (char *)&control_buffer[SOCKET_MEAS].value, sizeof (long int), 0);
    if(!(n == sizeof (long int))) {
        ERROR("send_all: sent only %d bytes\n", n);
    }

    /* Send all values in the MEAS buffer */
    for (i = 0; i < MEAS_NUMBER; i++) {
        n = send(sockets[SOCKET_MEAS].accept_fd, (char *)&meas_buffer[i], sizeof(double), 0);
        if(!(n == sizeof (double))) {
            ERROR("send_all: sent only %d bytes\n", n);
        }
    }

    /* Empty the MEAS buffer and control variable */
    clean_MEAS_buffer();

#ifndef USE_PHEV
    meas_buffer[MEAS_PHEV].isset = meas_buffer[MEAS_PHEV_READY_HOURS].isset = 1;
#endif
}

/**
 * Fills the CMDS buffer and the control variable with values
 * taken from the CMDS socket.
 */
static void get_all(void)
{
    if((0 == sockets[SOCKET_CMDS].accept_fd) || (0 == sockets[SOCKET_MEAS].accept_fd)) {
        ERROR("get_all: sockets not started\n");
    }

    int n;
    Commands i;

    /* Wait for data */
    wait_for_answer(sockets[SOCKET_CMDS].accept_fd);

    /* Get control message */
    n = recv(sockets[SOCKET_CMDS].accept_fd, (char *)&control_buffer[SOCKET_CMDS].value, sizeof(long int), 0);
    if(!(n == sizeof (long int))) {
        ERROR("get_all: read only %d bytes\n", n);
    }
    control_buffer[SOCKET_CMDS].isset = 1;

    /* Get all CMDS values */
    for (i = 0; i < CMDS_NUMBER; i++) {
        wait_for_answer(sockets[SOCKET_CMDS].accept_fd);

        n = recv(sockets[SOCKET_CMDS].accept_fd, (char *)&(cmds_buffer[i].value), sizeof(double), 0);
        if(!(n == sizeof (double))) {
            ERROR("get_all: read only %d bytes\n", n);
        }
        cmds_buffer[i].isset = 1;
    }

    /* Send control message through socket */
    n = send(sockets[SOCKET_CMDS].accept_fd, (char *)&control_buffer[SOCKET_CMDS].value, sizeof (long int), 0);
    if(!(n == sizeof (long int))) {
        ERROR("send_control_ack: sent only %d bytes\n", n);
    }

#ifndef USE_PHEV
    cmds_buffer[CMDS_PHEV].isset = 0;
#endif

    /* Restart controller timer */
    set_timer();
}

/************************************************************
* Local buffer utilities
************************************************************/

/**
 * Returns 0 if all MEAS variables are set.
 */
static unsigned char isfull_MEAS_buffer(void)
{
    unsigned char ret = 1;
    Measures i;

    ret &= (0 != control_buffer[SOCKET_MEAS].isset);

    for(i = 0; i < MEAS_NUMBER; ++i) {
        ret &= (0 != meas_buffer[i].isset);
    }

    return ret;
}

/**
 * Returns 0 if there are 1+ CMDS variables set.
 */
static unsigned char isempty_CMDS_buffer(void)
{
    unsigned char ret = 1;
    Commands i;

    ret &= (0 == control_buffer[SOCKET_CMDS].isset);

    for(i = 0; i < CMDS_NUMBER; ++i) {
        ret &= (0 == cmds_buffer[i].isset);
    }

    return ret;
}

/**
 * Unsets all MEAS variables.
 */
static void clean_MEAS_buffer(void)
{
    Measures i;

    control_buffer[SOCKET_MEAS].isset = 0;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        meas_buffer[i].isset = 0;
    }
}

/**
 * Prints the double buffer [b].
 */
static void print_buffer(struct buffer_val_double b[])
{
    Measures i;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        if(0 == b[i].isset) {
            DEBUG_PRINT("%d: not set\n", i);
        }
        else {
            DEBUG_PRINT("%d: %f\n", i, b[i].value);
        }
    }
}
