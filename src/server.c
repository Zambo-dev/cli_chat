#include "include.h"


int server_conns_init(conn_t **conn, int fd, char *ip)
{
	pthread_mutex_lock(&fd_mtx);

	*conn = (conn_t *)calloc(1, sizeof(conn_t));
	if(errck() == -1)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}
	(*conn)->c_fd = fd;
	strcpy((*conn)->c_ip, ip);
	if(errck() == -1)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}

	pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int server_conns_close(conn_t **conn, int idx)
{
	pthread_mutex_lock(&fd_mtx);
	int fd = (*conn)->c_fd;

	close((*conn)->c_fd);
	if(errck() == -1)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}
	free(*conn);
	
	*conn = NULL;
	pool[idx] = 0;

	printf("Connection %d closed!\n", fd);
	fflush(stdout);

	pthread_mutex_unlock(&fd_mtx);
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
	pthread_mutex_lock(&sock_mtx);

	/* Bind the socket */
	socklen_t len = sizeof(server->s_host);
	bind(server->s_fd, (struct sockaddr *)&server->s_host, len);
	if(errck() == -1)
	{
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	puts("Socket binded!");
	fflush(stdout);

	listen(server->s_fd, CONNLIMIT);
	if(errck() == -1)
	{
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	puts("Listening...");
	fflush(stdout);

	pthread_mutex_unlock(&sock_mtx);

	int idx;
	fd_set readfd;
	struct timeval tv;

	while(running)
	{
		while((idx = server_conns_getfree(server->s_conn_list)) == -1);

		FD_ZERO(&readfd);
		FD_SET(server->s_fd, &readfd);


		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(server->s_fd + 1, &readfd, NULL, NULL, &tv);
		if(!FD_ISSET(server->s_fd, &readfd)) continue;

		int fd_tmp = accept(server->s_fd, (struct sockaddr *)&server->s_host, &len);
		if(errck() == -1) continue;

		struct sockaddr_in client;
		socklen_t client_len = sizeof(server->s_host);

		getpeername(fd_tmp, (struct sockaddr *)&client, &client_len);
		
		if(server_conns_init(&server->s_conn_list[idx], fd_tmp, inet_ntoa(client.sin_addr)) == -1)
		{
			server_conns_close(&server->s_conn_list[idx], idx);
			continue;
		}
		printf("Connected to %d -> %s\n", fd_tmp, server->s_conn_list[idx]->c_ip);
		fflush(stdout);

		tdata_t tmp = {server, idx};
		pthread_create(&pool[idx], NULL, (void *)server_recv, (void *)&tmp);
	}

	return 0;
}

int server_recv(tdata_t *data)
{
	sock_t *server = data->sock;
	int idx = data->idx;

	char buffer[BUFFERLEN] = {0};
	char buffer2[BUFFERLEN] = {0};
	conn_t *c = server->s_conn_list[idx];
	int retval;

	fd_set readfd;
	struct timeval tv;

	while(1)
	{
		FD_ZERO(&readfd);
		FD_SET(c->c_fd, &readfd);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(c->c_fd + 1, &readfd, NULL, NULL, &tv);
		if(!FD_ISSET(c->c_fd, &readfd)) continue;

		retval = recv(c->c_fd, buffer, BUFFERLEN, 0);
		if(errck() == -1 || retval == 0) break;

		snprintf(buffer2, BUFFERLEN, "%s: ", c->c_ip);
		strcat(buffer2, buffer);

		printf("%s", buffer2);
		fflush(stdout);

		if(strncmp(buffer, "/quit\n", 7) == 0) break;
		if(server_send(server, idx, buffer2) == -1) break;

		memset(buffer, 0, BUFFERLEN);
		memset(buffer2, 0, BUFFERLEN);
	}

	server_conns_close(&server->s_conn_list[idx], idx);
	pthread_exit(0);
	return 0;
}

int server_send(sock_t *server, int idx, char *buffer)
{
	pthread_mutex_lock(&fd_mtx);
	for(size_t i=0; i<CONNLIMIT; ++i)
	{
		if(server->s_conn_list[i] != NULL)
		{
			send(server->s_conn_list[i]->c_fd, buffer, strlen(buffer), 0);
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
