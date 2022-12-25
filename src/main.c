#include <stdio.h>
#include <stdlib.h>
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
		char *arr[5] = {"U=", "I=", "P=", "C=", "K="};

		while(count < 5)
		{
			retval = parse_args(argc, argv, arr[count]) - 1;
			sprintf(tmp, "%s=%c%s%c\r\n", arr[count], quote, (retval > 0) ? argv[retval]+2 : " ", quote);
			strcat(buffer, tmp);

			++retval;
			++count;
		}

		conf_store(&sock.conf, buffer);

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
	if(sock_connect(&sock) == -1) return EXIT_FAILURE;
	puts("Connected!");

	char buff[BUFFERLEN] = "zambo";
	if(sock_write(&sock, buff, 6) == -1) return EXIT_SUCCESS;
	puts("Data sent!");

	char buffer[1024];
	sock_read(&sock, buffer, 1024);
	puts(buffer);

	sock_close(&sock);

	return EXIT_SUCCESS;
}
