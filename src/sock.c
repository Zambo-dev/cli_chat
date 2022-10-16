#include "include.h"


int errck()
{
	/* Lock errno mutex */
	pthread_mutex_lock(&errno_mtx);
	
	if(errno != 0)
	{	
		/* Print errno's value and string */
		printf("ERROR %d: %s\n", errno, strerror(errno));
		fflush(stdout);
		/* Reset errno */
		errno = 0;
		/* Unock errno mutex */
		pthread_mutex_unlock(&errno_mtx);
		return -1;
	}

	/* Unock errno mutex */
	pthread_mutex_unlock(&errno_mtx);
	return 0;
}

int sock_init(sock_t *sock, char* ip, char *port)
{	
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	/* Create socket */
	sock->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(errck() == -1)
	{
		/* Unock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	/* Setup host data */
	sock->host.sin_family = AF_INET;
	sock->host.sin_port = htons(strtol(port, NULL, 10));
	sock->host.sin_addr.s_addr = (ip == NULL) ? INADDR_ANY : inet_addr(ip);
	if(errck() == -1)
	{
		/* Unock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	/* Set conns only for server */
	sock->conns = (ip == NULL) ? (conn_t **)calloc(CONNLIMIT, sizeof(conn_t)) : NULL;
	if(errck() == -1)
	{
		/* Unock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	/* Print created socket */
	printf("Socked %d created!\n", sock->fd);
	fflush(stdout);

	/* Unock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_close(sock_t *sock)
{	
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	int fd = sock->fd;
	/* Close scoket fd */
	close(sock->fd);
	if(errck() == -1)
	{
		/* Unock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	sock->fd = 0;
	/* Clear sockaddr_in struct */
	memset(&sock->host, 0, sizeof(struct sockaddr_in));
	/* Close all connection (only server) */
	if(sock->conns != NULL)
	{
		for(size_t i=0; i<CONNLIMIT; ++i)
		{
			if(sock->conns[i] != NULL && server_conns_close(sock, i) == -1)
			{
				/* Unock scoket mutex */
				pthread_mutex_unlock(&sock_mtx);
				return -1;
			}
		}
		free(sock->conns);
	}
	/* Print closed */
	printf("Socked %d closed!\n", fd);
	fflush(stdout);

	/* Unock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}
