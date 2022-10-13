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
	if(argv[1][0] == 'c')	/* Client code */
	{
		sock_t client;
		if(sock_init(&client, argv[2], argv[3]) == -1) return EXIT_FAILURE;
		if(client_connect(&client) == -1) return EXIT_FAILURE;

		pthread_t send_thd, recv_thd;
		pthread_create(&send_thd, NULL, (void *)client_send, (void *)&client);
		pthread_create(&recv_thd, NULL, (void *)client_recv, (void *)&client);

		while(running);

		if(sock_close(&client) == -1) return EXIT_FAILURE;
	}
	else if(argv[1][0] == 's')	/* Server code */
	{
		sock_t server;
		if(sock_init(&server, NULL, argv[2]) == -1) return EXIT_FAILURE;
		pthread_t server_thd;
		pthread_create(&server_thd, NULL, (void *)server_connect, (void *)&server);

		char command = 0;
		while(command != 'q')
			command = fgetc(stdin);

		running = 0;
		for(int i=0; i<CONNLIMIT; ++i)
		{
			printf("Value of thread: %lu\n", pool[i]);
		}


		if(sock_close(&server) == -1) return EXIT_FAILURE;
	}
	else
	{
		puts("Wront paramenter! <s/c> <ip> <port>");
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}
