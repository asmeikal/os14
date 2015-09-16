#ifndef __SOCKETS_H
#define __SOCKETS_H

#include <stdlib.h>
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

void recv_complete(int fd, char *buf, size_t count);
void send_complete(int fd, char *buf, size_t count);


#endif
