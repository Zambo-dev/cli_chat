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
	int retval;

	do
	{
		retval = recv(client->fd, buffer, BUFFERLEN, 0);
		printf("RETVAL: %d\n", retval);
		if(errck() == -1 || retval == 0) 
		{
			puts("Connection closed!");
			running = 0;
			return -1;
		}

		fprintf(stdout, "\x1b[5;1H%s\x1b[10;10H", buffer);
		fflush(stdout);

		memset(buffer, 0, BUFFERLEN);
	}
	while(running);

	return 0;
}

int client_send(sock_t *client)
{
	char buffer[BUFFERLEN] = {0};

	while(running)
	{
		printf("\x1b[10;1HClient: \x1b[0K");
		fflush(stdout);
		fgets(buffer, BUFFERLEN, stdin);
		fflush(stdin);

		send(client->fd, buffer, BUFFERLEN, 0);
		
		if(errck() == -1) return -1;
		memset(buffer, 0, BUFFERLEN);
	}

	return 0;
}
