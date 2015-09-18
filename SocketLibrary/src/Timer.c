#include <Timer.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <poll.h>

/************************************************************
* Defines and macros
************************************************************/

#define NORMAL_TIME_WAIT    60L    /* in seconds */
#define ANSWER_TIME_OUT     3600L  /* in seconds */

/************************************************************
* Local structs
************************************************************/

unsigned long data_avail_time_out = NORMAL_TIME_WAIT;
unsigned long answer_time_out = ANSWER_TIME_OUT;
struct timeval tv_init;

/************************************************************
* Function definition
************************************************************/

/**
 * Returns 0 if no data can be read from [fd_source], non-zero 
 * otherwise. Waits up to [NORMAL_TIME_WAIT] seconds for data
 * to become available.
 */
int data_available(const int fd_source)
{
//    DEBUG_PRINT("data_available: checking...\n");
    struct pollfd fd_wait = {0};
    int ret;
    fd_wait.fd = fd_source;
    fd_wait.events = POLLIN;

    if(0 > (ret = poll(&fd_wait, 1, data_avail_time_out * 1000L))) {
        ERROR("data_available: poll failure\n");
    }

    return ret;
}

/**
 * Waits for poll to be ready, for up to [tv_init] + 1 hour.
 */
int wait_for_answer(const int fd_source)
{
//    DEBUG_PRINT("wait_for_answer: waiting...\n");
    struct timeval now;
    int ret;

    gettimeofday(&now, NULL);

    unsigned long time_left;
    if(tv_init.tv_sec + answer_time_out < now.tv_sec) {
        time_left = 0;
    }
    else {
        time_left = tv_init.tv_sec + answer_time_out - now.tv_sec;
    }

    struct pollfd fd_wait = {0};
    fd_wait.fd = fd_source;
    fd_wait.events = POLLIN;

    if(0 > (ret = poll(&fd_wait, 1, time_left * 1000L))) {
        ERROR("wait_for_answer: poll failure\n");
    }

    return ret;
}

/**
 * Sets the local timeout values.
 */
void setup_timer(const unsigned long answer_timeout_sec, const unsigned long data_avail_timeout_sec)
{
    answer_time_out = answer_timeout_sec;
    data_avail_time_out = data_avail_timeout_sec;
}

/**
 * Sets the local timeval to the current time.
 */
void reset_timer(void)
{
    gettimeofday(&tv_init, NULL);
}
