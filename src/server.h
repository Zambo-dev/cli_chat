#ifndef SERVER_H
#define SERVER_H

#include "include.h"
#include "sock.h"


typedef struct THREAD_DATA
{
	sock_t *sock;
	int idx;
} tdata_t;

typedef struct SERVER_T
{
	sock_t sock;
	conf_t conf;
	conn_t **conn_list;
} server_t;

int server_connect(sock_t *server);
int server_recv(tdata_t *data);
int server_send(conn_t **conns, char *buffer);

#endif
