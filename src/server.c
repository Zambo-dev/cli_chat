#include "include.h"


int server_conns_init(conn_t **conn, int fd, char *ip)
{
	pthread_mutex_lock(&fd_mtx);

	*conn = (conn_t *)calloc(1, sizeof(conn_t));

	(*conn)->fd = fd;
	strcpy((*conn)->ip, ip);
	if(errck() == -1)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}

	pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int server_conns_close(sock_t *server, int idx)
{
	pthread_mutex_lock(&fd_mtx);

	close(server->conns[idx]->fd);
	if(errck() != 0)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}
	free(server->conns[idx]);
	server->conns[idx] = NULL;

	pool[idx] = 0;

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
		fflush(stdout);

		tdata_t tmp = {server, idx};
		pthread_create(&pool[idx], NULL, (void *)server_recv, (void *)&tmp);
	}

	return 0;
}

void server_recv(tdata_t *data)
{
	sock_t *server = data->sock;
	int idx = data->idx;

	char buffer[BUFFERLEN] = {0};
	char buffer2[BUFFERLEN] = {0};
	conn_t *c = server->conns[idx];
	int retval;

	while(1)
	{
		snprintf(buffer2, BUFFERLEN, "%s: ", c->ip);
		retval = recv(c->fd, buffer, BUFFERLEN, 0);
		if(errck() == -1 || retval == 0) break;

		strcat(buffer2, buffer);

		printf("%s", buffer2);
		fflush(stdout);

		if(strncmp(buffer, "/quit\n", 7) == 0) break;
		
		if(server_send(server, idx, buffer2) == -1) break;

		memset(buffer, 0, BUFFERLEN);
		memset(buffer2, 0, BUFFERLEN);
	}

	server_conns_close(server, idx);
	pthread_exit(0);
}

int server_send(sock_t *server, int idx, char *buffer)
{
	pthread_mutex_lock(&fd_mtx);
	for(size_t i=0; i<CONNLIMIT; ++i)
	{
		if(server->conns[i] != NULL)
		{
			send(server->conns[i]->fd, buffer, strlen(buffer), 0);
			if(errck() == -1)
			{
				pthread_mutex_unlock(&fd_mtx);
				return -1;
			}
		}
	}

	pthread_mutex_unlock(&fd_mtx);

	return 0;
}
