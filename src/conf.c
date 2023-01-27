#include "conf.h"


int conf_read(conf_t *conf, char *filepath)
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
	if(read(fd, buffer, BUFFILE) == -1)
	{
		fd_errck("read");
		return -1;
	}

	if(close(fd) == -1)
	{
		fd_errck("close");
		return -1;
	}

	conf_init_buff(conf, buffer);

	return 0;
}

int conf_init_buff(conf_t *conf, char *buffer)
{
	if(conf == NULL) return -1;

	char *delim = "\"";
	char *data, *string;

	string = strdup(buffer);

	data = strtok(string, delim);
	if((data = strtok(NULL, delim)))
		strncpy(&conf->type, data, 1);

	data = strtok(NULL, delim);
	if((data = strtok(NULL, delim)))
		strncpy(conf->username, data, DATALEN);

	data = strtok(NULL, delim);
	if((data = strtok(NULL, delim)))
		strncpy(conf->ip, data, 16);

	data = strtok(NULL, delim);
	if((data = strtok(NULL, delim)))
		conf->port = strtol(data, NULL, 10);

	data = strtok(NULL, delim);
	if((data = strtok(NULL, delim)))
	{
		char *sstr;
		if((sstr = strstr(data, "~/")) != NULL)
			snprintf(conf->certfile, DATALEN, "%s/%s", getenv("HOME"), sstr+2);
		else
			strncpy(conf->certfile, data, DATALEN);
	}

	data = strtok(NULL, delim);
	if((data = strtok(NULL, delim)))
	{
		char *sstr;
		if((sstr = strstr(data, "~/")) != NULL)
			snprintf(conf->keyfile, DATALEN, "%s/%s", getenv("HOME"), sstr+2);
		else
			strncpy(conf->keyfile, data, DATALEN);
	}

	return 0;
}

int conf_init_args(conf_t *conf, char type, short port, char *ip, char *username, char *certfile, char *keyfile)
{
	if(conf == NULL) return -1;

	conf->type = type;
	conf->port = port;
	if(ip != NULL) strncpy(conf->ip, ip, 16);
	if(username != NULL) strncpy(conf->username, username, DATALEN);
	if(certfile != NULL) strncpy(conf->certfile, certfile, DATALEN);
	if(keyfile != NULL) strncpy(conf->keyfile, keyfile, DATALEN);

	return 0;
}

int conf_write(conf_t *conf, char *filepath)
{
	char *sstr;
	if((sstr = strstr(filepath, "~/")) != NULL)
		sprintf(filepath, "%s/%s", getenv("HOME"), sstr+2);

	int fd = open(filepath, O_RDWR | O_CREAT, S_IRWXU);
	char quotes = '"';
	char tmp[BUFFILE] = {0};

	sprintf(tmp, "TYPE=%c%c%c\n", quotes, conf->type, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "USERNAME=%c%s%c\n", quotes, conf->username, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "IP=%c%s%c\n", quotes, conf->ip, quotes);
	write(fd, tmp, strlen(tmp));
	sprintf(tmp, "PORT=%c%d%c\n", quotes, conf->port, quotes);
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
	printf("Config log:\nType: %c\nUsername: %s\nIp: %s\nPort: %d\nCert: %s\nKey: %s\n\n",
		conf->type,
		conf->username,
		conf->ip,
		conf->port,
		conf->certfile,
		conf->keyfile
		);
	fflush(stdout);
}
