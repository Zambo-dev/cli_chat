#ifndef SOCK_H
#define SOCK_H

#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include "err.h"
#include "conf.h"
#include "conf.h"

#define BUFFERLEN 256
#define CONNLIMIT 16


typedef struct SOCK_T
{
	int fd;
	SSL *ssl; 	
	SSL_CTX *sslctx; 
	conf_t conf;
	struct sockaddr_in host;
} sock_t;

int sock_init(sock_t *sock);
int sock_connect(sock_t *sock);
int sock_listen(sock_t *sock);
int sock_accept(sock_t *sock, sock_t *conn);
int sock_write(sock_t *sock, char *buffer, size_t *size);
int sock_read(sock_t *sock, char **buffer, size_t *size);
int sock_close(sock_t *sock);

#endif
