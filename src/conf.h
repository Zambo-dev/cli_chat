#ifndef CONFIG_H
#define CONFIG_H

#include "include.h"

#define DATALEN 256
#define BUFFILE 2048


typedef struct CONF_T
{
	char type;
	short port;
	char ip[16];
	char username[DATALEN];
	char certfile[DATALEN];
	char keyfile[DATALEN];
} conf_t;

int conf_read(conf_t *conf, char *filepath);
int conf_init_buff(conf_t *conf, char *buffer);
int conf_init_args(conf_t *conf, char type, short port, char *ip, char *username, char *certfile, char *keyfile);
int conf_write(conf_t *conf, char *filepath);
void conf_log(conf_t *conf);

#endif
