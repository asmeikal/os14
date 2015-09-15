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
#include <errno.h>
#include <string.h>
#include <poll.h>

/************************************************************
* Local structs
************************************************************/

typedef enum status_t {
    STATUS_EMPTY = 0,
    STATUS_READY,
    STATUS_DELIVERED,
    STATUS_NUMBER
} Status;

struct buffer_val_double {
    double value;
    Status status;
};

struct buffer_val_int {
    long int value;
    Status status;
};

/************************************************************
* Local functions declaration
************************************************************/

static void send_all(void);
static void get_all(void);
static void get_one(void);

static void read_complete(int fd, char *buf, size_t count);
static void send_complete(int fd, char *buf, size_t count);

static void print_buffer(struct buffer_val_double b[]);
static unsigned char isempty_CMDS_buffer(void);
static unsigned char isfull_CMDS_buffer(void);
static unsigned char isdelivered_CMDS_buffer(void);
static unsigned char isfull_MEAS_buffer(void);

static void clean_MEAS_buffer(void);
static void clean_CMDS_buffer(void);

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
        ERROR("startServers: unable to create CMDS socket: %s\n", strerror(errno));
    }

    if(0 > (sockets[SOCKET_MEAS].listen_fd = socketBuilder(MEAS_LISTEN_PORT, 1))) {
        ERROR("startServers: unable to create MEAS socket: %s\n", strerror(errno));
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
    control_buffer[SOCKET_MEAS].status = STATUS_READY;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        meas_buffer[i].status = STATUS_READY;
    }

    clean_MEAS_buffer();

#ifndef USE_PHEV
    meas_buffer[MEAS_PHEV].status = meas_buffer[MEAS_PHEV_READY_HOURS].status = STATUS_READY;
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
    if(STATUS_EMPTY != control_buffer[SOCKET_MEAS].status) {
        ERROR("sendOMcontrol: control already set\n");
    }

    // PRINT_MSG("sendOMcontrol: got command at time %.10f\n", t);

    control_buffer[SOCKET_MEAS].status = STATUS_READY;
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

    if(STATUS_EMPTY != meas_buffer[n].status) {
        ERROR("sendOM: '%s' already set\n", name);
    }

    meas_buffer[n].value = val;
    meas_buffer[n].status = STATUS_READY;

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
long int getOMcontrol_NB(long int o, double t)
{
    if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
        return control_buffer[SOCKET_CMDS].value;
    }
    else {
        while((data_available(sockets[SOCKET_CMDS].accept_fd)) && 
              (0 == isfull_CMDS_buffer())) 
        {
            get_one();
        }

        if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
            return control_buffer[SOCKET_CMDS].value;
        }
        else {
            return o;
        }

    }
}

long int getOMcontrol(long int o, double t)
{
    long int ret;
    if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
        control_buffer[SOCKET_CMDS].status = STATUS_DELIVERED;
        ret = control_buffer[SOCKET_CMDS].value;
        if(0 != isdelivered_CMDS_buffer()) {
            clean_CMDS_buffer();
        }
        return ret;
    }
    else {
        while(0 != isfull_CMDS_buffer()) {
            wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
            get_one();
        }

        control_buffer[SOCKET_CMDS].status = STATUS_DELIVERED;
        ret = control_buffer[SOCKET_CMDS].value;
        if(0 != isdelivered_CMDS_buffer()) {
            clean_CMDS_buffer();
        }
        return ret;
    }
}

/**
 * Returns the [name] variable received from CMDS socket. If the
 * [name] variable has already been taken, throws an error. If the 
 * CMDS buffer is empty, loads data from the socket.
 */
double getOM_NB(double o, char *name, double t)
{
    if(NULL == name) {
        ERROR("getOM: NULL pointer argument\n");
    }

    Commands n = get_CMDS_num_from_name(name);
    if((-1 == n) || (n >= CMDS_NUMBER)) {
        ERROR("getOM: unkwon name '%s'\n", name);
    }

    if(STATUS_EMPTY != cmds_buffer[n].status) {
        return cmds_buffer[n].value;
    }
    else {
        while((data_available(sockets[SOCKET_CMDS].accept_fd)) && 
              (0 == isfull_CMDS_buffer())) 
        {
            get_one();
        }

        if(STATUS_EMPTY != cmds_buffer[n].status) {
            return cmds_buffer[n].value;
        }
        else {
            return o;
        }
    }
}

