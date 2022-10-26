#ifndef SOCK_H
#define SOCK_H

#include "include.h"


typedef struct CONN_T
{
	SSL *c_ssl;
	SSL_CTX *c_sslctx;
	int c_fd;
	char c_ip[32];
} conn_t;

typedef struct SOCK_T
{
	conn_t s_conn;
	conn_t **s_conn_list;
	struct sockaddr_in s_host;
} sock_t;

int sock_init(sock_t *sock, char *ip, char *port, char *cert, char *key);
int sock_close(sock_t *sock);

#endif
