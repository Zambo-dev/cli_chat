#ifndef CLIENT_H
#define CLIENT_H

#include <sys/select.h>
#include "sock.h"


typedef struct CLIENT_T
{
	fd_set sigfd;
	sock_t *sock;
} client_t;


#endif
