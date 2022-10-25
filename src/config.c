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

	data = strtok(buffer, "=");
	if((data = strtok(NULL, "\n")))
		strcpy(conf->username, data);

	data = strtok(NULL, "=");
	if((data = strtok(NULL, "\n")))
		strcpy(conf->ip, data);

	data = strtok(NULL, "=");
	if((data = strtok(NULL, "\n")))
	strcpy(conf->port, data);

	data = strtok(NULL, "=");
	if((data = strtok(NULL, "\n")))
		strcpy(conf->certfile, data);

	data = strtok(NULL, "=");
	if((data = strtok(NULL, "\n")))
		strcpy(conf->keyfile, data);

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
