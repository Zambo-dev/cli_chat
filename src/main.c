#include <stdio.h>
#include <stdlib.h>
#include "sock.h"
#include "client.h"
#include "server.h"
#include "err.h"
#include "config.h"


int parse_args(int argc, char **argv, char *find)
{
	for(int i=0; i<argc; ++i)
		if(strcmp(argv[i], find) == 0)
			return i+1;

	return 0;
}

int main(int argc, char** argv)
{
	conf_t conf;
	int retval, is_client;

	if((retval = parse_args(argc, argv, "-t")) == 0)
	{
		puts("Wrong paramenter! -t <s/c> -c <config.conf>");
		return EXIT_FAILURE;
	}

	is_client = (argv[retval][0] == 'c') ? 1 : 0;

	printf("\x1b[2J\x1b[1;1H");
	fflush(stdout);

	if((retval = parse_args(argc, argv, "-c")) == 0)
	{
		if(is_client)
			conf_load(&conf, "client.conf");
		else
			conf_load(&conf, "server.conf");
	}
	else
		conf_load(&conf, argv[retval]);

	if(is_client) printf("CLIENT"); else printf("SERVER");

	if(is_client)	/* Client code */
	{
		sock_t client;
		pthread_t send_thd, recv_thd;
		struct winsize wsize;

		ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);
        cli_row = wsize.ws_row-1; 

		if(sock_init(&client, conf.ip, conf.port, NULL, NULL) == -1) return EXIT_FAILURE;
		if(client_connect(&client) == -1) return EXIT_FAILURE;

		pthread_create(&send_thd, NULL, (void *)client_send, (void *)&client);
		pthread_create(&recv_thd, NULL, (void *)client_recv, (void *)&client);

		while(running);
		
		while(pthread_join(send_thd, 0) != 0);
		puts("Closed sending thread!\n");
		while(pthread_join(recv_thd, 0) != 0);
		puts("Closed receiving thread!\n");

		if(sock_close(&client) == -1) return EXIT_FAILURE;
	}
	else if(argv[1][0] == 's')	/* Server code */
	{
		int retval;
		char command[32] = {0};
		sock_t server;
		pthread_t server_thd;
		fd_set readfd;
		struct timeval tv;

		if(sock_init(&server, NULL, conf.port, conf.certfile, conf.keyfile) == -1) return EXIT_FAILURE;

		pthread_create(&server_thd, NULL, (void *)server_connect, (void *)&server);

		do
		{
			FD_ZERO(&readfd);
			FD_SET(STDIN_FILENO, &readfd);

			tv.tv_sec = 1;
			tv.tv_usec = 0;

			select(STDIN_FILENO+1, &readfd, NULL, NULL, &tv);
			if(!FD_ISSET(STDIN_FILENO, &readfd)) continue;

			if(read(STDIN_FILENO, command, 32) == -1) break;
		}
		while(strncmp(command, "/quit\n", 6) != 0);

		pthread_mutex_lock(&run_mtx);
		running = 0;
		pthread_mutex_unlock(&run_mtx);

		while(pthread_join(server_thd, 0) != 0);
		puts("Closed connection thread!\n");

		if(sock_close(&server) == -1) return EXIT_FAILURE;
		
		for(int i=0; i<CONNLIMIT; ++i)
			printf("Threads status: %lu\n", pool[i]);
	}
	else
	{
		puts("Wrong paramenter! <s/c> <c_ip> <port>");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
