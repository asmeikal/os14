#include <Debug.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

/**
 * Redirects stderr to a file named [fname]_[current time].log
 */
void OPEN_DEBUG(const char * const fname)
{
    if(NULL == fname) {
        ERROR("OPEN_DEBUG: NULL pointer argument.\n");
    }

    char *full_fname = (char *) malloc(strlen(fname) + 32);
    if(NULL == full_fname) {
        ERROR("OPEN_DEBUG: failed malloc.\n");
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    sprintf(full_fname, "%s_%ld.log", fname, tv.tv_sec);

    FILE *log = fopen(full_fname, "w");
    if(NULL == log) {
        ERROR("OPEN_DEBUG: unable to create log file.\n");
    }

    if(0 > dup2(fileno(log), STDERR_FILENO)) {
        ERROR("OPEN_DEBUG: unable to redirect stderr.\n");
    }
}

