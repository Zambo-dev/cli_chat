#include "client.h"


int client_connect(client_t *client)
{
	size_t bytes;

	if(sock_connect(client->sock) == -1) return -1;
	puts("Connected!");

	bytes = strlen(client->sock->conf.username)+1;
	if(sock_write(client->sock, client->sock->conf.username, &bytes) == -1) return -1;

	return 0;
}

int client_read(client_t *client)
{
	int retval;
	size_t bytes;
	char *buffer = NULL;

	if((retval = sock_read(client->sock, &buffer, &bytes)) != 0) return -1;

	printf("SERVER: %s", buffer);
	fflush(stdout);

	if(strncmp(buffer, "/quit", 5) == 0) return 1;	


	return 0;
}

int client_write(client_t *client)
{
	int retval;
	size_t bytes;
	char *buffer = NULL;
	fd_set readfd;
	struct timeval tv;

	FD_ZERO(&readfd);
	FD_SET(0, &readfd);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(1, &readfd, NULL, NULL, &tv) == -1) return -1;
	if(FD_ISSET(0, &readfd))
	{
		buffer = (buffer == NULL)
			? (char *)realloc(buffer, 1024)
			: (char *)calloc(1024, 1);
		retval = read(0, buffer, 1024);
		bytes = strlen(buffer)+1;
		if(sock_write(client->sock, buffer, &bytes) == -1) return -1;
	}

	return 0;
}
