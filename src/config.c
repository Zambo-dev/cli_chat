#include "config.h"
#include "err.h"

int conf_load(conf_t *conf, char *filepath)
{
	int fd, retval;
	char buffer[BUFFILE] = {0};
	char *data;

	if((fd = open(filepath, O_RDONLY)) == -1)
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

	if(close(fd) == -1)
	{
		fd_errck("close");
		return -1;
	}

	conf_store(conf, buffer);

	return 0;
}

void conf_store(conf_t *conf, char *buffer)
{
	char delim = '"';
	char *data;

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
}

int conf_save(conf_t *conf, char *filepath)
{
	char *sstr;
	if((sstr = strstr(filepath, "~/")) != NULL)
		sprintf(filepath, "%s/%s", getenv("HOME"), sstr+2);

	int fd = open(filepath, O_RDWR | O_CREAT, S_IRWXU);
	char quotes = '"';
	char tmp[BUFFILE] = {0};

	sprintf(tmp, "USERNAME=%c%s%c\n", quotes, conf->username, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "IP=%c%s%c\n", quotes, conf->ip, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "PORT=%c%s%c\n", quotes, conf->port, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "CERT=%c%s%c\n", quotes, conf->certfile, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "KEY=%c%s%c\n", quotes, conf->keyfile, quotes);
	write(fd, tmp, strlen(tmp));

	fsync(fd);

	if(close(fd) == -1)
	{
		fd_errck("close");
		return -1;
	}

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
