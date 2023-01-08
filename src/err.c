#include <openssl/ssl.h>
#include <openssl/err.h>
#include "err.h"
#include "string.h"


/* 
* Check if errno is set and if so,
* print the relative error string
* return: no error, -1 error
*/
int fd_errck(char *func_name)
{
	int retval;
	switch(errno)
	{
		case 0:
			retval = -1;
			break;

		case 114:
		case EAGAIN:
		case EINPROGRESS:
			retval = 0;
			break;

		default:
			/* Print errno's value and string */
			printf("%s -> ERROR %d: %s\n", func_name, errno, strerror(errno));
			fflush(stdout);
			retval = -1;
			break;
	}

	/* Reset errno */
	errno = 0;

	return retval;
}

/* 
* Check ssl return value and
* print the relative error string
* return: 0 no error, -1 error
*/
int ssl_errck(char *func_name, int errval)
{
	int retval;

	fflush(stdout);
	switch(errval)
	{
		case SSL_ERROR_NONE:
		case SSL_SENT_SHUTDOWN:
			retval = -1;
			break;

		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			retval = 0;
			break;

		default:
			char error[128];
			snprintf(error, 128, "%s -> %s", func_name, ERR_error_string(retval, NULL));
			printf("%s\n", error);
			fflush(stdout);
			retval = -1;
			break;
	}

	return retval;
}
