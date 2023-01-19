#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "sock.h"
#include "err.h"
#include "conf.h"
#include "client.h"


int parse_args(int argc, char **argv, char *find)
{
	for(int i=0; i<argc; ++i)
		if(strstr(argv[i], find) != NULL)
			return i+1;

	return 0;
}

int main(int argc, char** argv)
{
	sock_t sock;
	int retval;
	size_t bytes;

	if((retval = parse_args(argc, argv, "-t")) == 0)
	{
		puts("Wrong paramenter! -t <s/c>");
		return EXIT_FAILURE;
	}
	/* Check if socket is client or server */
	sock.conf.type = argv[retval][0];

	if(parse_args(argc, argv, "-c") != 0)
	{
		char tmp[BUFFILE] = {0};
		char buffer[BUFFILE] = {0};
		char quote = '"';
		int count = 0;
		char *arr[6] = {"T=", "U=", "I=", "P=", "C=", "K="};

		while(count < 6)
		{
			retval = parse_args(argc, argv, arr[count]) - 1;
			sprintf(tmp, "%s=%c%s%c\r\n", arr[count], quote, (retval > 0) ? argv[retval]+2 : " ", quote);
			strcat(buffer, tmp);

			++retval;
			++count;
		}

		conf_init_buff(&sock.conf, buffer);

		if((retval = parse_args(argc, argv, "-s")) != 0)
			conf_write(&sock.conf, argv[retval]);
	}
	else if((retval = parse_args(argc, argv, "-l")) == 0)
	{
		if(sock.conf.type == 'c')
			conf_read(&sock.conf, "client.conf");
		else
			conf_read(&sock.conf, "server.conf");
	}
	else
		conf_read(&sock.conf, argv[retval]);

	conf_log(&sock.conf);

	sock_init(&sock);

	if(sock.conf.type == 'c')		// -------------------------------------- CLIENT
	{
		char *buffer = NULL;
		fd_set sigfd;
		struct timeval tv;
		client_t client;

		client.sock = &sock;

		if(client_connect(&client) == -1) return EXIT_FAILURE;
		puts("Connected!");
		fflush(stdout);
		
		do
		{
			if(client_read(&client) == -1) break;
			client_write(&client);
		}
		while(1);
	}
	else					// -------------------------------------- SERVER
	{
		char *buffer = NULL;
		fd_set sigfd;
		struct timeval tv;

		if(sock_listen(&sock) == -1) return EXIT_FAILURE;
		puts("Listening...");
		sock_t client;

		while((retval = sock_accept(&sock, &client)) != 0);
		puts("Connection accepted!");
		fflush(stdout);

		do
		{
			FD_ZERO(&sigfd);
			FD_SET(client.fd, &sigfd);

			tv.tv_sec = 0;
			tv.tv_usec = 50000;

			select(client.fd+1, &sigfd, NULL, NULL, &tv);
		}
		while(!FD_ISSET(client.fd, &sigfd));
		
		sock_read(&client, &buffer, &bytes);
		strncpy(client.conf.username, buffer, bytes);

		FD_CLR(client.fd, &sigfd);

		puts("CLIENT:");
		conf_log(&client.conf);

		do
		{	
			FD_ZERO(&sigfd);
			FD_SET(client.fd, &sigfd);

			tv.tv_sec = 0;
			tv.tv_usec = 50000;

			if(select(client.fd+1, &sigfd, NULL, NULL, &tv) > 0 && FD_ISSET(client.fd, &sigfd))
			{
				if(sock_read(&client, &buffer, &bytes) == 0)
				{
					printf("%s: %s\n", client.conf.username, buffer);
					fflush(stdout);
			
					FD_ZERO(&sigfd);
					FD_SET(client.fd, &sigfd);

					tv.tv_sec = 0;
					tv.tv_usec = 50000;

					if(select(client.fd+1,NULL, &sigfd, NULL, &tv) > 0 && FD_ISSET(client.fd, &sigfd))
						while(sock_write(&client, buffer, &bytes) != 0);

					if(strncmp(buffer, "/quit", 5) == 0) break;

					memset(buffer, 0, strlen(buffer)+1);
				}
			}

		}
		while(1);	
	}

	puts("Closing connection...");
	if(sock_close(&sock) == -1) return EXIT_FAILURE;
	puts("Connection succesfully closed!");

	return EXIT_SUCCESS;
}
