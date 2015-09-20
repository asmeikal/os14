#ifndef __TIMER_H
#define __TIMER_H

/************************************************************
* Function declaration
************************************************************/

int data_available(const int fd_source);
int wait_for_answer(const int fd_source);
void reset_timer(void);
void setup_timer(const unsigned int param_simulation_speed, const unsigned int param_queries_per_int);

#endif
