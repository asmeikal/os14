#include <Timer.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <poll.h>

/************************************************************
* Defines and macros
************************************************************/

#define DEFAULT_TIMEOUT	 3600L  /* in seconds */

/************************************************************
* Local structs
************************************************************/

enum _timer_return_values {
	_TIMER_SUCCESS = 0,
	_TIMER_INVALID,
	_TIMER_FAILURE
};

struct _timer {
	struct timeval timestamp;
	unsigned int speed;
	unsigned int queries_per_int;
};

/************************************************************
* Local functions declaration
************************************************************/

static unsigned long long remaining_time_millis(const Timer t, const int step);
static int timed_poll(const Timer t, const long int step, const int fd_source, const short events);
static int timer_check(Timer t, const char * const fname);

/************************************************************
* Function definition
************************************************************/

Timer create_timer(const unsigned int speed, const unsigned int queries_per_int)
{
	struct _timer *ret = calloc(1, sizeof(*ret));
	if (NULL == ret) {
		return NULL;
	}
	if (0 != gettimeofday(&ret->timestamp, NULL)) {
		free(ret);
		return NULL;
	}
	ret->speed = speed;
	ret->queries_per_int = queries_per_int;
	return ret;
}

/**
 * Returns 0 if no data can be read from [fd_source], non-zero
 * otherwise. Waits up to [NORMAL_TIME_WAIT] seconds for data
 * to become available.
 */
int read_possible(const Timer t, const long int step, const int fd_source)
{
	return timed_poll(t, step, fd_source, POLLIN);
}

int write_possible(const Timer t, const long int step, const int fd_source)
{
	return timed_poll(t, step, fd_source, POLLOUT);
}

static int timed_poll(const Timer t, const long int step, const int fd_source, const short events)
{
//	DEBUG_PRINT("timed_poll: checking...\n");
	if (_TIMER_SUCCESS != timer_check(t, "timed_poll")) {
		return 0;
	}
	if((step < 0) || (step >= t->queries_per_int)) {
		DEBUG_PRINT("timed_poll: invalid step %ld\n", step);
		return 0;
	}
	struct pollfd fd_wait = {0};
	int ret;

	unsigned long long timeout = remaining_time_millis(t, step + 1);

	fd_wait.fd = fd_source;
	fd_wait.events = events;

	DEBUG_PRINT("timed_poll: waiting for %lld milliseconds.\n", timeout);
	if(0 > (ret = poll(&fd_wait, 1, timeout))) {
		DEBUG_PRINT("timed_poll: poll failure\n");
		return 0;
	}

	if (events == fd_wait.revents) {
		return ret;
	}
	else {
		return 0;
	}
}

/**
 * Waits for poll to be ready, for up to [tv_init] + 1 hour.
 */
//int wait_for_answer(const int fd_source)
//{
////	DEBUG_PRINT("wait_for_answer: waiting...\n");
//	int ret;
//	struct pollfd fd_wait = {0};
//	unsigned long time_left = remaining_time_millis();
//
//	fd_wait.fd = fd_source;
//	fd_wait.events = POLLIN;
//
//	if(0 > (ret = poll(&fd_wait, 1, time_left * 1000L))) {
//		ERROR("wait_for_answer: poll failure\n");
//	}
//
//	return ret;
//}

//int time_is_up(void)
//{
//	return remaining_time_millis() != 0;
//}

/**
 * Sets the local timeout values.
 */
//void setup_timer(const unsigned int param_simulation_speed, const unsigned int param_queries_per_int)
//{
//	DEBUG_PRINT("setup_timer: simulation speed was %d, is now %d\n", simulation_speed, param_simulation_speed);
//	simulation_speed = param_simulation_speed;
//
//	DEBUG_PRINT("setup_timer: queries per interval was %d, is now %d\n", queries_per_int, param_queries_per_int);
//	queries_per_int  = param_queries_per_int;
//}

/**
 * Sets the local timeval to the current time.
 */
int reset_timer(Timer t)
{
	return gettimeofday(&t->timestamp, NULL);
}

/**
 * Returns the time left in the current step, in milliseconds
 */
static unsigned long long remaining_time_millis(const Timer t, const int step)
{
	if (_TIMER_SUCCESS != timer_check(t, "remaining_time_millis")) {
		return 0;
	}
	if((step <= 0) || (step > t->queries_per_int)) {
		DEBUG_PRINT("remaining_time_millis: invalid step %d\n", step);
		return 0;
	}
	struct timeval now;

	if (0 != gettimeofday(&now, NULL)) {
		return 0;
	}

	unsigned long long time_left = 0,
			  max_time = t->timestamp.tv_sec * 1000ULL + (((1000ULL * step * DEFAULT_TIMEOUT) / t->queries_per_int)/ t->speed) + t->timestamp.tv_usec / 1000ULL,
			  now_time = now.tv_sec * 1000ULL + now.tv_usec / 1000ULL;
	if(max_time > now_time) {
		time_left = max_time - now_time;
	}

	return time_left;
}

static int timer_check(Timer t, const char * const fname)
{
	if (NULL == t) {
		DEBUG_PRINT("%s: NULL pointer argument.\n", fname);
		return -_TIMER_INVALID;
	}
	return _TIMER_SUCCESS;
}

