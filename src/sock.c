#include "include.h"

extern pthread_mutex_t fd_mtx;
extern pthread_mutex_t sock_mtx;
extern pthread_mutex_t errno_mtx;
extern pthread_mutex_t tdata_mtx;
extern pthread_t pool[CONNLIMIT];
extern int running;

int errck()
{
	pthread_mutex_lock(&errno_mtx);
	if(errno != 0)
	{
		printf("ERROR %d: %s\n", errno, strerror(errno));
		errno = 0;
		pthread_mutex_unlock(&errno_mtx);
		return -1;
	}
	pthread_mutex_unlock(&errno_mtx);

	return 0;
}

int sock_init(sock_t *sock, char* ip, char *port)
{	
	/* Create socket */
	sock->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(errck() != 0) return -1;
	printf("Socked created! id: %d\n", sock->fd);

	/* Setup host data */
	sock->host.sin_family = AF_INET;
	sock->host.sin_port = htons(strtol(port, NULL, 10));
	sock->host.sin_addr.s_addr = (ip == NULL) ? INADDR_ANY : inet_addr(ip);
	errck();

	/* Set conns only for server */
	sock->conns = (ip == NULL) ? (conn_t **)calloc(CONNLIMIT, sizeof(conn_t)) : NULL;

	return 0;
}

int sock_close(sock_t *sock)
{	
	pthread_mutex_lock(&sock_mtx);

	close(sock->fd);
	sock->fd = 0;

	memset(&sock->host, 0, sizeof(struct sockaddr_in));
	if(sock->conns != NULL)
	{
		for(size_t i=0; i<CONNLIMIT; ++i)
			if(sock->conns[i] != NULL)
				server_conns_close(&sock->conns[i]);
		free(sock->conns);
	}

	pthread_mutex_unlock(&sock_mtx);
	puts("Socket closed!");


	return 0;
}
