#include <Sockets.h>

#include <Debug.h>
#include <House.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

/************************************************************
* Function definition
************************************************************/

/**
 * Creates a socket listening on the given port for a maximum of max_con
 * connections. The socket is a TCP reusable socket, listening on any 
 * address.
 * Return value is non-negative on success, negative on failure.
 */
int socketBuilder(unsigned short port, unsigned int max_con)
{
    int errors = 0, result = 0, yes = 1;
    struct sockaddr_in addr = {0};

    /* Create socket. */
    if (0 > (result = socket(PF_INET, SOCK_STREAM, 0))) {
        return result;
    }

    /* Make the socket port reusable. */
    if(0 > (errors = setsockopt(result, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))) {
        return errors;
    }

    /* Load up structures and bind socket. */
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(0 > (errors = bind(result, (struct sockaddr*) &addr, sizeof addr))) {
        return errors;
    }

    /* Start listening. */
    if(0 > (errors = listen(result, max_con))) {
        return errors;
    }

    return result;
}

/**
 * Builds a pollfd with [fds_left] file descriptors, taken from the
 * [sockets] not already started. Checks for read events.
 */
void buildPoll(struct pollfd *fds, int fds_left, struct socket_singleton *sockets)
{
    if(0 >= fds_left) return;

    int i;

    for(i = 0; i < fds_left; ++i, ++sockets) {
        while(0 != sockets->started) {
            ++sockets;
        }
        fds[i].events = POLLIN;
        fds[i].fd = sockets->listen_fd;
    }

}

/**
 * Accepts a connection from all ready file descriptors, and returns 
 * the number of file descriptors who must still be started.
 */
int acceptConnections(struct pollfd *fds, int fds_left, struct socket_singleton *sockets)
{
    int i, j, m, found;

    m = fds_left;

    for(i = 0; i < m; ++i) {
        if(fds[i].revents & POLLIN) {
            found = 0;
            for(j = 0; (j < SOCKET_NUMBER) && (0 == found); ++j) {
                if(sockets[j].listen_fd == fds[i].fd) {
                    found = 1;
                    sockets[j].accept_fd = accept(sockets[j].listen_fd, NULL, NULL);
                    sockets[j].started = 1;
                    close(sockets[j].listen_fd);
                    PRINT_MSG("Started socket %d.\n", j);
                }
            }
            --fds_left;
        }
    }
    return fds_left;
}
