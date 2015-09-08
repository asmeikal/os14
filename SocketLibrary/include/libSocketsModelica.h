#ifndef __LIB_SOCKETS_MODELICA_H
#define __LIB_SOCKETS_MODELICA_H

/************************************************************
* Function declaration
************************************************************/

double startServers(double t);

double sendOM(double val, char *name, double t);
long int sendOMcontrol(long int val, double t);

double getOM(char *name, double t);
long int getOMcontrol(double t);

#endif
