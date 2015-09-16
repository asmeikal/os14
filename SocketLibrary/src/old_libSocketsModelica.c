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
#include <math.h>

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

static void send_all_vars(void);
// static void get_all(void);
static void recv_next_var(void);

static void print_MEAS_buffer(void);
static void print_CMDS_buffer(void);

static unsigned char isempty_CMDS_buffer(void);
static unsigned char isfull_CMDS_buffer(void);
static unsigned char isdelivered_CMDS_buffer(void);
static unsigned char isfull_MEAS_buffer(void);

static void clean_MEAS_buffer(void);
static void clean_CMDS_buffer(void);
static void reset_MEAS_buffer(void);
static void reset_CMDS_buffer(double reset_time);

/************************************************************
* Local variables
************************************************************/

struct buffer_val_int control_buffer[SOCKET_NUMBER] = {{0}};

struct buffer_val_double meas_buffer[MEAS_NUMBER] = {{0}};
struct buffer_val_double cmds_buffer[CMDS_NUMBER] = {{0}};

struct socket_singleton sockets[SOCKET_NUMBER] = {{0}};

double global_last_time_sent;

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

    DEBUG_PRINT("startServers: all connection accepted at time %f\n", t);

    reset_MEAS_buffer();
    reset_CMDS_buffer(-INFINITY);

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
        /* We already have a buffered value... something went wrong */
        ERROR("sendOMcontrol: control already set\n");
    }

    control_buffer[SOCKET_MEAS].status = STATUS_READY;
    control_buffer[SOCKET_MEAS].value = val;

    if(0 != isfull_MEAS_buffer()) {
        /* The buffer is full, send it */
        send_all_vars();
        /* Now the buffer is empty */
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
        /* The buffer is full, send it */
        send_all_vars();
        /* Now the buffer is empty */
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

    long int ret;
    // DEBUG_PRINT("getOMcontrol_NB: called with (o: %ld, t: %f)\n", o, t);
    if((t != global_last_time_sent) &&
       (0.0 == fmod(t, 60.0))) 
    {
        // DEBUG_PRINT("getOMcontrol_NB: calling blocking function at time %f\n", t);
        if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
            control_buffer[SOCKET_CMDS].status = STATUS_DELIVERED;
            ret = control_buffer[SOCKET_CMDS].value;
            if(0 != isdelivered_CMDS_buffer()) {
                reset_CMDS_buffer(t);
            }
            return ret;
        }
        else {
            while(0 == isfull_CMDS_buffer()) {
                // DEBUG_PRINT("getOMcontrol: waiting\n");
                wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
                recv_next_var();
                // print_CMDS_buffer();
            }

            control_buffer[SOCKET_CMDS].status = STATUS_DELIVERED;
            ret = control_buffer[SOCKET_CMDS].value;
            if(0 != isdelivered_CMDS_buffer()) {
                reset_CMDS_buffer(t);
            }
            return ret;
        }
    }
    else {
        if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
            return control_buffer[SOCKET_CMDS].value;
        }
        else {
            while((data_available(sockets[SOCKET_CMDS].accept_fd)) && 
                  (0 == isfull_CMDS_buffer())) 
            {
                // DEBUG_PRINT("getOMcontrol_NB: there is data available\n");
                recv_next_var();
                // DEBUG_PRINT("getOMcontrol_NB: all data read\n");
            }

            if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
                return control_buffer[SOCKET_CMDS].value;
            }
            else {
                return o;
            }

        }

    }
}

// TODO: remove this
// long int getOMcontrol(long int o, double t)
// {
//     long int ret;
//     if(STATUS_EMPTY != control_buffer[SOCKET_CMDS].status) {
//         control_buffer[SOCKET_CMDS].status = STATUS_DELIVERED;
//         ret = control_buffer[SOCKET_CMDS].value;
//         if(0 != isdelivered_CMDS_buffer()) {
//             reset_CMDS_buffer(t);
//         }
//         return ret;
//     }
//     else {
//         while(0 == isfull_CMDS_buffer()) {
//             // DEBUG_PRINT("getOMcontrol: waiting\n");
//             wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
//             recv_next_var();
//             // print_CMDS_buffer();
//         }

//         control_buffer[SOCKET_CMDS].status = STATUS_DELIVERED;
//         ret = control_buffer[SOCKET_CMDS].value;
//         if(0 != isdelivered_CMDS_buffer()) {
//             reset_CMDS_buffer(t);
//         }
//         return ret;
//     }
// }

/**
 * Returns the [name] variable received from CMDS socket. If the
 * [name] variable has already been taken, throws an error. If the 
 * CMDS buffer is empty, loads data from the socket.
 */