double getOM(double o, char *name, double t)
{
    if(NULL == name) {
        ERROR("getOM: NULL pointer argument\n");
    }

    Commands n = get_CMDS_num_from_name(name);
    if((-1 == n) || (n >= CMDS_NUMBER)) {
        ERROR("getOM: unkwon name '%s'\n", name);
    }

    double ret;
    if(STATUS_EMPTY != cmds_buffer[n].status) {
        cmds_buffer[n].status = STATUS_DELIVERED;
        ret = cmds_buffer[n].value;
        if(0 != isdelivered_CMDS_buffer()) {
            clean_CMDS_buffer();
        }
        return ret;
    }
    else {
        while(0 != isfull_CMDS_buffer()) {
            wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
            get_one();
        }

        cmds_buffer[n].status = STATUS_DELIVERED;
        ret = cmds_buffer[n].value;
        if(0 != isdelivered_CMDS_buffer()) {
            clean_CMDS_buffer();
        }
        return ret;
    }

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

    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
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
    meas_buffer[MEAS_PHEV].status = meas_buffer[MEAS_PHEV_READY_HOURS].status = STATUS_READY;
#endif
}

/**
 * Fills the CMDS buffer and the control variable with values
 * taken from the CMDS socket.
 */
static void get_all(void)
{
    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
        ERROR("get_all: sockets not started\n");
    }

    if(0 == isempty_CMDS_buffer()) {
        ERROR("get_all: CMDS buffer not empty\n");
    }

    int n;
    Commands i;

    /* Get control message */
    read_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(control_buffer[SOCKET_CMDS].value), sizeof(long int));
    control_buffer[SOCKET_CMDS].status = STATUS_READY;

    /* Get all CMDS values */
    for (i = 0; i < CMDS_NUMBER; i++) {
        read_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(cmds_buffer[i].value), sizeof(double));
        cmds_buffer[i].status = STATUS_READY;
    }

    /* Send control message through socket */
    long int ctrl_back = 0;
    send_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&ctrl_back, sizeof (long int));

#ifndef USE_PHEV
    cmds_buffer[CMDS_PHEV].status = STATUS_DELIVERED;
#endif

    /* Restart controller timer */
    set_timer();
}

static void get_one(void)
{
    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
        ERROR("get_one: sockets not started\n");
    }

    if(0 != isfull_CMDS_buffer()) {
        ERROR("get_one: CMDS buffer not empty\n");
    }

    if(STATUS_EMPTY == control_buffer[SOCKET_CMDS].status) {
        read_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(control_buffer[SOCKET_CMDS].value), sizeof(long int));
        control_buffer[SOCKET_CMDS].status = STATUS_READY;
        return;
    }
    else {
        Commands i;

        for(i = 0; i < CMDS_NUMBER; ++i) {
            if(STATUS_EMPTY == control_buffer[SOCKET_CMDS].status) {
                read_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(cmds_buffer[i].value), sizeof(double));
                cmds_buffer[i].status = STATUS_READY;
                return;
            }
        }

        ERROR("get_one: CMDS buffer was full\n");
    }
}

/************************************************************
* Socket read and write functions
************************************************************/

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

    ret &= (STATUS_EMPTY != control_buffer[SOCKET_MEAS].status);

    for(i = 0; i < MEAS_NUMBER; ++i) {
        ret &= (STATUS_EMPTY != meas_buffer[i].status);
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

    ret &= (STATUS_EMPTY == control_buffer[SOCKET_CMDS].status);

    for(i = 0; i < CMDS_NUMBER; ++i) {
        ret &= (STATUS_EMPTY == cmds_buffer[i].status);
    }

    return ret;
}

static unsigned char isfull_CMDS_buffer(void)
{
    unsigned char ret = 1;
    Commands i;

    ret &= (STATUS_EMPTY != control_buffer[SOCKET_CMDS].status);

    for(i = 0; i < CMDS_NUMBER; ++i) {
        ret &= (STATUS_EMPTY != cmds_buffer[i].status);
    }

    return ret;
}

static unsigned char isdelivered_CMDS_buffer(void)
{
    unsigned char ret = 1;
    Commands i;

    ret &= (STATUS_DELIVERED == control_buffer[SOCKET_CMDS].status);

    for(i = 0; i < CMDS_NUMBER; ++i) {
        ret &= (STATUS_DELIVERED == cmds_buffer[i].status);
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

    control_buffer[SOCKET_MEAS].status = STATUS_EMPTY;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        meas_buffer[i].status = STATUS_EMPTY;
    }
}

static void clean_CMDS_buffer(void)
{
    if(0 == isdelivered_CMDS_buffer()) {
        ERROR("clean_MEAS_buffer: buffer is not full\n");
    }

    Commands i;

    control_buffer[SOCKET_CMDS].status = STATUS_EMPTY;
    for(i = 0; i < CMDS_NUMBER; ++i) {
        cmds_buffer[i].status = STATUS_EMPTY;
    }

#ifndef USE_PHEV
    cmds_buffer[CMDS_PHEV].status = STATUS_DELIVERED;
#endif

    /* Restart controller timer */
    set_timer();
}

/**
 * Prints the double buffer [b].
 */
static void print_buffer(struct buffer_val_double b[])
{
    Measures i;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        if(STATUS_EMPTY == b[i].status) {
            DEBUG_PRINT("%d: not set\n", i);
        }
        else {
            DEBUG_PRINT("%d: %f\n", i, b[i].value);
        }
    }
}
