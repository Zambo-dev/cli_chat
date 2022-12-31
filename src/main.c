#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include.h"
#include "sock.h"
#include "err.h"
#include "conf.h"


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


	if(sock.conf.type == 'c')
	{

		fd_set readfd;
		struct timeval tv;

		sock_init(&sock);
		if(sock_connect(&sock) == -1) return EXIT_FAILURE;
		puts("Connected!");

		char buff[] = "zambo00";
		bytes = strlen(buff)+1;
		if(sock_write(&sock, buff, &bytes) == -1) return EXIT_FAILURE;
		
		char *buffer = NULL;

		do
		{
			if((retval = sock_read(&sock, &buffer, &bytes)) == -1) break;
			if(retval == 0)
			{
				if(strncmp(buffer, "/quit", 5) == 0) break;	
				printf("SERVER: %s", buffer);
				fflush(stdout);
			}

			FD_ZERO(&readfd);
			FD_SET(0, &readfd);

			tv.tv_sec = 0;
			tv.tv_usec = 50000;

			if(select(1, &readfd, NULL, NULL, &tv) == -1) break;
			if(FD_ISSET(0, &readfd))
			{
				buffer = (buffer == NULL)
					? (char *)realloc(buffer, 1024)
					: (char *)calloc(1024, 1);
				retval = read(0, buffer, 1024);
				bytes = strlen(buffer)+1;
				if(sock_write(&sock, buffer, &bytes) == -1) break;
			}
		}
		while(1);
	}
	else
	{
		fd_set readfd;
		struct timeval tv;
		sock_init(&sock);
		if(sock_listen(&sock) == -1) return EXIT_FAILURE;
		sock_t client;

		while((retval = sock_accept(&sock, &client)) == 1);
		if(retval == -1) return EXIT_FAILURE;

		puts("CLIENT:");
		conf_log(&client.conf);

		char *buffer = (char *)calloc(1, 1);

		if((retval = sock_read(&client, &buffer, &bytes)) == -1) return EXIT_FAILURE;
		if(retval != 0) return EXIT_FAILURE;
		strncpy(client.conf.username, buffer, bytes);

		do
		{
			if((retval = sock_read(&client, &buffer, &bytes)) == -1) break;
			if(retval != 0) continue;

			printf("%s: %ld %s", client.conf.username, bytes, buffer);
			
			if(sock_write(&client, buffer, &bytes) == -1) break;

			if(strncmp(buffer, "/quit", 5) == 0) break;
		}
		while(1);	
	}

	if(sock_close(&sock) == -1) return EXIT_FAILURE;
	puts("Connection succesfully closed!");

	return EXIT_SUCCESS;
}
