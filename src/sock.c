#include "include.h"


void fd_errck(char *func_name)
{
	/* Lock errno mutex */
	pthread_mutex_lock(&errno_mtx);
	
	if(errno != 0)
	{	
		/* Print errno's value and string */
		printf("%s -> ERROR %d: %s\n", func_name, errno, strerror(errno));
		/* Reset errno */
		errno = 0;
	}

	fflush(stdout);

	/* Unlock errno mutex */
	pthread_mutex_unlock(&errno_mtx);
}

void ssl_errck(char *func_name, int retval)
{
	char err[BUFFERLEN];
	snprintf(err, BUFFERLEN, "%s -> %s", func_name, ERR_error_string(retval, NULL));
	printf("%s\n", err);
	fflush(stdout);
}

int sock_init(sock_t *sock, char* ip, char *port)
{	
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	/* Init socket */
	if((sock->s_conn.c_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
		fd_errck("socket");
        /* Unlock scoket mutex */
        pthread_mutex_unlock(&sock_mtx);
        return -1;
    }
    strcpy(sock->s_conn.c_ip, "127.0.0.1");

	/* Setup s_host data */
	sock->s_host.sin_family = AF_INET;
	sock->s_host.sin_port = htons(strtol(port, NULL, 10));
	sock->s_host.sin_addr.s_addr = (ip == NULL) ? INADDR_ANY : inet_addr(ip);

	/* Set s_conn_list only for server */
	sock->s_conn_list = (ip == NULL) ? (conn_t **)calloc(CONNLIMIT, sizeof(conn_t)) : NULL;

	/* Print created socket */
	printf("Socked created! Id: %d Ip: %s\n", sock->s_conn.c_fd, sock->s_conn.c_ip);
	fflush(stdout);

	/* Unlock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_close(sock_t *sock)
{	
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	int fd = sock->s_conn.c_fd;

	/* Close scoket s_fd */
	if(close(sock->s_conn.c_fd) == -1)
	{
		fd_errck("close");
		/* Unlock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	sock->s_conn.c_fd = 0;
	/* Clear sockaddr_in struct */
	memset(&sock->s_host, 0, sizeof(struct sockaddr_in));
	/* Close all connection (only server) */
	if(sock->s_conn_list != NULL)
	{
		for(size_t i=0; i<CONNLIMIT; ++i)
		{
			if(sock->s_conn_list[i] != NULL && server_conns_close(&sock->s_conn_list[i], i) == -1)
			{
				/* Unlock scoket mutex */
				pthread_mutex_unlock(&sock_mtx);
				return -1;
			}
		}
		free(sock->s_conn_list);
	}
	/* Print closed */
	printf("Socked %d closed!\n", fd);
	fflush(stdout);

	/* Unlock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}
