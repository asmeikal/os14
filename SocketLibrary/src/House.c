#include <House.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/************************************************************
* Function definition
************************************************************/

Measures get_MEAS_num_from_name(char *name)
{
    if(NULL == name) {
        ERROR("get_MEAS_num_from_name: NULL pointer argument\n");
    }

    if(0 == (strncmp(name, "energy", strlen("energy")))) {
        return MEAS_ENERGY;
    }
    else if(0 == (strncmp(name, "consumption", strlen("consumption")))) {
        return MEAS_CONSUMPTION;
    }
    else if(0 == (strncmp(name, "production", strlen("production")))) {
        return MEAS_PRODUCTION;
    }
    else if(0 == (strncmp(name, "battery", strlen("battery")))) {
        return MEAS_BATTERY;
    }
    else if(0 == (strncmp(name, "phev", strlen("phev")))) {
        return MEAS_PHEV;
    }
    else if(0 == (strncmp(name, "phev_ready_hours", strlen("phev_ready_hours")))) {
        return MEAS_PHEV_READY_HOURS;
    }
    else {
        return -1;
    }
}

Commands get_CMDS_num_from_name(char *name)
{
    if(NULL == name) {
        ERROR("get_CMDS_num_from_name: NULL pointer argument\n");
    }

    if(0 == (strncmp(name, "battery", strlen("battery")))) {
        return CMDS_BATTERY;
    }
    else if(0 == (strncmp(name, "phev", strlen("phev")))) {
        return CMDS_PHEV;
    }
    else {
        return -1;
    }
}

