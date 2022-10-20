#include "include.h"


int client_connect(sock_t *client)
{	
	/* Lock socket mutex*/
	pthread_mutex_lock(&sock_mtx);

	int retval;

	/* Connect to the server */
	socklen_t len = sizeof(client->s_host);
	if(connect(client->s_conn.c_fd, (struct sockaddr *)&client->s_host, len) == -1)
	{
		fd_errck("connect");
		/* Unlock socket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}

    /* Init SSL context */
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    if((client->s_conn.c_sslctx = SSL_CTX_new(TLS_client_method())) == NULL)
    {
		ssl_errck("SSL_CTX_new", 0);
        /* Unlock socket mutex */
        pthread_mutex_unlock(&sock_mtx);
        return -1;
    }

    /* Init SSL  */
    if((client->s_conn.c_ssl = SSL_new(client->s_conn.c_sslctx)) == NULL)
    {
		ssl_errck("SSL_new", 0);
        /* Unlock socket mutex */
        pthread_mutex_unlock(&sock_mtx);
        return -1;
    }
    if((retval = SSL_set_fd(client->s_conn.c_ssl, client->s_conn.c_fd)) == 0)
    {
		ssl_errck("SSL_set_fd", retval);
        /* Unlock socket mutex */
        pthread_mutex_unlock(&sock_mtx);
        return -1;
    }
    if((retval = SSL_connect(client->s_conn.c_ssl)) <= 0)
    {
		ssl_errck("SSL_connect", retval);
        /* Unlock socket mutex */
        pthread_mutex_unlock(&sock_mtx);
        return -1;
    }

	/* Print connection message */
	printf("Connected to %s!\n", inet_ntoa(client->s_host.sin_addr));
	fflush(stdout);

	/* Unlock socket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int client_recv(sock_t *client)
{
	char buffer[BUFFERLEN] = {0};
	int retval;
	int serv_row = 6;

	fd_set readfd;
	struct timeval tv;

	while(running)
	{
		FD_ZERO(&readfd);
		FD_SET(client->s_conn.c_fd, &readfd);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(client->s_conn.c_fd + 1, &readfd, NULL, NULL, &tv);
		if(!FD_ISSET(client->s_conn.c_fd, &readfd)) continue;
	
		if((retval = SSL_read(client->s_conn.c_ssl, buffer, BUFFERLEN)) <= 0)
		{
			ssl_errck("SSL_read", retval);
			/* Lock running mutex */
			pthread_mutex_lock(&run_mtx);
			running = 0;
			/* Unlock running mutex */
			pthread_mutex_unlock(&run_mtx);
			
			printf("\x1b[%d;1H\x1b[0KServer: Connection closed!\x1b[%d;1HClient: ", serv_row, cli_row);
			fflush(stdout);

			pthread_exit(0);
		}

		if(serv_row == cli_row-1)
		{
			printf("\n\n");
			fflush(stdout);
		}
		else
			++serv_row;

		printf("\x1b[%d;1H\x1b[0K%s\x1b[%d;1HClient: ", serv_row, buffer, cli_row);
		fflush(stdout);

		memset(buffer, 0, BUFFERLEN);
	}
	
	pthread_exit(0);
}

int client_send(sock_t *client)
{
	int retval;
	char buffer[BUFFERLEN] = {0};

	fd_set readfd;
	struct timeval tv;

	printf("\x1b[%d;1HClient: \x1b[0K", cli_row);
	fflush(stdout);

	while(running)
	{
		FD_ZERO(&readfd);
		FD_SET(STDIN_FILENO, &readfd);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		select(STDIN_FILENO+1, &readfd, NULL, NULL, &tv);
		if(!FD_ISSET(STDIN_FILENO, &readfd)) continue;

		if(read(STDIN_FILENO, buffer, BUFFERLEN) == -1)
		{
			fd_errck("read");
			/* Lock running mutex */
			pthread_mutex_lock(&run_mtx);
			running = 0;
			/* Unock running mutex */
			pthread_mutex_unlock(&run_mtx);
			
			pthread_exit(0);
		}

		if((retval = SSL_write(client->s_conn.c_ssl, buffer, BUFFERLEN)) <= 0)
		{
			ssl_errck("SSL_write", retval);
			/* Lock running mutex */
			pthread_mutex_lock(&run_mtx);
			running = 0;
			/* Unock running mutex */
			pthread_mutex_unlock(&run_mtx);
			
			pthread_exit(0);
		}

		printf("\x1b[%d;1HClient: \x1b[0K", cli_row);
		fflush(stdout);

		if(strncmp(buffer, "/quit\n", 7) == 0)
		{
			/* Lock running mutex */
			pthread_mutex_lock(&run_mtx);
			running = 0;
			/* Unock running mutex */
			pthread_mutex_unlock(&run_mtx);

			break;	
		}

		memset(buffer, 0, BUFFERLEN);
	}

	pthread_exit(0);
}
