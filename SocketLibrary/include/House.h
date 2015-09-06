#ifndef __HOUSE_H
#define __HOUSE_H

/************************************************************
* Defines for House comms
************************************************************/

#define MEAS_LISTEN_PORT    2324
#define CMDS_LISTEN_PORT    2325
#define CONTROL_STOP        -1

typedef enum socks {
    SOCKET_MEAS = 0,
    SOCKET_CMDS,
    SOCKET_NUMBER
} Sockets;

typedef enum meas {
    MEAS_ENERGY = 0,
    MEAS_CONSUMPTION,
    MEAS_PRODUCTION,
    MEAS_BATTERY,
    MEAS_PHEV,
    MEAS_PHEV_READY_HOURS,
    MEAS_NUMBER
} Measures;

typedef enum commands {
    CMDS_BATTERY = 0,
    CMDS_PHEV,
    CMDS_NUMBER
} Commands;

/************************************************************
* Function declaration
************************************************************/

Commands get_CMDS_num_from_name(char *name);
Measures get_MEAS_num_from_name(char *name);


#endif