double getOM_NB(double o, char *name, double t)
{
    // DEBUG_PRINT("getOM_NB: called with (o: %f, name: \"%s\" t: %f)\n", o, name, t);

    if(NULL == name) {
        ERROR("getOM_NB: NULL pointer argument\n");
    }

    double ret;
    Commands n = get_CMDS_num_from_name(name);

    if((-1 == n) || (n >= CMDS_NUMBER)) {
        ERROR("getOM_NB: unkwon name '%s'\n", name);
    }

    if((t != global_last_time_sent) &&
       (0.0 == fmod(t, 60.0))) 
    {
        /* One hour of simulation passed: recv all from socket */
        if(STATUS_EMPTY != cmds_buffer[n].status) {
            cmds_buffer[n].status = STATUS_DELIVERED;
            ret = cmds_buffer[n].value;
            if(0 != isdelivered_CMDS_buffer()) {
                reset_CMDS_buffer(t);
            }
            return ret;
        }
        else {
            while(0 == isfull_CMDS_buffer()) {
                // DEBUG_PRINT("getOM: waiting\n");
                wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
                recv_next_var();
                // print_CMDS_buffer();
            }

            cmds_buffer[n].status = STATUS_DELIVERED;
            ret = cmds_buffer[n].value;
            if(0 != isdelivered_CMDS_buffer()) {
                reset_CMDS_buffer(t);
            }
            return ret;
        }
    }
    else {
        /* We still have time... get only if available */
        if(STATUS_EMPTY != cmds_buffer[n].status) {
            /* We have data, we can return it */
            return cmds_buffer[n].value;
        }
        else {
            /* We don't have data... we check if it's available... */
            while((data_available(sockets[SOCKET_CMDS].accept_fd)) && 
                  (0 == isfull_CMDS_buffer())) 
            {
                // DEBUG_PRINT("getOM_NB: there is data available\n");
                recv_next_var();
                // DEBUG_PRINT("getOM_NB: all data read\n");
            }

            /* If we did get something, we return it, otherwise return the old value */
            if(STATUS_EMPTY != cmds_buffer[n].status) {
                return cmds_buffer[n].value;
            }
            else {
                return o;
            }
        }
    }
}

// TODO: remove this
// double getOM(double o, char *name, double t)
// {
//     if(NULL == name) {
//         ERROR("getOM: NULL pointer argument\n");
//     }

//     Commands n = get_CMDS_num_from_name(name);
//     if((-1 == n) || (n >= CMDS_NUMBER)) {
//         ERROR("getOM: unkwon name '%s'\n", name);
//     }

// }

/************************************************************
* Local function definition
************************************************************/

/**
 * Sends all MEAS buffer values + the MEAS control through the MEAS
 * socket.
 */
static void send_all_vars(void)
{
    static int count = 0;
    ++count;
    if(count != control_buffer[SOCKET_MEAS].value) {
        ERROR("ERROR: send_all_vars: called %d times, but control is %ld\n", count, control_buffer[SOCKET_MEAS].value);
    }

    // DEBUG_PRINT("send_all_vars: sending\n");
    // DEBUG_PRINT("control: %d\n", control_buffer[SOCKET_MEAS].value);
    // print_MEAS_buffer();

    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
        ERROR("send_all_vars: sockets not started\n");
    }

    if(0 == isfull_MEAS_buffer()) {
        ERROR("send_all_vars: MEAS buffer not full\n");
    }

    int n;
    Measures i;

    /* Receive control message from server */
    long int message_in;
    recv_complete(sockets[SOCKET_MEAS].accept_fd, (char*)&message_in, sizeof (long int));
    DEBUG_PRINT("send_all_vars: control message from server is %ld\n", message_in);

    /* Send back control message */
    send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&(control_buffer[SOCKET_MEAS].value), sizeof (long int));
    DEBUG_PRINT("send_all_vars: sent control message %ld\n", control_buffer[SOCKET_MEAS].value);

    /* Send all values in the MEAS buffer */
    for (i = 0; i < MEAS_NUMBER; i++) {
        send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&meas_buffer[i], sizeof(double));
    }

    /* Empty the MEAS buffer and control variable */
    clean_MEAS_buffer();

#if defined (NO_PHEV)
    meas_buffer[MEAS_PHEV].status = meas_buffer[MEAS_PHEV_READY_HOURS].status = STATUS_READY;
#endif

}

// TODO: remove this
// /**
//  * Fills the CMDS buffer and the control variable with values
//  * taken from the CMDS socket.
//  */
// static void get_all(void)
// {
//     if((0 == sockets[SOCKET_CMDS].accept_fd) || 
//        (0 == sockets[SOCKET_MEAS].accept_fd)) 
//     {
//         ERROR("get_all: sockets not started\n");
//     }

//     if(0 == isempty_CMDS_buffer()) {
//         ERROR("get_all: CMDS buffer not empty\n");
//     }

//     int n;
//     Commands i;

