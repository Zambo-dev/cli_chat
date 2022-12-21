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
	conf_t conf;
	int retval, is_client;

	if((retval = parse_args(argc, argv, "-t")) == 0)
	{
		puts("Wrong paramenter! -t <s/c>");
		return EXIT_FAILURE;
	}
	/* Check if socket is client or server */
	is_client = (argv[retval][0] == 'c') ? 1 : 0;

	printf("\x1b[2J\x1b[1;1H");
	fflush(stdout);

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

		conf_store(&conf, buffer);

		if((retval = parse_args(argc, argv, "-s")) != 0)
			conf_save(&conf, argv[retval]);
	}
	else if((retval = parse_args(argc, argv, "-l")) == 0)
	{
		if(is_client)
			conf_load(&conf, "client.conf");
		else
			conf_load(&conf, "server.conf");
	}
	else
		conf_load(&conf, argv[retval]);

	conf_log(&conf);

	
	return EXIT_SUCCESS;
}
