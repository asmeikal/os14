#ifndef __LIB_SOCKETS_MODELICA_H
#define __LIB_SOCKETS_MODELICA_H

/************************************************************
* Function declaration
************************************************************/

void startServers(const double t, const double comms_time_int, const long int sec_per_step, const long int sec_per_time_int);

double sendOM(const double val, const char * const name, const double t);

double getOM(const double o, const char * const name, const double t);

#endif
