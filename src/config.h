#ifndef CONFIG_H
#define CONFIG_H

#include "include.h"

#define DATALEN 128


typedef struct CONF_T
{
	char username[DATALEN];
	char ip[DATALEN];
	char port[DATALEN];
	char certfile[DATALEN];
	char keyfile[DATALEN];
	char is_client;
} conf_t;

int conf_load(conf_t *conf, char *filepath);
int conf_save(conf_t *conf, char *filepath);
void conf_log(conf_t *conf);

#endif
