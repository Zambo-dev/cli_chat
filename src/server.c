#include "include.h"

extern pthread_mutex_t fd_mtx;
extern pthread_mutex_t sock_mtx;
extern pthread_mutex_t errno_mtx;
extern pthread_mutex_t tdata_mtx;
extern pthread_t pool[CONNLIMIT];
extern int running;

int server_conns_init(conn_t **conn, int fd, char *ip)
{
	pthread_mutex_lock(&fd_mtx);

	*conn = (conn_t *)calloc(1, sizeof(conn_t));

	(*conn)->fd = fd;
	strcpy((*conn)->ip, ip);
	if(errck() == -1) return -1;

	pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int server_conns_close(conn_t **conn)
{
	pthread_mutex_lock(&fd_mtx);

	close((*conn)->fd);
	if(errck() != 0)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}
	free(*conn);
	*conn = NULL;
	pthread_mutex_unlock(&fd_mtx);
	printf("Connection closed!\n");

	return 0;
}

int server_conns_getfree(conn_t **conns)
{
	pthread_mutex_lock(&fd_mtx);
	
	for(size_t i=0; i<CONNLIMIT; ++i)
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

int server_connect(sock_t *server)
{
	/* Bind the socket */
	socklen_t len = sizeof(server->host);
	bind(server->fd, (struct sockaddr *)&server->host, len);
	if(errck() != 0) return -1;
	puts("Socket binded!");

	listen(server->fd, CONNLIMIT);
	if(errck() != 0) return -1;
	puts("Listening...");

	int idx;

	while(running)
	{
		while((idx = server_conns_getfree(server->conns)) == -1);

		int fd_tmp = accept(server->fd, (struct sockaddr *)&server->host, &len);
		if(errck() == -1) continue;

		struct sockaddr_in client;
		socklen_t client_len = sizeof(server->host);
		getpeername(fd_tmp, (struct sockaddr *)&client, &client_len);
		server_conns_init(&server->conns[idx], fd_tmp, inet_ntoa(client.sin_addr));
		printf("Connected to %d -> %s\n", fd_tmp, server->conns[idx]->ip);

		pthread_mutex_lock(&tdata_mtx);
		tdata_t tmp = {server, idx};
		pthread_create(&pool[idx], NULL, (void *)server_recv, (void *)&tmp);
	}

	return 0;
}

int server_recv(tdata_t *data)
{
	sock_t *server = data->sock;
	int idx = data->idx;
	
	pthread_mutex_unlock(&tdata_mtx);

	char buffer[BUFFERLEN] = {0};
	conn_t *c = server->conns[idx];
	int retval;
	int error = 0;

	while(1)
	{
		retval = 1;
		while(1)
		{
			retval = recv(c->fd, buffer, BUFFERLEN, 0);
			printf("retval: %d\n", retval);
			if(errck() == -1 || retval == 0)
			{
				error = 1;
				break;
			}

			fprintf(stdout, "%s: %s\n", c->ip, buffer);
			fflush(stdout);
			
			server_send(server, idx, buffer);

			memset(buffer, 0, BUFFERLEN);
		}
		if(error) break;
	}

	server_conns_close(&c);

	return 0;
}

int server_send(sock_t *server, int idx, char *buffer)
{
	for(size_t i=0; i<CONNLIMIT; ++i)
	{
		if(server->conns[i] != NULL && i != idx)
		{
			send(server->conns[i]->fd, buffer, BUFFERLEN, 0);
			if(errck() == -1) return -1;
		}
	}
	return 0;
}
