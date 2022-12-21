#ifndef CONN_H
#define CONN_H

#include "include.h"

typedef struct CONN_T
{
	SSL *ssl;
	SSL_CTX *sslctx; 
	int fd;
	int port;
	char type;			// s: server, c: client
	char ip[32];
	char usrnm[32];
} conn_t;

int conn_init(conn_t **conns, int idx, char type, SSL_CTX *sslctx, int fd, int port, char *ip, char *username);
int conn_close(conn_t **conns, int idx);
int conn_getfree(conn_t **conns, int size);


#endif
