#include "conn.h"
#include "err.h"


/* 
* conns is an already allocated array of conn_t *, conns[idx] will be allocated
* and initialized with parameters data
*/
int conn_init(conn_t **conns, int idx, char type, SSL_CTX *sslctx, int fd, int port, char *ip, char *username)
{
	pthread_mutex_lock(&fd_mtx);

	conn_t *conn = conns[idx];
	int retval;

	conn = (conn_t *)calloc(1, sizeof(conn_t));
	
	conn->fd = fd;
	conn->port = port;
	conn->type = type;

	if( ip != NULL) strncpy(conn->ip, ip, 32);
	if(sslctx != NULL) conn->sslctx = sslctx;
	if(username != NULL) strncpy(conn->usrnm, username, 32);

    pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int conn_close(conn_t **conns, int idx)
{
	pthread_mutex_lock(&fd_mtx);

	conn_t *conn = conns[idx];
    int fd = conn->fd;

    SSL_free(conn->ssl);
    if(close(conn->fd) == -1)
	{
		/* errno check */
		fd_errck("close");
		/* Unlock scoket mutex */
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}

    free(conn);
    conns[idx] = NULL;

	pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int conn_getfree(conn_t **conns, int size)
{
	pthread_mutex_lock(&fd_mtx);
	
	for(size_t i=0; i<size; ++i)
	{
		if(conns[i] == NULL)
		{
			pthread_mutex_unlock(&fd_mtx);
			return i;
		}
	}

	pthread_mutex_unlock(&fd_mtx);
	return -1;
}


