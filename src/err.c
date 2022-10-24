#include "err.h"


void fd_errck(char *func_name)
{
	/* Lock errno mutex */
	pthread_mutex_lock(&errno_mtx);
	
	if(errno != 0)
	{	
		/* Print errno's value and string */
		printf("%s -> ERROR %d: %s\n", func_name, errno, strerror(errno));
		/* Reset errno */
		errno = 0;
	}

	fflush(stdout);

	/* Unlock errno mutex */
	pthread_mutex_unlock(&errno_mtx);
}

void ssl_errck(char *func_name, int retval)
{
	char err[BUFFERLEN];
	snprintf(err, BUFFERLEN, "%s -> %s", func_name, ERR_error_string(retval, NULL));
	printf("%s\n", err);
	fflush(stdout);
}
