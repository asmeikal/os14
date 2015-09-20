#include <Timer.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <poll.h>

/************************************************************
* Defines and macros
************************************************************/

#define ANSWER_TIME_OUT     3600L  /* in seconds */

/************************************************************
* Local structs
************************************************************/

unsigned int simulation_speed = 1;
unsigned int queries_per_int  = 60;

struct timeval tv_init;

/************************************************************
* Local functions declaration
************************************************************/

static unsigned long remaining_time_millis(void);

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

    unsigned long timeout = (ANSWER_TIME_OUT / queries_per_int) / simulation_speed, time_left = remaining_time_millis();

    if(time_left < timeout) {
        timeout = time_left;
    }

    fd_wait.fd = fd_source;
    fd_wait.events = POLLIN;

    if(0 > (ret = poll(&fd_wait, 1, timeout * 1000L))) {
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
    int ret;
    struct pollfd fd_wait = {0};
    unsigned long time_left = remaining_time_millis();

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
void setup_timer(const unsigned int param_simulation_speed, const unsigned int param_queries_per_int)
{
    DEBUG_PRINT("setup_timer: simulation speed was %d, is now %d\n", simulation_speed, param_simulation_speed);
    simulation_speed = param_simulation_speed;

    DEBUG_PRINT("setup_timer: queries per interval was %d, is now %d\n", queries_per_int, param_queries_per_int);
    queries_per_int  = param_queries_per_int;
}

/**
 * Sets the local timeval to the current time.
 */
void reset_timer(void)
{
    gettimeofday(&tv_init, NULL);
}

static unsigned long remaining_time_millis(void)
{
    struct timeval now;
    unsigned long time_left;

    gettimeofday(&now, NULL);

    if(tv_init.tv_sec + (ANSWER_TIME_OUT / simulation_speed) < now.tv_sec) {
        time_left = 0;
    }
    else {
        time_left = tv_init.tv_sec + (ANSWER_TIME_OUT / simulation_speed) - now.tv_sec;
    }

    return time_left;
}
