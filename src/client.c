#include "include.h"


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

void client_recv(sock_t *client)
{
	char buffer[BUFFERLEN] = {0};
	int retval;
	int serv_row = 5;

	while(running)
	{
		retval = recv(client->fd, buffer, BUFFERLEN, 0);
		if(errck() == -1 || retval == 0) 
		{
			pthread_mutex_lock(&run_mtx);
			running = 0;
			pthread_mutex_unlock(&run_mtx);
			
			printf("\x1b[%d;1H\x1b[0KServer: Connection closed! Press ENTER to quit.\x1b[%d;1HClient: ", serv_row, cli_row);
			fflush(stdout);

			++serv_row;

			break;
		}

		if(serv_row == cli_row-1)
		{
			printf("\n\n");
			fflush(stdout);
		}
		else
			++serv_row;

		if(running)
		{
			printf("\x1b[%d;1H\x1b[0K%s\x1b[%d;1HClient: ", serv_row, buffer, cli_row);
			fflush(stdout);
		}

		memset(buffer, 0, BUFFERLEN);
	}
	
	pthread_exit(0);
}

void client_send(sock_t *client)
{
	char buffer[BUFFERLEN] = {0};

	while(running)
	{
		printf("\x1b[%d;1HClient: \x1b[0K", cli_row);
		fflush(stdout);
		read(STDIN_FILENO, buffer, BUFFERLEN);
		fflush(stdin);
		
		send(client->fd, buffer, BUFFERLEN, 0);
		if(errck() == -1)
		{
			pthread_mutex_lock(&run_mtx);
			running = 0;
			pthread_mutex_unlock(&run_mtx);
			
			break;
		}

		if(strncmp(buffer, "/quit\n", 7) == 0)
		{
			pthread_mutex_lock(&run_mtx);
			running = 0;
			pthread_mutex_unlock(&run_mtx);

			break;	
		}

		memset(buffer, 0, BUFFERLEN);
	}

	sock_close(client);

	pthread_exit(0);
}
