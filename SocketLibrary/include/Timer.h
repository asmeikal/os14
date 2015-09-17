#ifndef __TIMER_H
#define __TIMER_H

/************************************************************
* Function declaration
************************************************************/

int data_available(int fd_source);
void wait_for_answer(int fd_source);
void set_timer(void);

#endif
