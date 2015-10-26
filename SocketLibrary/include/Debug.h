#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/************************************************************
* Debug macros
************************************************************/

void OPEN_DEBUG(const char * const fname);

#define ERROR(...) \
            do { \
		fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
                fprintf(stderr, "ERROR: "); \
                WARNING(__VA_ARGS__); \
                raise(SIGINT); \
                exit(1); \
            } while(0)

#define WARNING(...) \
            do { \
                fprintf(stderr, __VA_ARGS__); \
            } while(0)

#ifndef NDEBUG
    #define DEBUG_PRINT(...) \
            do { \
                WARNING(__VA_ARGS__); \
            } while(0)
#else
    #define DEBUG_PRINT(...) \
            do {;} while(0)
#endif


#endif

