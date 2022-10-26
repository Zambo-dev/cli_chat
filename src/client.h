#ifndef CLIENT_H
#define CLIENT_H

#include "include.h"
#include "sock.h"


int client_connect(sock_t *client);
int client_recv(sock_t *client);
int client_send(sock_t *client);

#endif
