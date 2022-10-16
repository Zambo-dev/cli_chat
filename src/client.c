#include "include.h"


int client_connect(sock_t *client)
{	
	/* Lock socket mutex*/
	pthread_mutex_lock(&sock_mtx);

	/* Connect to the server */
	socklen_t len = sizeof(client->host);
	connect(client->fd, (struct sockaddr *)&client->host, len);
	if(errck() == -1)
	{
		/* Unlock socket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	/* Print connection message */
	printf("Connected to %s!\n", inet_ntoa(client->host.sin_addr));
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

	while(running)
	{
		retval = recv(client->fd, buffer, BUFFERLEN, 0);
		if(errck() == -1 || retval == 0) 
		{	
			/* Lock running mutex */
			pthread_mutex_lock(&run_mtx);
			running = 0;
			/* Unlock running mutex */
			pthread_mutex_unlock(&run_mtx);
			
			printf("\x1b[%d;1H\x1b[0KServer: Connection closed!\x1b[%d;1HClient: ", serv_row, cli_row);
			fflush(stdout);

			pthread_exit(0);
			return -1;
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
	return 0;
}

int client_send(sock_t *client)
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
			/* Lock running mutex */
			pthread_mutex_lock(&run_mtx);
			running = 0;
			/* Unock running mutex */
			pthread_mutex_unlock(&run_mtx);
			
			pthread_exit(0);
			return -1;
		}

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
	return 0;
}
