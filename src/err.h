#ifndef ERR_H
#define ERR_H


void fd_errlog(char *func_name);
void ssl_errlog(char *func_name, int errval);
int fd_errck(char *func_name);
int ssl_errck(char *func_name, int retval);

#endif
