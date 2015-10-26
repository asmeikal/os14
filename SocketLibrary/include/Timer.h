#ifndef __TIMER_H
#define __TIMER_H

/************************************************************
* Function declaration
************************************************************/

typedef struct _timer * Timer;

Timer create_timer(const unsigned int speed, const unsigned int queries_per_int);
int read_possible(const Timer t, const long int step, const int fd_source);
int write_possible(const Timer t, const long int step, const int fd_source);
int reset_timer(Timer t);

#endif
