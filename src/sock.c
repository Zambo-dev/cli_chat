#include <signal.h>
#include "sock.h"
#include "err.h"
#include "pthread.h"


pthread_mutex_t sock_mtx = PTHREAD_MUTEX_INITIALIZER;

int sock_init(sock_t *sock)
{
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	int retval;

	/* Init socket */
	if ((sock->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		fd_errck("socket");
		/* Unlock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}

	/* Setup s_host data */
	sock->host.sin_family = AF_INET;
	sock->host.sin_port = htons(sock->conf.port);
	sock->host.sin_addr.s_addr = (sock->conf.type == 's') ? INADDR_ANY : inet_addr(sock->conf.ip);

	/* Init SSL context */
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	if (sock->conf.type == 's') /* Server CTX */
	{
		if ((sock->sslctx = SSL_CTX_new(TLS_server_method())) == NULL)
		{
			ssl_errck("SSL_CTX_new", 0);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		/* set the local certificate from CertFile */
		if ((retval = SSL_CTX_use_certificate_file(sock->sslctx, sock->conf.certfile, SSL_FILETYPE_PEM)) <= 0)
		{
			ssl_errck("SSL_CTX_use_certificate_file", retval);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		/* set the private key from KeyFile (may be the same as CertFile) */
		if ((retval = SSL_CTX_use_PrivateKey_file(sock->sslctx, sock->conf.keyfile, SSL_FILETYPE_PEM)) <= 0)
		{
			ssl_errck("SSL_CTX_use_PrivateKey_file", retval);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		/* verify private key */
		if (!(retval = SSL_CTX_check_private_key(sock->sslctx)))
		{
			ssl_errck("SSL_CTX_check_private_key", retval);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
	}
	else /* Client CTX */
	{
		if ((sock->sslctx = SSL_CTX_new(TLS_client_method())) == NULL)
		{
			ssl_errck("SSL_CTX_new", 0);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
	}

	/* Unlock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_connect(sock_t *sock)
{
	/* Lock socket mutex*/
	pthread_mutex_lock(&sock_mtx);

	int retval;

	/* Connect to the server */
	socklen_t len = sizeof(sock->host);
	do
	{
		retval = connect(sock->fd, (struct sockaddr *)&sock->host, len);
		if(errno != EINPROGRESS)
		{
			fd_errck("connect");
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
	}
	while(retval != 0);

	/* Init SSL  */
	if ((sock->ssl = SSL_new(sock->sslctx)) == NULL)
	{
		ssl_errck("SSL_new", 0);
		/* Unlock socket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	if ((retval = SSL_set_fd(sock->ssl, sock->fd)) == 0)
	{
		ssl_errck("SSL_set_fd", retval);
		/* Unlock socket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}

	do
	{
		if((retval = SSL_connect(sock->ssl)) < 1)
		{
			int err = SSL_get_error(sock->ssl, retval);
			if(err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
			{
				ssl_errck("SSL_connect", retval);
				/* Unlock socket mutex */
				pthread_mutex_unlock(&sock_mtx);
				return -1;
			}
		}
	}
	while(retval != 1);

	/* Unlock socket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_write(sock_t *sock, char *buffer, size_t size)
{
	pthread_mutex_lock(&sock_mtx);

	int retval;
	fd_set writefd;
	struct timeval tv;

	do
	{
		FD_ZERO(&writefd);
		FD_SET(sock->fd, &writefd);

		// Wait 0.05
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if ((retval = select(sock->fd + 1, NULL, &writefd, NULL, &tv)) == -1)
		{
			fd_errck("select");
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}

		if (!FD_ISSET(sock->fd, &writefd)) continue;

		if ((retval = SSL_write(sock->ssl, buffer, BUFFERLEN)) <= 0)
		{
			ssl_errck("SSL_write", retval);
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}

		if (size >= retval)
			size -= retval;
	} while (size >= retval);

	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_read(sock_t *sock, char *buffer, size_t size)
{
	pthread_mutex_lock(&sock_mtx);

	int retval = 0, limit = size / BUFFERLEN;
	char buff[BUFFERLEN];
	fd_set readfd;
	struct timeval tv;

	do
	{
		FD_ZERO(&readfd);
		FD_SET(sock->fd, &readfd);

		// Wait 0.05
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if ((retval = select(sock->fd + 1, &readfd, NULL, NULL, &tv)) == -1)
		{
			fd_errck("select");
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		
		if (!FD_ISSET(sock->fd, &readfd)) continue;

		if ((retval = SSL_read(sock->ssl, buff, BUFFERLEN)) <= 0)
		{
			ssl_errck("SSL_read", retval);
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}

		puts(buff);

		strncat(buffer, buff, BUFFERLEN);
		memset(buff, 0, BUFFERLEN);

		if (size >= retval)
			size -= retval;
	} while (size >= retval && limit > 0);

	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_close(sock_t *sock)
{
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	/* Clear sockaddr_in struct */
	memset(&sock->host, 0, sizeof(struct sockaddr_in));

	/* Shutdown SSL */
	if (sock->conf.type == 'c')
	{
		SSL_shutdown(sock->ssl);
		SSL_free(sock->ssl);
	}
	SSL_CTX_free(sock->sslctx);

	/* Close scoket fd */
	if (close(sock->fd) == -1)
	{
		fd_errck("close");
		/* Unlock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	sock->fd = 0;

	/* Unlock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}
