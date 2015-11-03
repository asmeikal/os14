#ifndef __LIB_SOCKETS_MODELICA_H
#define __LIB_SOCKETS_MODELICA_H

#include <stdint.h>

/************************************************************
* Function declaration
************************************************************/

void startServers(const double t, const unsigned long sec_per_step, const unsigned long sec_per_time_int);

double sendOM(const double val, const char * const name, const double t, const int32_t ctrl);

double getOM(const double o, const char * const name, const double t, const int32_t ctrl);

#endif
