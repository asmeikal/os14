#ifndef __BUFFER_H
#define __BUFFER_H

typedef struct buffer_general *GBuffer;

typedef const char * const (*PrintFunction)(const int);

/************************************************************
* Function declaration
************************************************************/

/* Initializers */
GBuffer GB_initDouble(const int size);
GBuffer GB_initLongInt(const int size);

/* Getters */
double GB_getDoubleValue(GBuffer b, const int i);
long int GB_getLongIntValue(GBuffer b, const int i);

/* Setters */
void GB_setDoubleValue(GBuffer b, const int i, const double v);
void GB_setLongIntValue(GBuffer b, const int i, const long int v);

/* Status setters */
void GB_set(GBuffer b, const int i);
void GB_unset(GBuffer b, const int i);
void GB_markAsDelivered(GBuffer b, const int i);

/* Buffer empty utility */
void GB_empty(GBuffer b);

/* Single index checker */
int GB_isSet(GBuffer b, const int i);
int GB_isDelivered(GBuffer b, const int i);

/* Full buffer checkers */
int GB_isFull(GBuffer b);
int GB_isEmpty(GBuffer b);
int GB_isAllDelivered(GBuffer b);

/* Printers */
void GB_print(GBuffer b);
void GB_printDecorated(GBuffer b, PrintFunction f);

#endif
