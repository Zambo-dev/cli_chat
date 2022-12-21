#ifndef CLIENT_H
#define CLIENT_H

#include "include.h"
#include "sock.h"


typedef struct CLIENT_T
{
	sock_t sock;
	conf_t conf;
	conn_t **server_list;
} client_t;

int client_connect(sock_t *client);
int client_recv(sock_t *client);
int client_send(sock_t *client);

#endif
