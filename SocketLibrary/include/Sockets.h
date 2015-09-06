#ifndef __SOCKETS_H
#define __SOCKETS_H

#include <poll.h>

/************************************************************
* Socket struct
************************************************************/

struct socket_singleton {
    int listen_fd;
    int accept_fd;
    int started;
};

/************************************************************
* Function declaration
************************************************************/

int socketBuilder(unsigned short port, unsigned int max_con);
void buildPoll(struct pollfd *fds, int fds_left, struct socket_singleton *sockets);
int acceptConnections(struct pollfd *fds, int fds_left, struct socket_singleton *sockets);

#endif