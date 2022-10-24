#ifndef SOCK_H
#define SOCK_H

#include "include.h"


int sock_init(sock_t *sock, char *ip, char *port, char *cert, char *key);
int sock_close(sock_t *sock);

#endif
