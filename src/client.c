#include "include.h"

extern pthread_mutex_t fd_mtx;
extern pthread_mutex_t sock_mtx;
extern pthread_mutex_t errno_mtx;
extern pthread_mutex_t tdata_mtx;
extern pthread_t pool[CONNLIMIT];
extern int running;

int client_connect(sock_t *client)
{	
	pthread_mutex_lock(&fd_mtx);

	/* Connect to the server */
	socklen_t len = sizeof(client->host);
	int connfd = connect(client->fd, (struct sockaddr *)&client->host, len);
	
	pthread_mutex_unlock(&fd_mtx);
	
	if(errck() != 0) return -1;
	printf("Connected!\n");

	return connfd;
}

int client_recv(sock_t *client)
{
	char buffer[BUFFERLEN] = {0};
	int bytes;

	printf("\x1b[5;1HServer: ");
	fflush(stdout);

	while(1)
	{
		while(read(client->fd, buffer, BUFFERLEN) != 0)
		{	
			if(errck() == -1) return -1;

			fprintf(stdout, "\x1b[5;10H%s\n", buffer);
			fflush(stdout);

			memset(buffer, 0, BUFFERLEN);
		}

		printf("\x1b[10;1HClient: ");
		fflush(stdout);
	}

	return 0;
}

int client_send(sock_t *client)
{
	char buffer[BUFFERLEN] = {0};

	while(1)
	{
		printf("\x1b[10;1HClient: ");
		fflush(stdout);
		fgets(buffer, BUFFERLEN, stdin);
		fflush(stdin);

		send(client->fd, buffer, BUFFERLEN, 0);
		
		if(errck() == -1) return -1;
		memset(buffer, 0, BUFFERLEN);
	}

	return 0;
}
