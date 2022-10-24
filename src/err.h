#ifndef ERR_H
#define ERR_H

#include "include.h"


void fd_errck(char *func_name);
void ssl_errck(char *func_name, int retval);

#endif
