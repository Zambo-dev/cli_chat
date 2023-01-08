#ifndef CLIENT_H
#define CLIENT_H

#include "sock.h"


typedef struct CLIENT_T
{
	sock_t *sock;
	//TO DO: sock_t * server_history;
} client_t;

int client_connect(client_t *client);
int client_read(client_t *client);
int client_write(client_t *client);

#endif
