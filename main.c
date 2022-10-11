#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#define BUFFERLEN 512
#define CONNLIMIT 8


pthread_mutex_t fd_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sock_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t errno_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tdata_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_t pool[CONNLIMIT];
int running = 1;

typedef struct CONN_T
{
	int fd;
	char ip[32];
} conn_t;

typedef struct SOCK_T
{
	int fd;
	conn_t **conns;
	struct sockaddr_in host;
} sock_t;

typedef struct THREAD_DATA
{
	sock_t *sock;
	int idx;
} tdata_t;

int errck()
{
	pthread_mutex_lock(&errno_mtx);
	if(errno != 0)
	{
		printf("ERROR %d: %s\n", errno, strerror(errno));
		errno = 0;
		pthread_mutex_unlock(&errno_mtx);
		return -1;
	}
	pthread_mutex_unlock(&errno_mtx);

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

int server_conns_set(conn_t **conn, int fd, char *ip)
{
	pthread_mutex_lock(&fd_mtx);

	*conn = (conn_t *)calloc(1, sizeof(conn_t));

	(*conn)->fd = fd;
	strcpy((*conn)->ip, ip);
	if(errck() == -1) return -1;

	pthread_mutex_unlock(&fd_mtx);
	return 0;
}

int server_conns_close(conn_t **conn)
{
	pthread_mutex_lock(&fd_mtx);

	close((*conn)->fd);
	if(errck() != 0)
	{
		pthread_mutex_unlock(&fd_mtx);
		return -1;
	}
	free(*conn);
	*conn = NULL;
	pthread_mutex_unlock(&fd_mtx);
	printf("Connection closed!\n");

	return 0;
}

int sock_init(sock_t *sock, char* ip, char *port)
{	
	/* Create socket */
	sock->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(errck() != 0) return -1;
	printf("Socked created! id: %d\n", sock->fd);

	/* Setup host data */
	sock->host.sin_family = AF_INET;
	sock->host.sin_port = htons(strtol(port, NULL, 10));
	sock->host.sin_addr.s_addr = (ip == NULL) ? INADDR_ANY : inet_addr(ip);
	errck();

	/* Set conns only for server */
	sock->conns = (ip == NULL) ? (conn_t **)calloc(CONNLIMIT, sizeof(conn_t)) : NULL;

	return 0;
}

int sock_close(sock_t *sock)
{	
	pthread_mutex_lock(&sock_mtx);

	close(sock->fd);
	sock->fd = 0;

	memset(&sock->host, 0, sizeof(struct sockaddr_in));
	if(sock->conns != NULL)
	{
		for(size_t i=0; i<CONNLIMIT; ++i)
			if(sock->conns[i] != NULL)
				server_conns_close(&sock->conns[i]);
		free(sock->conns);
	}

	pthread_mutex_unlock(&sock_mtx);
	puts("Socket closed!");


	return 0;
}

int server_send(sock_t *server, int idx, char *buffer)
{
	for(size_t i=0; i<CONNLIMIT; ++i)
	{
		if(server->conns[i] != NULL && i != idx)
		{
			send(server->conns[i]->fd, buffer, BUFFERLEN, 0);
			if(errck() == -1) return -1;
		}
	}

}

int server_recv(tdata_t *data)
{
	sock_t *server = data->sock;
	int idx = data->idx;
	
	pthread_mutex_unlock(&tdata_mtx);

	char buffer[BUFFERLEN] = {0};
	conn_t *c = server->conns[idx];
	int retval;
	int error = 0;

	while(1)
	{
		retval = 1;
		while(1)
		{
			retval = recv(c->fd, buffer, BUFFERLEN, 0);
			printf("retval: %d\n", retval);
			if(errck() == -1 || retval == 0)
			{
				error = 1;
				break;
			}

			fprintf(stdout, "%s: %s\n", c->ip, buffer);
			fflush(stdout);
			
			server_send(server, idx, buffer);

			memset(buffer, 0, BUFFERLEN);
		}
		if(error) break;
	}

	server_conns_close(&c);

	return 0;
}

int server_connect(sock_t *server)
{
	/* Bind the socket */
	socklen_t len = sizeof(server->host);
	bind(server->fd, (struct sockaddr *)&server->host, len);
	if(errck() != 0) return -1;
	puts("Socket binded!");

	listen(server->fd, CONNLIMIT);
	if(errck() != 0) return -1;
	puts("Listening...");

	int idx;

	while(running)
	{
		while((idx = server_conns_getfree(server->conns)) == -1);

		int fd_tmp = accept(server->fd, (struct sockaddr *)&server->host, &len);
		if(errck() == -1) continue;

		struct sockaddr_in client;
		socklen_t client_len = sizeof(server->host);
		getpeername(fd_tmp, (struct sockaddr *)&client, &client_len);
		server_conns_set(&server->conns[idx], fd_tmp, inet_ntoa(client.sin_addr));
		printf("Connected to %d -> %s\n", fd_tmp, server->conns[idx]->ip);

		pthread_mutex_lock(&tdata_mtx);
		tdata_t tmp = {server, idx};
		pthread_create(&pool[idx], NULL, (void *)server_recv, (void *)&tmp);
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

		printf("\n");
		fflush(stdout);
	}

	return 0;
}

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


int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		puts("Missing parameters: <s/c> <ip> <port>");
		return EXIT_FAILURE;
	} 
	if(argv[1][0] == 'c')	/* Client code */
	{
		sock_t client;
		if(sock_init(&client, argv[2], argv[3]) == -1) return EXIT_FAILURE;
		if(client_connect(&client) == -1) return EXIT_FAILURE;

		pthread_t send_thd, recv_thd;
		pthread_create(&send_thd, NULL, (void *)client_send, (void *)&client);
		pthread_create(&recv_thd, NULL, (void *)client_recv, (void *)&client);

		while(1);

		if(sock_close(&client) == -1) return EXIT_FAILURE;
	}
	else if(argv[1][0] == 's')	/* Server code */
	{
		sock_t server;
		if(sock_init(&server, NULL, argv[2]) == -1) return EXIT_FAILURE;
		if(server_connect(&server) == -1) return EXIT_FAILURE;
		
		while(1);
		
		if(sock_close(&server) == -1) return EXIT_FAILURE;
	}
	else
	{
		puts("Wront paramenter! <s/c> <ip> <port>");
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}
