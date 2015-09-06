#ifndef __LIB_SOCKETS_MODELICA_H
#define __LIB_SOCKETS_MODELICA_H

/************************************************************
* Function declaration
************************************************************/

void startServers(void);

void sendOM(double val, char *name);
void sendOMcontrol(long int val);

double getOM(char *name);
long int getOMcontrol(void);

#endif
