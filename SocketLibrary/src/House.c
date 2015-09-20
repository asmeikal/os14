#include <House.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/************************************************************
* Function definition
************************************************************/

Measures get_MEAS_num_from_name(const char * const name)
{
    if(NULL == name) {
        ERROR("get_MEAS_num_from_name: NULL pointer argument\n");
    }

    if((strlen(name) == strlen("energy")) && 
       (0 == (strncmp(name, "energy", strlen("energy"))))) {
        return MEAS_ENERGY;
    }
    else if((strlen(name) == strlen("consumption")) && 
       (0 == (strncmp(name, "consumption", strlen("consumption"))))) {
        return MEAS_CONSUMPTION;
    }
    else if((strlen(name) == strlen("production")) && 
       (0 == (strncmp(name, "production", strlen("production"))))) {
        return MEAS_PRODUCTION;
    }
    else if((strlen(name) == strlen("battery")) && 
       (0 == (strncmp(name, "battery", strlen("battery"))))) {
        return MEAS_BATTERY;
    }
    else if((strlen(name) == strlen("phev")) && 
       (0 == (strncmp(name, "phev", strlen("phev"))))) {
        return MEAS_PHEV;
    }
    else if((strlen(name) == strlen("phev_ready_hours")) && 
       (0 == (strncmp(name, "phev_ready_hours", strlen("phev_ready_hours"))))) {
        return MEAS_PHEV_READY_HOURS;
    }
    else {
        return -1;
    }
}

Commands get_CMDS_num_from_name(const char * const name)
{
    if(NULL == name) {
        ERROR("get_CMDS_num_from_name: NULL pointer argument\n");
    }

    if((strlen(name) == strlen("battery")) && 
       (0 == (strncmp(name, "battery", strlen("battery"))))) {
        return CMDS_BATTERY;
    }
    else if((strlen(name) == strlen("phev")) && 
       (0 == (strncmp(name, "phev", strlen("phev"))))) {
        return CMDS_PHEV;
    }
    else {
        return -1;
    }
}


const char * const get_MEAS_name_from_num(const Measures c)
{
    switch (c) {
        case MEAS_ENERGY:
            return "energy load on grid";
        case MEAS_CONSUMPTION:
            return "energy consumption";
        case MEAS_PRODUCTION:
            return "energy production";
        case MEAS_BATTERY:
            return "battery charge";
        case MEAS_PHEV:
            return "PHEV charge";
        case MEAS_PHEV_READY_HOURS:
            return "PHEV ready hours";
        default:
            ERROR("get_MEAS_name_from_num: measurement type not recognized\n");
    }
}

const char * const get_CMDS_name_from_num(const Commands c)
{
    switch (c) {
        case CMDS_BATTERY:
            return "battery charge rate";
        case CMDS_PHEV:
            return "PHEV charge rate";
        default:
            ERROR("get_MEAS_name_from_num: measurement type not recognized\n");
    }
}

