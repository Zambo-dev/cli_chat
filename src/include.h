#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#define BUFFERLEN 512
#define CONNLIMIT 8


extern pthread_mutex_t fd_mtx;
extern pthread_mutex_t sock_mtx;
extern pthread_mutex_t errno_mtx;
extern pthread_mutex_t tdata_mtx;
extern pthread_mutex_t run_mtx;
extern pthread_t pool[CONNLIMIT];
extern int running;
extern int cli_row;

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

typedef struct THREAD_DATA
{
	sock_t *sock;
	int idx;
} tdata_t;


/* General functions */
void errck(char *func_name);
int sock_init(sock_t *sock, char *ip, char *port);
int sock_close(sock_t *sock);

/* Client functions */
int client_connect(sock_t *client);
int client_recv(sock_t *client);
int client_send(sock_t *client);

/* Server functions */
int server_conns_init(conn_t **conn, SSL_CTX *server_ctx, int fd, char *ip);
int server_conns_close(conn_t **conn, int idx);
int server_conns_getfree(conn_t **conns);
int server_connect(sock_t *server);
int server_recv(tdata_t *data);
int server_send(sock_t *server, char *buffer);

#endif
