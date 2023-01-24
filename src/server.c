#include "server.h"


static fd_set sigfd;

static int list_get_free(sock_t *list[CONNLIMIT])
{
	for(int i=0; i<CONNLIMIT; ++i)
		if(list[i] == NULL)
			return i;

	return -1;
}

int server_accept(server_t *server)
{
	int retval, idx;
	size_t bytes;
	char *buffer = NULL;
	struct timeval tv;

	if((idx = list_get_free(server->list)) == -1) return -1;
	server->list[idx] = (sock_t *)calloc(1, sizeof(sock_t));

	if((retval = sock_accept(server->sock, server->list[idx])) == -1)
	{	
		free(server->list[idx]);
		server->list[idx] = NULL;
		return -1;
	}

	do
	{
		FD_ZERO(&sigfd);
		FD_SET(server->list[idx]->fd, &sigfd);

		tv.tv_sec = 0;
		tv.tv_usec = 50000;

		select(server->list[idx]->fd+1, &sigfd, NULL, NULL, &tv);
	}
	while(!FD_ISSET(server->list[idx]->fd, &sigfd));
	
	FD_CLR(server->list[idx]->fd, &sigfd);

	if(sock_read(server->list[idx], &buffer, &bytes) == -1)
	{
		free(server->list[idx]);
		server->list[idx] = NULL;
		return -1;
	}

	strncpy(server->list[idx]->conf.username, buffer, bytes);

	puts("New client:");
	conf_log(&server->list[idx]->conf);
	fflush(stdout);

	return 0;
}
int server_read(server_t *server)
{
	int retval, maxfd = -1;
	size_t bytes;
	char *buffer = NULL;
	struct timeval tv;

	FD_ZERO(&sigfd);
	for(int i=0; i<CONNLIMIT; ++i)
	{
		if(server->list[i] != NULL)
		{
			FD_SET(server->list[i]->fd, &sigfd);
			if(maxfd < server->list[i]->fd) maxfd = server->list[i]->fd;
		}
	}

	if(maxfd == -1) return -1;

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(maxfd+1, &sigfd, NULL, NULL, &tv) <= 0) return -1;
	
	for(int i=0; i<CONNLIMIT; ++i)
	{
		if(server->list[i] == NULL || !FD_ISSET(server->list[i]->fd, &sigfd)) continue;
		
		FD_CLR(server->list[i]->fd, &sigfd);

		if(sock_read(server->list[i], &buffer, &bytes) == -1) return -1;
						
		if(strncmp(buffer, "/quit", 5) == 0)
		{
			if(sock_close(server->list[i]) == 0)
				printf("Closed connection with %s\n", server->list[i]->conf.username);
			fflush(stdout);
			
			free(server->list[i]);
			server->list[i] = NULL;
			
			break;
		}

		size_t len = strlen(server->list[i]->conf.username)+strlen(buffer)+3;
		char *msg = (char *)calloc(len, 1);
		sprintf(msg, "%s: %s", server->list[i]->conf.username, buffer);

		puts(msg);
		fflush(stdout);

		if(server_write(server, msg, len) == -1) return -1;

		break;
	}
	
	return 0;
}

int server_write(server_t *server, char *buffer, size_t bytes)
{
	int retval, maxfd = -1;
	struct timeval tv;

	FD_ZERO(&sigfd);
	for(int i=0; i<CONNLIMIT; ++i)
	{
		if(server->list[i] != NULL)
		{
			FD_SET(server->list[i]->fd, &sigfd);
			if(maxfd < server->list[i]->fd) maxfd = server->list[i]->fd;
		}
	}
	
	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(maxfd+1, NULL, &sigfd, NULL, &tv) <= 0) return -1;

	for(int i=0; i<CONNLIMIT; ++i)
	{
		if(server->list[i] == NULL || !FD_ISSET(server->list[i]->fd, &sigfd)) continue;
	
		FD_SET(server->list[i]->fd, &sigfd);
		sock_write(server->list[i], buffer, &bytes);
	}

	return 0;
}
