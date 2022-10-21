#include <stdio.h>
#include <stdlib.h>
#include "include.h"


int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		puts("Missing parameters: <s/c> <c_ip> <port>");
		return EXIT_FAILURE;
	} 

	printf("\x1b[2J\x1b[1;1H");
	fflush(stdout);

	if(argv[1][0] == 'c')	/* Client code */
	{
		sock_t client;
		pthread_t send_thd, recv_thd;
		struct winsize wsize;

		ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);
        cli_row = wsize.ws_row-1; 

		if(sock_init(&client, argv[2], argv[3], NULL, NULL) == -1) return EXIT_FAILURE;
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

		if(sock_init(&server, NULL, argv[2], argv[3], argv[4]) == -1) return EXIT_FAILURE;

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
		while(strstr(command, "/quit") == NULL);

        pthread_mutex_lock(&run_mtx);
		running = 0;
        pthread_mutex_unlock(&run_mtx);

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
