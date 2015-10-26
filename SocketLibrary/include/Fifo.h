#ifndef __FIFO_H
#define __FIFO_H

typedef struct _fifo *FIFO;

FIFO fifo_init(void);
void fifo_destroy(FIFO f);
int fifo_insert(FIFO f, void *item);
void *fifo_peek(FIFO f);
void *fifo_pop(FIFO f);
void fifo_print(FIFO f, void (*printFunction)(void *));

#endif

