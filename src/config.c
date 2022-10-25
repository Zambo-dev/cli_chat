#include "config.h"
#include "err.h"

int conf_load(conf_t *conf, char *filepath)
{
	int fd, retval;
	char buffer[BUFFERLEN] = {0};
	char *data;

	fd = open(filepath, O_RDONLY);
	if(fd == -1)
	{
		fd_errck("open");
		return -1;
	}

	memset(conf, 0, sizeof(conf_t));

	/* Read ip */
	if(read(fd, buffer, BUFFERLEN) == -1)
	{
		fd_errck("read");
		return -1;
	}

	char delim = '"';

	data = strtok(buffer, &delim);
	if((data = strtok(NULL, &delim)))
		strcpy(conf->username, data);

	data = strtok(NULL, &delim);
	if((data = strtok(NULL, &delim)))
		strcpy(conf->ip, data);

	data = strtok(NULL, &delim);
	if((data = strtok(NULL, &delim)))
	strcpy(conf->port, data);

	data = strtok(NULL, &delim);
	if((data = strtok(NULL, &delim)))
	{
		char *sstr;
		if((sstr = strstr(data, "~/")) != NULL)
			sprintf(conf->certfile, "%s/%s", getenv("HOME"), sstr+2);
		else
		strcpy(conf->certfile, data);
	}

	data = strtok(NULL, &delim);
	if((data = strtok(NULL, &delim)))
	{
		char *sstr;
		if((sstr = strstr(data, "~/")) != NULL)
			sprintf(conf->keyfile, "%s/%s", getenv("HOME"), sstr+2);
		else
			strcpy(conf->keyfile, data);
	}

	close(fd);

	conf_log(conf);

	return 0;
}

void conf_log(conf_t *conf)
{
	printf("Config log:\nUsername: %s\nIp: %s\nPort: %s\nCert: %s\nKey: %s\n",
		conf->username,
		conf->ip,
		conf->port,
		conf->certfile,
		conf->keyfile
		);
	fflush(stdout);
}
