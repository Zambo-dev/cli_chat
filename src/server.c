#include "server.h"
#include "err.h"


int server_conns_init(conn_t **conn, SSL_CTX *server_ctx, int fd, char *ip)
{
	pthread_mutex_lock(&fd_mtx);

	int retval;

	*conn = (conn_t *)calloc(1, sizeof(conn_t));
	(*conn)->c_fd = fd;
	strcpy((*conn)->c_ip, ip);

    /* Init SSL  */
    (*conn)->c_ssl = SSL_new(server_ctx);
    SSL_set_fd((*conn)->c_ssl, (*conn)->c_fd);
    if((retval = SSL_accept((*conn)->c_ssl)) == -1)
    {
		ssl_errck("SSL_accept", retval);
        /* Unlock socket mutex */
        pthread_mutex_unlock(&fd_mtx);
        return -1;
    }

	char username[32];
	if((retval = SSL_read((*conn)->c_ssl, username, 32)) <= 0)
	{
		ssl_errck("SSL_read", retval);
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}

	strncpy((*conn)->c_usnm, username, 32);

    pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int server_conns_close(conn_t **conn, int idx)
{
	pthread_mutex_lock(&fd_mtx);

	server_send(conn, "/quit\n");

    int fd = (*conn)->c_fd;
    SSL_free((*conn)->c_ssl);
    if(close((*conn)->c_fd) == -1)
	{
		fd_errck("close");
		/* Unlock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
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
	if(bind(server->s_conn.c_fd, (struct sockaddr *)&server->s_host, len) == -1)
	{
		fd_errck("bind");
		pthread_mutex_unlock(&sock_mtx);
		pthread_mutex_lock(&run_mtx);
		running = 0;
		pthread_mutex_unlock(&run_mtx);
        pthread_exit(0);
	}
	puts("Socket binded!");
	fflush(stdout);

	if(listen(server->s_conn.c_fd, CONNLIMIT) == -1)
	{
		fd_errck("listen");
		pthread_mutex_unlock(&sock_mtx);
		pthread_mutex_lock(&run_mtx);
		running = 0;
		pthread_mutex_unlock(&run_mtx);
        pthread_exit(0);
	}
	puts("Listening...");
	fflush(stdout);

	pthread_mutex_unlock(&sock_mtx);

	int idx, run;
	fd_set readfd;
	struct timeval tv;
	int fd_tmp;

	while(1)
	{
		pthread_mutex_lock(&run_mtx);
		if(!running) break;
		pthread_mutex_unlock(&run_mtx);

		while((idx = server_conns_getfree(server->s_conn_list)) == -1);

		FD_ZERO(&readfd);
		FD_SET(server->s_conn.c_fd, &readfd);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(server->s_conn.c_fd+1, &readfd, NULL, NULL, &tv);
		if(!FD_ISSET(server->s_conn.c_fd, &readfd)) continue;

		if((fd_tmp = accept(server->s_conn.c_fd, (struct sockaddr *)&server->s_host, &len)) == -1)
		{
			fd_errck("accept");
			continue;
		}

		struct sockaddr_in client;
		socklen_t client_len = sizeof(server->s_host);
		getpeername(fd_tmp, (struct sockaddr *)&client, &client_len);
		
		if(server_conns_init(&server->s_conn_list[idx], server->s_conn.c_sslctx, fd_tmp, inet_ntoa(client.sin_addr)) == -1)
		{
			fd_errck("server_conns_init");
			server_conns_close(&server->s_conn_list[idx], idx);
			continue;
		}
		printf("Connected to %d -> %s\n", fd_tmp, server->s_conn_list[idx]->c_ip);
		fflush(stdout);

		pthread_mutex_lock(&tdata_mtx);

		tdata_t tmp = {server, idx};
		pthread_create(&pool[idx], NULL, (void *)server_recv, (void *)&tmp);
	}

    pthread_exit(0);
}

int server_recv(tdata_t *data)
{
	sock_t *server = data->sock;
	int idx = data->idx;

	pthread_mutex_unlock(&tdata_mtx);

	char buffer[BUFFERLEN] = {0};
	char buffer2[BUFFERLEN] = {0};
	conn_t *c = server->s_conn_list[idx];
	int retval;

	fd_set readfd;
	struct timeval tv;

	while(running)
	{
		FD_ZERO(&readfd);
		FD_SET(c->c_fd, &readfd);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(c->c_fd + 1, &readfd, NULL, NULL, &tv);
		if(!FD_ISSET(c->c_fd, &readfd)) continue;

		if((retval = SSL_read(c->c_ssl, buffer, BUFFERLEN)) <= 0)
		{
			ssl_errck("SSL_read", retval);
			pthread_mutex_unlock(&fd_mtx);
			pthread_mutex_lock(&run_mtx);
			running = 0;
			pthread_mutex_unlock(&run_mtx);
			break;
		}

		snprintf(buffer2, BUFFERLEN, "%s: ", c->c_usnm);
		strcat(buffer2, buffer);

		printf("%s", buffer2);
		fflush(stdout);

		if(strncmp(buffer, "/quit\n", 6) == 0)
		{
			if((retval = SSL_write(server->s_conn_list[idx]->c_ssl, buffer, strlen(buffer))) <= 0)
			{
				ssl_errck("SSL_write", retval);
				pthread_mutex_unlock(&fd_mtx);
				pthread_mutex_lock(&run_mtx);
				running = 0;
				pthread_mutex_unlock(&run_mtx);
				break;
			}
			break;
		}

		pthread_mutex_lock(&fd_mtx);
		if(server_send(server->s_conn_list, buffer2) == -1) break;
		pthread_mutex_unlock(&fd_mtx);


		memset(buffer, 0, BUFFERLEN);
		memset(buffer2, 0, BUFFERLEN);
	}

	server_conns_close(&server->s_conn_list[idx], idx);
	pthread_exit(0);
}

int server_send(conn_t **conns, char *buffer)
{
	int retval;

	for(size_t i=0; i<CONNLIMIT; ++i)
	{
		if(conns[i] != NULL)
		{
			if((retval = SSL_write(conns[i]->c_ssl, buffer, strlen(buffer))) <= 0)
			{
				ssl_errck("SSL_write", retval);
				pthread_mutex_unlock(&fd_mtx);
				return -1;
			}
		}
	}

	return 0;
}
