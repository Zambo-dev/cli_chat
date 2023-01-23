#include "client.h"


static fd_set sigfd;

int client_connect(client_t *client)
{
	int retval;
	size_t bytes;
	struct timeval tv;

	if(sock_connect(client->sock) == -1) return -1;

	do
	{
		FD_ZERO(&sigfd);
		FD_SET(client->sock->fd, &sigfd);

		tv.tv_sec = 0;
		tv.tv_usec = 50000;

		if(select(client->sock->fd+1, NULL, &sigfd, NULL, &tv) == -1) return -1;
	}
	while(!FD_ISSET(client->sock->fd, &sigfd));

	FD_CLR(client->sock->fd, &sigfd);

	bytes = strlen(client->sock->conf.username)+1;
	if(sock_write(client->sock, client->sock->conf.username, &bytes) == -1) return -1;

	return 0;
}

int client_read(client_t *client)
{
	size_t bytes;
	char *buffer = NULL;
	struct timeval tv;

	FD_ZERO(&sigfd);
	FD_SET(client->sock->fd, &sigfd);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(client->sock->fd+1, &sigfd, NULL, NULL, &tv) > 0 
		&& FD_ISSET(client->sock->fd, &sigfd)
		&& sock_read(client->sock, &buffer, &bytes) == 0)
	{
		FD_CLR(client->sock->fd, &sigfd);

		printf("%s", buffer);
		fflush(stdout);

		if(buffer != NULL) free(buffer);
	}

	return 0;
}

int client_write(client_t *client)
{
	size_t bytes;
	struct timeval tv;

	FD_ZERO(&sigfd);
	FD_SET(0, &sigfd);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(1, &sigfd, NULL, NULL, &tv) > 0 && FD_ISSET(0, &sigfd))
	{	
		FD_CLR(0, &sigfd);

		char buffer[1024] = {0};
		if((bytes = read(0, buffer, 1024)) > 0)
		{
			do
			{
				FD_ZERO(&sigfd);
				FD_SET(client->sock->fd, &sigfd);

				tv.tv_sec = 0;
				tv.tv_usec = 50000;
				
				select(client->sock->fd+1,NULL, &sigfd, NULL, &tv);
			}
			while(!FD_ISSET(client->sock->fd, &sigfd));
			FD_CLR(client->sock->fd, &sigfd);

			++bytes;
			if(sock_write(client->sock, buffer, &bytes) == -1) return -1;
		}
	}

	return 0;
}
