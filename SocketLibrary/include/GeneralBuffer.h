#ifndef __BUFFER_H
#define __BUFFER_H

typedef struct _GB_head *GBuffer;

typedef const char * const (*PrintFunction)(const int);

/************************************************************
* Function declaration
************************************************************/

/* Initializers */
GBuffer GB_initDouble(const int size);
GBuffer GB_initLongInt(const int size);
int GB_destroy(GBuffer b);

/* Getter */
int GB_getValue(GBuffer b, const int i, const void *res);

/* Setter */
int GB_setValue(GBuffer b, const int i, const void * const val);

/* Status setters */
int GB_set(GBuffer b, const int i);
int GB_unset(GBuffer b, const int i);
int GB_markAsDelivered(GBuffer b, const int i);

/* Buffer empty utility */
int GB_empty(GBuffer b);

/* Single index checker */
int GB_isSet(GBuffer b, const int i);
int GB_isDelivered(GBuffer b, const int i);

/* Full buffer checkers */
int GB_isFull(GBuffer b);
int GB_isEmpty(GBuffer b);
int GB_isAllDelivered(GBuffer b);

/* Printer */
int GB_print(GBuffer b, PrintFunction f);

#endif
