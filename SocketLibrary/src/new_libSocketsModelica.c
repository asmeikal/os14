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
* Local functions declaration
************************************************************/

static void send_all_vars(void);
// static void get_all(void);
static void recv_next_var(void);

static void reset_MEAS_buffer(void);
static void reset_CMDS_buffer(double reset_time);

/************************************************************
* Local variables
************************************************************/

GBuffer control_buffer;
GBuffer meas_buffer;
GBuffer cmds_buffer;

struct socket_singleton sockets[SOCKET_NUMBER] = {{0}};

double last_meas_time;
double last_cmds_time;

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

    /* Initialize buffers */
    control_buffer = GB_initLongInt(SOCKET_NUMBER);
    meas_buffer = GB_initDouble(MEAS_NUMBER);
    cmds_buffer = GB_initDouble(CMDS_NUMBER);

    reset_MEAS_buffer();
    reset_CMDS_buffer(0.0);

    return t;
}

/**
 * Buffers the control message [val]. Sends all values if MEAS buffer
 * is full.
 * Throws error if control message has already been buffered.
 */
long int sendOMcontrol(long int val, double t)
{
    if(GB_isSet(control_buffer, SOCKET_MEAS)) {
        /* We already have a buffered value... something went wrong */
        ERROR("sendOMcontrol: control already set\n");
    }

    GB_setLongIntValue(control_buffer, SOCKET_MEAS, val);

    if(GB_isSet(control_buffer, SOCKET_MEAS) && 
       GB_isFull(meas_buffer)) 
    {
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

    if(GB_isSet(meas_buffer, n)) {
        ERROR("sendOM: '%s' already set\n", name);
    }

    GB_setDoubleValue(meas_buffer, n, val);

    if(isfull_MEAS_buffer()) 
    {
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
    // DEBUG_PRINT("getOMcontrol_NB: called with (o: %ld, t: %f)\n", o, t);

    long int ret;

    if(t >= (last_cmds_time + 60.0)) 
    {
        /* One hour has passed, get all commands */

        /* If CMDS buffers are not full... */
        while(!isfull_CMDS_buffer()) 
        {
            /* ... wait and receive */
            wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
            recv_next_var();
        }

        /* Set current var as delivered */
        GB_markAsDelivered(control_buffer, SOCKET_CMDS);
        ret = GB_getLongIntValue(control_buffer, SOCKET_CMDS);

        /* If all have been delivered, reset buffer */
        if(isdelivered_CMDS_buffer()) 
        {
            reset_CMDS_buffer(t);
        }
    }
    else {
        /* We can still wait */

        /* Receive only if data available and buffer not full */
        while(data_available(sockets[SOCKET_CMDS].accept_fd) && 
              !isfull_CMDS_buffer()) 
        {
            recv_next_var();
        }


        /* If we already have new data, deliver it. 
         * Otherwise, deliver the old value */
        if(GB_isSet(control_buffer, SOCKET_CMDS)) {
            ret = GB_getLongIntValue(control_buffer, SOCKET_CMDS);
        }
        else {
            ret = o;
        }

    }

    return ret;
}

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

    if(t >= (last_cmds_time + 60.0)) 
    {
        /* One hour has passed, get all commands */

        /* If CMDS buffers are not full... */
        while(!isfull_CMDS_buffer()) 
        {
            /* ... wait and receive */
            wait_for_answer(sockets[SOCKET_CMDS].accept_fd);
            recv_next_var();
        }

        /* Set current var as delivered */
        GB_markAsDelivered(cmds_buffer, n);
        ret = GB_getDoubleValue(cmds_buffer, n);

        /* If all have been delivered, reset buffer */
        if(isdelivered_CMDS_buffer()) 
        {
            reset_CMDS_buffer(t);
        }
    }
    else {
        /* We can still wait */

        /* Receive only if data available and buffer not full */
        while(data_available(sockets[SOCKET_CMDS].accept_fd) && 
              !isfull_CMDS_buffer()) 
        {
            recv_next_var();
        }


        /* If we already have new data, deliver it. 
         * Otherwise, deliver the old value */
        if(GB_isSet(cmds_buffer, n)) {
            ret = GB_getDoubleValue(cmds_buffer, n);
        }
        else {
            ret = o;
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
    static long int count = 0;
    ++count;
    if(count != GB_getLongIntValue(control_buffer, SOCKET_MEAS)) {
        ERROR("ERROR: send_all_vars: called %ld times, but control is %ld\n", count, GB_getLongIntValue(control_buffer, SOCKET_MEAS));
    }

    // DEBUG_PRINT("send_all_vars: sending\n");
    // DEBUG_PRINT("control: %d\n", control_buffer[SOCKET_MEAS].value);
    // print_MEAS_buffer();

    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
        ERROR("send_all_vars: sockets not started\n");
    }

    if(!isfull_MEAS_buffer()) {
        ERROR("send_all_vars: MEAS buffer not full\n");
    }

    int n;
    Measures i;
    long int message_in, control_out;
    double meas_out;

    /* Receive control message from server */
    recv_complete(sockets[SOCKET_MEAS].accept_fd, (char*)&message_in, sizeof (long int));
    DEBUG_PRINT("send_all_vars: control message from server is %ld\n", message_in);

    /* Send back control message */
    control_out = GB_getLongIntValue(control_buffer, SOCKET_MEAS);
    send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&control_out, sizeof (long int));
    DEBUG_PRINT("send_all_vars: sent control message %ld\n", control_out);

    /* Send all values in the MEAS buffer */
    for (i = 0; i < MEAS_NUMBER; i++) {
        meas_out = GB_getDoubleValue(meas_buffer, i);
        send_complete(sockets[SOCKET_MEAS].accept_fd, (char *)&meas_out, sizeof(double));
    }

    /* Empty the MEAS buffer and control variable */
    reset_MEAS_buffer();

}

static void recv_next_var(void)
{
    if((0 == sockets[SOCKET_CMDS].accept_fd) || 
       (0 == sockets[SOCKET_MEAS].accept_fd)) 
    {
        ERROR("recv_next_var: sockets not started\n");
    }

    if(isfull_CMDS_buffer()) {
        ERROR("recv_next_var: CMDS buffer is full\n");
    }

    // DEBUG_PRINT("recv_next_var: receiving\n");
    if(!GB_isSet(control_buffer, SOCKET_CMDS)) {
        /* Control is missing, get it */
        long int control_in;
        recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&control_in, sizeof(long int));
        GB_setLongIntValue(control_buffer, SOCKET_CMDS, control_in)
        return;
    }
    else {
        /* Find the missing command and get it */
        Commands i;
        double cmds_in;

        for(i = 0; i < CMDS_NUMBER; ++i) {
            if(!GB_isSet(cmds_buffer, i)) {
                recv_complete(sockets[SOCKET_CMDS].accept_fd, (char *)&cmds_in, sizeof(double));
                GB_setDoubleValue(cmds_buffer, i, cmds_in);

#if defined (NO_PHEV)
                if(i == CMDS_PHEV) {
                    GB_markAsDelivered(cmds_buffer, CMDS_PHEV);
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

static void reset_MEAS_buffer(void)
{
    GB_unset(control_buffer, SOCKET_MEAS);
    GB_empty(meas_buffer);

#if defined (NO_PHEV)
    GB_setDoubleValue(meas_buffer, MEAS_PHEV, 0.0);
    GB_setDoubleValue(meas_buffer, MEAS_PHEV_READY_HOURS, 0.0);
#endif

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
}

static void reset_CMDS_buffer(double reset_time)
{
    GB_unset(control_buffer, SOCKET_CMDS);
    GB_empty(cmds_buffer);

    /* Restart controller timer */
    reset_timer();
    last_cmds_time = reset_time;

}

static int isfull_MEAS_buffer(void)
{
    return GB_isSet(control_buffer, SOCKET_MEAS) && GB_isFull(meas_buffer);
}

static int isfull_CMDS_buffer(void)
{
    return GB_isSet(control_buffer, SOCKET_CMDS) && GB_isFull(cmds_buffer);
}

static int isdelivered_CMDS_buffer(void)
{
    return GB_isDelivered(control_buffer, SOCKET_CMDS) && GB_isAlldelivered(cmds_buffer);
}

