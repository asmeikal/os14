#include <Timer.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <poll.h>

/************************************************************
* Local structs
************************************************************/

struct timeval tv_init;

/************************************************************
* Function definition
************************************************************/

/**
 * Waits for poll to be ready, for up to [tv_init] + 1 hour.
 */
void wait_for_answer(int fd_source)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    unsigned long time_left = tv_init.tv_sec + (60 * 60) - now.tv_sec;

    struct pollfd fd_wait = {0};
    fd_wait.fd = fd_source;
    fd_wait.events = POLLIN;

    if(0 >= poll(&fd_wait, 1, time_left * 1000L)) {
        ERROR("Stopping simulation: controller timed out\n");
    }
}

/**
 * Sets the local timeval to the current time.
 */
void set_timer(void)
{
    gettimeofday(&tv_init, NULL);
}
