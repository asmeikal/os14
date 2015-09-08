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
    long int value;
    unsigned char isset;
};

/************************************************************
* Local functions declaration
************************************************************/

static void send_all(void);
static void get_all(void);

static void read_complete(int fd, char *buf, size_t count);
static void send_complete(int fd, char *buf, size_t count);

static void print_buffer(struct buffer_val_double b[]);
static void clean_MEAS_buffer(void);
static unsigned char isempty_CMDS_buffer(void);
static unsigned char isfull_MEAS_buffer(void);

/************************************************************
* Local variables
************************************************************/

struct buffer_val_int control_buffer[SOCKET_NUMBER] = {{0}};

struct buffer_val_double meas_buffer[MEAS_NUMBER] = {{0}};
struct buffer_val_double cmds_buffer[CMDS_NUMBER] = {{0}};

struct socket_singleton sockets[SOCKET_NUMBER] = {{0}};

/************************************************************
* Function definition
************************************************************/

/**
 * Listens on MEAS and CMDS ports and accepts a connection on each one.
 */
double startServers(double t)
{
    if((0 != sockets[SOCKET_CMDS].accept_fd) || (0 != sockets[SOCKET_MEAS].accept_fd)) {
        DEBUG_PRINT("startServers: connections already started\n");
        return t;
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

    PRINT_MSG("startServers: all connection accepted at time %f\n", t);
    // accept both connections

    Measures i;
    control_buffer[SOCKET_MEAS].isset = 1;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        meas_buffer[i].isset = 1;
    }

    clean_MEAS_buffer();

#ifndef USE_PHEV
    meas_buffer[MEAS_PHEV].isset = meas_buffer[MEAS_PHEV_READY_HOURS].isset = 1;
#endif

    set_timer();

    return t;
}

/**
 * Buffers the control message [val]. Sends all values if MEAS buffer
 * is full.
 * Throws error if control message has already been buffered.
 */
long int sendOMcontrol(long int val, double t)
{
    if(0 != control_buffer[SOCKET_MEAS].isset) {
        ERROR("sendOMcontrol: control already set\n");
    }

    // PRINT_MSG("sendOMcontrol: got command at time %.10f\n", t);

    control_buffer[SOCKET_MEAS].isset = 1;
    control_buffer[SOCKET_MEAS].value = val;

    if(0 != isfull_MEAS_buffer()) {
        send_all();
    }

    return val;
}

/**
 * Buffers the [val] in the [name] slot. Sends all values if the MEAS
 * buffer is full. Throws econnectrror if [name] variable has already been
 * buffered.
 */
double sendOM(double val, char *name, double t)
{
    if(NULL == name) {
        ERROR("sendOM: NULL pointer argument\n");
    }

    // PRINT_MSG("sendOM: got command at time %.10f\n", t);

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

    return val;
}

/**
 * Returns the control message received from CMDS socket. If the
 * control message has already been taken, throws an error. If the 
 * CMDS buffer is empty, loads data from the socket.
 */
long int getOMcontrol(double t)
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
double getOM(char *name, double t)
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
    static int count = 0;
    ++count;
    if(count != control_buffer[SOCKET_MEAS].value) {
        ERROR("ERROR: send_all: called %d times, but control is %ld\n", count, control_buffer[SOCKET_MEAS].value);
    }

    // DEBUG_PRINT("send_all: sending\n");
    // DEBUG_PRINT("control: %d\n", control_buffer[SOCKET_MEAS].value);
    // print_buffer(meas_buffer);

    if((0 == sockets[SOCKET_CMDS].accept_fd) || (0 == sockets[SOCKET_MEAS].accept_fd)) {
        ERROR("send_all: sockets not started\n");
    }

    if(0 == isfull_MEAS_buffer()) {
        ERROR("send_all: MEAS buffer not full\n");
    }

    int n;
    Measures i;

    /* Send control message */
    send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&(control_buffer[SOCKET_MEAS].value), sizeof (long int));

    /* Send all values in the MEAS buffer */
    for (i = 0; i < MEAS_NUMBER; i++) {
        send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&meas_buffer[i], sizeof(double));
    }

    /* Empty the MEAS buffer and control variable */
    clean_MEAS_buffer();

#ifndef USE_PHEV
    meas_buffer[MEAS_PHEV].isset = meas_buffer[MEAS_PHEV_READY_HOURS].isset = 1;
#endif
}

static void read_complete(int fd, char *buf, size_t count)
{
    if(0 >= count) {
        ERROR("read_complete: can't read %d bytes\n", count);
    }

    size_t n = 0, r;

    while(n < count) {
        wait_for_answer(fd);

        r = recv(fd, buf+n, count - n, 0);
        if(0 >= r) {
            ERROR("read_complete: recv failed\n");
        }
        n += r;
    }

}

static void send_complete(int fd, char *buf, size_t count)
{
    if(0 >= count) {
        ERROR("send_complete: can't send %d bytes\n", count);
    }

    size_t n = 0, s;

    while(n < count) {
        s = send(fd, buf+n, count - n, 0);
        if(0 >= s) {
            ERROR("send_complete: send failed\n");
        }
        n += s;
    }
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

    if(0 == isempty_CMDS_buffer()) {
        ERROR("get_all: CMDS buffer not empty\n");
    }

    int n;
    Commands i;

    /* Get control message */
    read_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(control_buffer[SOCKET_CMDS].value), sizeof(long int));
    control_buffer[SOCKET_CMDS].isset = 1;

    /* Get all CMDS values */
    for (i = 0; i < CMDS_NUMBER; i++) {
        read_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(cmds_buffer[i].value), sizeof(double));
        cmds_buffer[i].isset = 1;
    }

    /* Send control message through socket */
    long int ctrl_back = 0;
    send_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&ctrl_back, sizeof (long int));

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
    if(0 == isfull_MEAS_buffer()) {
        ERROR("clean_MEAS_buffer: buffer is not full\n");
    }

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
