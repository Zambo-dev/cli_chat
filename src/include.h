#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

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
	int fd;
	char ip[32];
} conn_t;

typedef struct SOCK_T
{
	int fd;
	conn_t **conns;
	struct sockaddr_in host;
} sock_t;

typedef struct THREAD_DATA
{
	sock_t *sock;
	int idx;
} tdata_t;


/* General functions */
int errck();
int sock_init(sock_t *sock, char *ip, char *port);
int sock_close(sock_t *sock);

/* Client functions */
int client_connect(sock_t *client);
void client_recv(sock_t *client);
void client_send(sock_t *client);

/* Server functions */
int server_conns_init(conn_t **conn, int fd, char *ip);
int server_conns_close(conn_t **conn);
int server_conns_getfree(conn_t **conns);
int server_connect(sock_t *server);
void server_recv(tdata_t *data);
void server_send(sock_t *server, int idx, char *buffer);

#endif
