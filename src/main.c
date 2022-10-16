#include <stdio.h>
#include <stdlib.h>
#include "include.h"


int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		puts("Missing parameters: <s/c> <ip> <port>");
		return EXIT_FAILURE;
	} 

	printf("\x1b[2J\x1b[1;1H");
	fflush(stdout);

	if(argv[1][0] == 'c')	/* Client code */
	{
		struct winsize wsize;
    	ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);

        cli_row = wsize.ws_row-1; 

		sock_t client;
		if(sock_init(&client, argv[2], argv[3]) == -1) return EXIT_FAILURE;
		if(client_connect(&client) == -1) return EXIT_FAILURE;

		pthread_t send_thd, recv_thd;
		
		pthread_create(&send_thd, NULL, (void *)client_send, (void *)&client);
		pthread_create(&recv_thd, NULL, (void *)client_recv, (void *)&client);

		while(running);
		
		while(pthread_join(send_thd, 0) != 0);
		puts("Closed sending thread!");
		while(pthread_join(recv_thd, 0) != 0);
		puts("Closed receiving thread!");

		if(sock_close(&client) == -1) return EXIT_FAILURE;
	}
	else if(argv[1][0] == 's')	/* Server code */
	{
		sock_t server;
		if(sock_init(&server, NULL, argv[2]) == -1) return EXIT_FAILURE;
		pthread_t server_thd;
		pthread_create(&server_thd, NULL, (void *)server_connect, (void *)&server);

		char command[32] = {0};
		do
		{
			read(STDIN_FILENO, command, 32);
		}
		while(strstr(command, "/quit") == NULL);

		running = 0;

		if(sock_close(&server) == -1) return EXIT_FAILURE;
		
		for(int i=0; i<CONNLIMIT; ++i)
			printf("Threads status: %lu\n", pool[i]);
	}
	else
	{
		puts("Wrong paramenter! <s/c> <ip> <port>");
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}
