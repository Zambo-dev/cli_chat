#ifndef SOCK_H
#define SOCK_H

#include "include.h"
#include "conn.h"

typedef struct SOCK_T
{
	conn_t conn;
	struct sockaddr_in host;
} sock_t;

int sock_init(sock_t *sock, char *cert, char *key);
int sock_close(sock_t *sock);

#endif
