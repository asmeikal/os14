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

int socketBuilder(const unsigned short port, const unsigned int max_con);
void buildPoll(struct pollfd *fds, const int fds_left, struct socket_singleton *sockets);
int acceptConnections(const struct pollfd *fds, int fds_left, struct socket_singleton *sockets);

void recv_complete(struct socket_singleton *socket, char *buf, const size_t count);
void send_complete(struct socket_singleton *socket, const char * const buf, const size_t count);


#endif
