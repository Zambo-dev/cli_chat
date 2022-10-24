#ifndef SERVER_H
#define SERVER_H

#include "include.h"


typedef struct THREAD_DATA
{
	sock_t *sock;
	int idx;
} tdata_t;

int server_conns_init(conn_t **conn, SSL_CTX *server_ctx, int fd, char *ip);
int server_conns_close(conn_t **conn, int idx);
int server_conns_getfree(conn_t **conns);
int server_connect(sock_t *server);
int server_recv(tdata_t *data);
int server_send(conn_t **conns, char *buffer);

#endif
