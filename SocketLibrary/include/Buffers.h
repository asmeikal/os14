#ifndef __BUFFER_H
#define __BUFFER_H

typedef struct buffer_general *GBuffer;

/************************************************************
* Function declaration
************************************************************/

/* Initializers */
GBuffer GB_initDouble(int size);
GBuffer GB_initLongInt(int size);

/* Getters */
double GB_getDoubleValue(GBuffer b, int i);
long int GB_getLongIntValue(GBuffer b, int i);

/* Setters */
void GB_setDoubleValue(GBuffer b, int i, double v);
void GB_setLongIntValue(GBuffer b, int i, long int v);

/* Status setters */
void GB_set(GBuffer b, int i);
void GB_unset(GBuffer b, int i);
void GB_markAsDelivered(GBuffer b, int i);

/* Buffer empty utility */
void GB_empty(GBuffer b);

/* Single index checker */
int GB_isSet(GBuffer b, int i);
int GB_isDelivered(GBuffer b, int i);

/* Full buffer checkers */
int GB_isFull(GBuffer b);
int GB_isEmpty(GBuffer b);
int GB_isAllDelivered(GBuffer b);

/* Printers */
void GB_print(GBuffer b);

#endif
