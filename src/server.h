#ifndef SERVER_H
#define SERVER_H

#include <sys/select.h>
#include "sock.h"


typedef struct SERVER_T
{
	sock_t *sock;
	sock_t *list[CONNLIMIT];
} server_t;

int server_accept(server_t *server);
int server_read(server_t *server);
int server_write(server_t *server, char *buffer, size_t bytes);

#endif
