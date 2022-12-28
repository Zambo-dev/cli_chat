#include <signal.h>
#include "sock.h"
#include "err.h"
#include "conf.h"


pthread_mutex_t sock_mtx = PTHREAD_MUTEX_INITIALIZER;

int sock_init(sock_t *sock)
{
	int retval;

	/* Init socket */
	if ((sock->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		fd_errck("socket");
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
			ssl_errck("SSL_CTX_new", SSL_get_error(sock->ssl, retval));
			return -1;
		}
		/* set the local certificate from CertFile */
		if ((retval = SSL_CTX_use_certificate_file(sock->sslctx, sock->conf.certfile, SSL_FILETYPE_PEM)) <= 0)
		{
			ssl_errck("SSL_CTX_use_certificate_file", SSL_get_error(sock->ssl, retval));
			return -1;
		}
		/* set the private key from KeyFile (may be the same as CertFile) */
		if ((retval = SSL_CTX_use_PrivateKey_file(sock->sslctx, sock->conf.keyfile, SSL_FILETYPE_PEM)) <= 0)
		{
			ssl_errck("SSL_CTX_use_PrivateKey_file", SSL_get_error(sock->ssl, retval));
			return -1;
		}
		/* verify private key */
		if (!(retval = SSL_CTX_check_private_key(sock->sslctx)))
		{
			ssl_errck("SSL_CTX_check_private_key", SSL_get_error(sock->ssl, retval));
			return -1;
		}
	}
	else /* Client CTX */
	{
		if ((sock->sslctx = SSL_CTX_new(TLS_client_method())) == NULL)
		{
			ssl_errck("SSL_CTX_new", SSL_get_error(sock->ssl, retval));
			return -1;
		}
	}

	return 0;
}

int sock_connect(sock_t *sock)
{
	int retval;

	/* Connect to the server */
	socklen_t len = sizeof(sock->host);
	do
	{
		retval = connect(sock->fd, (struct sockaddr *)&sock->host, len);
		if((retval = (fd_errck("connect"))) == -1) return -1;
	}
	while(retval == 1);

	/* Init SSL  */
	if ((sock->ssl = SSL_new(sock->sslctx)) == NULL)
	{
		ssl_errck("SSL_new", SSL_get_error(sock->ssl, retval));
		return -1;
	}
	if ((retval = SSL_set_fd(sock->ssl, sock->fd)) == 0)
	{
		ssl_errck("SSL_set_fd", SSL_get_error(sock->ssl, retval));
		return -1;
	}

	do
	{
		retval = SSL_connect(sock->ssl);
		if((retval = ssl_errck("SSL_connect", SSL_get_error(sock->ssl, retval))) == -1)
			return -1;
		
	}
	while(retval == 1);

	return 0;
}

int sock_listen(sock_t *sock)
{
	bind(sock->fd, (struct sockaddr *)&sock->host, (socklen_t)sizeof(sock->host));
	if(fd_errck("bind") == -1) return -1;

	listen(sock->fd, CONNLIMIT);
	if(fd_errck("listen") == -1) return -1;

	return 0;
}

int sock_accept(sock_t *sock, sock_t *conn)
{
	int retval;
	fd_set readfd;
	struct timeval timer;

	FD_ZERO(&readfd);
	FD_SET(sock->fd, &readfd);

	timer.tv_sec = 0;
	timer.tv_usec = 50000;

	select(sock->fd + 1, &readfd, NULL, NULL, &timer);
	if(fd_errck("select") == -1) return -1;

	if (!FD_ISSET(sock->fd, &readfd)) return 1;	

	socklen_t socklen = (socklen_t)sizeof(sock->host);
	retval = accept(sock->fd, (struct sockaddr *)&sock->host, &socklen);
	if(fd_errck("accept") == -1) return -1;

	conf_init_args(&conn->conf, 'c', sock->conf.port, inet_ntoa(sock->host.sin_addr), NULL, NULL, NULL);
	conn->fd = retval;
	conn->sslctx = sock->sslctx;
	conn->ssl = SSL_new(conn->sslctx);
	SSL_set_fd(conn->ssl, conn->fd);

	retval = SSL_accept(conn->ssl);
	if(ssl_errck("SSL_accept", SSL_get_error(conn->ssl, retval)) == -1) return -1;

	return 0;
}

int sock_write(sock_t *sock, char *buffer, size_t *size)
{
	int retval;
	fd_set writefd;
	struct timeval tv;

	FD_ZERO(&writefd);
	FD_SET(sock->fd, &writefd);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	select(sock->fd + 1, NULL, &writefd, NULL, &tv);
	if(fd_errck("select") == -1) return -1;

	if (!FD_ISSET(sock->fd, &writefd)) return 1;

	retval = SSL_write(sock->ssl, buffer, BUFFERLEN);
	if(ssl_errck("SSL_write", SSL_get_error(sock->ssl, retval)) == -1) return -1;

	*size = retval;

	return 0;
}

int sock_read(sock_t *sock, char *buffer, size_t *size)
{
	int retval;
	char buff[BUFFERLEN] = "";
	fd_set readfd;
	struct timeval timer;

	FD_ZERO(&readfd);
	FD_SET(sock->fd, &readfd);

	timer.tv_sec = 0;
	timer.tv_usec = 50000;

	select(sock->fd + 1, &readfd, NULL, NULL, &timer);
	if(fd_errck("select") == -1) return -1;

	if (!FD_ISSET(sock->fd, &readfd)) return 1;	

	retval = SSL_read(sock->ssl, buff, BUFFERLEN);
	if(ssl_errck("SSL_read", SSL_get_error(sock->ssl, retval)) == -1) return -1;
	

	strncat(buffer, buff, BUFFERLEN);
	memset(buff, 0, BUFFERLEN);

	*size = retval;

	return 0;
}

int sock_close(sock_t *sock)
{
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
	close(sock->fd);
	if(fd_errck("close") == -1) return -1;

	sock->fd = 0;

	return 0;
}
