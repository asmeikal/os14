#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/************************************************************
* Debug macros
************************************************************/

#define PRINT_MSG(...) \
            do { \
                fprintf(stderr, __VA_ARGS__); \
            } while(0)

#define ERROR(...) \
            do { \
                fprintf(stderr, __VA_ARGS__); \
                raise(SIGINT); \
                exit(1); \
            } while(0)

#ifndef NDEBUG
    #define DEBUG_PRINT(...) \
            do { \
                fprintf(stderr, __VA_ARGS__); \
            } while(0)
#else
    #define DEBUG_PRINT(...) \
            do {;} while(0)
#endif


#endif