//     /* Get control message */
//     recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(control_buffer[SOCKET_CMDS].value), sizeof(long int));
//     control_buffer[SOCKET_CMDS].status = STATUS_READY;

//     /* Get all CMDS values */
//     for (i = 0; i < CMDS_NUMBER; i++) {
//         recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(cmds_buffer[i].value), sizeof(double));
//         cmds_buffer[i].status = STATUS_READY;
//     }

//     /* Send control message through socket */
//     long int ctrl_back = 0;
//     send_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&ctrl_back, sizeof (long int));

// #if defined (NO_PHEV)
//     cmds_buffer[CMDS_PHEV].status = STATUS_DELIVERED;
// #endif

//     /* Restart controller timer */
//     reset_timer();
// }

static void recv_next_var(void)
{
    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
        ERROR("recv_next_var: sockets not started\n");
    }

    if(0 != isfull_CMDS_buffer()) {
        ERROR("recv_next_var: CMDS buffer not empty\n");
    }

    // DEBUG_PRINT("recv_next_var: receiving\n");
    if(STATUS_EMPTY == control_buffer[SOCKET_CMDS].status) {
        /* Control is missing, get it */
        recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(control_buffer[SOCKET_CMDS].value), sizeof(long int));
        control_buffer[SOCKET_CMDS].status = STATUS_READY;
        return;
    }
    else {
        /* Find the missing command and get it */
        Commands i;

        for(i = 0; i < CMDS_NUMBER; ++i) {
            if(STATUS_EMPTY == cmds_buffer[i].status) {
                recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&(cmds_buffer[i].value), sizeof(double));
                cmds_buffer[i].status = STATUS_READY;
#if defined (NO_PHEV)
    if(i == CMDS_PHEV) {
        cmds_buffer[CMDS_PHEV].status = STATUS_DELIVERED;
    }
#endif
                return;
            }
        }

    }

    /* Oops! The buffer was full! */
    ERROR("recv_next_var: CMDS buffer was full\n");
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

static void reset_MEAS_buffer(void)
{
    Measures i;
    control_buffer[SOCKET_MEAS].status = STATUS_READY;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        meas_buffer[i].status = STATUS_READY;
    }

    clean_MEAS_buffer();

#if defined (NO_PHEV)
    meas_buffer[MEAS_PHEV].status = meas_buffer[MEAS_PHEV_READY_HOURS].status = STATUS_READY;
#endif
}

static void reset_CMDS_buffer(double reset_time)
{
    long int ctrl_back = 0;
    send_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&ctrl_back, sizeof (long int));

    clean_CMDS_buffer();

    /* Restart controller timer */
    reset_timer();
    global_last_time_sent = reset_time;

}

static void clean_CMDS_buffer(void)
{
    // if(0 == isdelivered_CMDS_buffer()) {
    //     ERROR("clean_CMDS_buffer: buffer has not been delivered\n");
    // }
    // DEBUG_PRINT("clean_CMDS_buffer: cleaning commands buffer\n");

    Commands i;

    control_buffer[SOCKET_CMDS].status = STATUS_EMPTY;
    for(i = 0; i < CMDS_NUMBER; ++i) {
        cmds_buffer[i].status = STATUS_EMPTY;
    }

}

/**
 * Buffer printing debug utilities
 */
static void print_MEAS_buffer(void)
{
    DEBUG_PRINT("=== MEAS_BUFFER ===\n");
    if(STATUS_EMPTY == control_buffer[SOCKET_MEAS].status) {
        DEBUG_PRINT("control: not set\n");
    }
    else {
        DEBUG_PRINT("control: %ld\n", control_buffer[SOCKET_MEAS].value);
    }
    Measures i;
    for(i = 0; i < MEAS_NUMBER; ++i) {
        if(STATUS_EMPTY == meas_buffer[i].status) {
            DEBUG_PRINT("%d: not set\n", i);
        }
        else {
            DEBUG_PRINT("%d: %f\n", i, meas_buffer[i].value);
        }
    }
    DEBUG_PRINT("===================\n\n");
}

static void print_CMDS_buffer(void)
{
    DEBUG_PRINT("=== CMDS_BUFFER ===\n");
    if(STATUS_EMPTY == control_buffer[SOCKET_CMDS].status) {
        DEBUG_PRINT("control: not set\n");
    }
    else {
        DEBUG_PRINT("control: %ld\n", control_buffer[SOCKET_CMDS].value);
    }
    Commands i;
    for(i = 0; i < CMDS_NUMBER; ++i) {
        if(STATUS_EMPTY == cmds_buffer[i].status) {
            DEBUG_PRINT("%d: not set\n", i);
        }
        else {
            DEBUG_PRINT("%d: %f\n", i, cmds_buffer[i].value);
        }
    }
    DEBUG_PRINT("===================\n\n");
}


