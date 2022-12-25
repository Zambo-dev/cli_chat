#include "err.h"
#include "string.h"


pthread_mutex_t errno_mtx = PTHREAD_MUTEX_INITIALIZER;

/* 
* Check if errno is set and if so,
* print the relative error string
*/
void fd_errck(char *func_name)
{
	/* Lock errno mutex */
	pthread_mutex_lock(&errno_mtx);

	if(errno != 0)
	{	
		/* Print errno's value and string */
		printf("%s -> ERROR %d: %s\n", func_name, errno, strerror(errno));
		fflush(stdout);
		/* Reset errno */
		errno = 0;
	}

	/* Unlock errno mutex */
	pthread_mutex_unlock(&errno_mtx);
}

/* 
* Check ssl return value and
* print the relative error string
*/
void ssl_errck(char *func_name, int retval)
{
	char error[128];
	snprintf(error, 128, "%s -> %s", func_name, ERR_error_string(retval, NULL));
	printf("%s\n", error);
	fflush(stdout);
}
