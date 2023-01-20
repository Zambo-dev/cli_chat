#include "sock.h"


int sock_init(sock_t *sock)
{
	int retval;

	/* Init socket */
	if((sock->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		fd_errlog("socket");
		return -1;
	}

	/* Setup s_host data */
	sock->host.sin_family = AF_INET;
	sock->host.sin_port = htons(sock->conf.port);
	sock->host.sin_addr.s_addr = (sock->conf.type == 's') ? INADDR_ANY : inet_addr(sock->conf.ip);

	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	/* Init SSL context */
	if (sock->conf.type == 's') /* Server CTX */
	{
		if((sock->sslctx = SSL_CTX_new(TLS_server_method())) == NULL)
		{
			ssl_errlog("SSL_new", SSL_get_error(sock->ssl, 0));
			return -1;
		}

		/* set the local certificate from CertFile */
		if((retval = SSL_CTX_use_certificate_file(sock->sslctx, sock->conf.certfile, SSL_FILETYPE_PEM)) != 1)
		{
			ssl_errlog("SSL_CTX_use_certificate_file", SSL_get_error(sock->ssl, retval));
			return -1;
		}

		/* set the private key from KeyFile (may be the same as CertFile) */
		if((retval = SSL_CTX_use_PrivateKey_file(sock->sslctx, sock->conf.keyfile, SSL_FILETYPE_PEM)) != 1)
		{
			ssl_errlog("SSL_CTX_use_PrivateKey_file", SSL_get_error(sock->ssl, retval));
			return -1;
		}

		/* verify private key */
		if((retval = SSL_CTX_check_private_key(sock->sslctx)) != 1)
		{
			ssl_errlog("SSL_CTX_check_private_key", SSL_get_error(sock->ssl, retval));
			return -1;
		}

	}
	else /* Client CTX */
	{
		if((sock->sslctx = SSL_CTX_new(TLS_client_method())) == NULL)
		{
			ssl_errlog("SSL_new", SSL_get_error(sock->ssl, 0));
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
		if(connect(sock->fd, (struct sockaddr *)&sock->host, len) == 0) break;
		if((retval = fd_errck("connect")) == -1) return -1; 
	}
	while(retval == 1);

	/* Init SSL  */
	if((sock->ssl = SSL_new(sock->sslctx)) == NULL)
	{
		ssl_errlog("SSL_new", SSL_get_error(sock->ssl, 0));
		return -1;
	}
	
	if((retval = SSL_set_fd(sock->ssl, sock->fd)) == 0)
	{
		ssl_errlog("SSL_set_fd", SSL_get_error(sock->ssl, retval));
		return -1;
	}

	do
	{
		if((retval = SSL_connect(sock->ssl)) == 1) break;
		if((retval = ssl_errck("SSL_connect", SSL_get_error(sock->ssl, retval))) == -1) return -1;
	}
	while(retval == 1);

	return 0;
}

int sock_listen(sock_t *sock)
{
	if(bind(sock->fd, (struct sockaddr *)&sock->host, (socklen_t)sizeof(sock->host)) == -1)
	{	
		fd_errlog("bind");
		return -1;
	}

	if(listen(sock->fd, CONNLIMIT) == -1)
	{	
		fd_errck("listen");
		return -1;
	}

	return 0;
}

int sock_accept(sock_t *sock, sock_t *conn)
{
	int retval;

	socklen_t socklen = (socklen_t)sizeof(sock->host);
	
	if((retval = accept(sock->fd, (struct sockaddr *)&sock->host, &socklen)) == -1) return -1;

	if(conf_init_args(&conn->conf, 'c', sock->conf.port, inet_ntoa(sock->host.sin_addr), NULL, NULL, NULL) == 1) return -1;
	conn->fd = retval;
	conn->sslctx = sock->sslctx;
	
	if((conn->ssl = SSL_new(conn->sslctx)) == NULL)
	{
		ssl_errlog("SSL_new", SSL_get_error(conn->ssl, 0));
		return -1;
	}
	
	if((retval = SSL_set_fd(conn->ssl, conn->fd)) == 0)
	{
		ssl_errlog("SSL_set_fd", SSL_get_error(conn->ssl, retval));
		return -1;
	}

	do
	{
		if((retval = SSL_accept(conn->ssl)) == 1) break;
		if((retval = ssl_errck("SSL_accept", SSL_get_error(conn->ssl, retval))) == -1) return -1;
	}
	while(retval == 1);

	return 0;
}

int sock_write(sock_t *sock, char *buffer, size_t *size)
{
	int retval;

	if((retval = SSL_write(sock->ssl, size, sizeof(size_t))) <= 0) return -1;

	do
	{
		if((retval = SSL_write(sock->ssl, buffer, *size)) > 0) break;
		if((retval = ssl_errck("SSL_write", SSL_get_error(sock->ssl, retval))) == -1) return -1;
	}
	while(retval == 1);

	return 0;
}

int sock_read(sock_t *sock, char **buffer, size_t *size)
{
	int retval;

	if((retval = SSL_read(sock->ssl, size, sizeof(size_t))) <= 0) return -1;

	*buffer = (*buffer != NULL)
		? (char *)realloc(*buffer, *size)
		: (char *)calloc(*size, 1);

	do
	{
		if((retval = SSL_read(sock->ssl, *buffer, *size)) > 0) break;
		if((retval = ssl_errck("SSL_read", SSL_get_error(sock->ssl, retval))) == -1) return -1;
	}
	while(retval == 1);

	return 0;
}

int sock_close(sock_t *sock)
{
	int retval;

	/* Clear sockaddr_in struct */
	memset(&sock->host, 0, sizeof(struct sockaddr_in));

	/* Shutdown SSL */
	if (sock->conf.type == 'c')
	{
		if((retval = SSL_shutdown(sock->ssl)) == 0) SSL_read(sock->ssl, NULL, 0);
		if((retval = ssl_errck("SSL_shutdown", SSL_get_error(sock->ssl, retval))) == -1) return -1;
		SSL_free(sock->ssl);
	}
	SSL_CTX_free(sock->sslctx);

	/* Close scoket fd */
	close(sock->fd);
	if(fd_errck("close") == -1) return -1;

	return 0;
}
