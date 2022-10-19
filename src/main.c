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
		puts("\nClosed sending thread!");
		while(pthread_join(recv_thd, 0) != 0);
		puts("Closed receiving thread!");

		if(sock_close(&client) == -1) return EXIT_FAILURE;

        close(client.s_conn.c_fd);
        SSL_free(client.s_conn.c_ssl);
        SSL_CTX_free(client.s_conn.c_sslctx);
	}
	else if(argv[1][0] == 's')	/* Server code */
	{
		sock_t server;
		if(sock_init(&server, NULL, argv[2]) == -1) return EXIT_FAILURE;

        /* Init SSL context */
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        if((server.s_conn.c_sslctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
            return EXIT_FAILURE;
        /* set the local certificate from CertFile */
        if(SSL_CTX_use_certificate_file(server.s_conn.c_sslctx, argv[3], SSL_FILETYPE_PEM) <= 0)
            return EXIT_FAILURE;
        /* set the private key from KeyFile (may be the same as CertFile) */
        if(SSL_CTX_use_PrivateKey_file(server.s_conn.c_sslctx, argv[4], SSL_FILETYPE_PEM) <= 0)
            return EXIT_FAILURE;
        /* verify private key */
        if(!SSL_CTX_check_private_key(server.s_conn.c_sslctx))
            return EXIT_FAILURE;

        printf("Running: %d\n", running);
        fflush(stdout);
		pthread_t server_thd;
        if(running)
		    pthread_create(&server_thd, NULL, (void *)server_connect, (void *)&server);

		char command[32] = {0};
		fd_set readfd;
		struct timeval tv;

		do
		{
			FD_ZERO(&readfd);
			FD_SET(STDIN_FILENO, &readfd);

			tv.tv_sec = 1;
			tv.tv_usec = 0;

			select(STDIN_FILENO+1, &readfd, NULL, NULL, &tv);
			if(!FD_ISSET(STDIN_FILENO, &readfd)) continue;
			
			if(read(STDIN_FILENO, command, 32) == -1)
			{
				puts("OK here");
				fflush(stdout);
				break;
			}
		}
		while(strstr(command, "/quit") == NULL);

        pthread_mutex_lock(&run_mtx);
		running = 0;
        pthread_mutex_unlock(&run_mtx);

        SSL_free(server.s_conn.c_ssl);
        SSL_CTX_free(server.s_conn.c_sslctx);
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
