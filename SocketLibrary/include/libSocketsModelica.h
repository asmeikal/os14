#ifndef __LIB_SOCKETS_MODELICA_H
#define __LIB_SOCKETS_MODELICA_H

/************************************************************
* Function declaration
************************************************************/

double startServers(double t);

double sendOM(double val, char *name, double t);
long int sendOMcontrol(long int val, double t);

double getOM(double o, char *name, double t);
long int getOMcontrol(long int o, double t);

double getOM_NB(double o, char *name, double t);
long int getOMcontrol_NB(long int o, double t);

#endif
