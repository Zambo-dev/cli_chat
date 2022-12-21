#include "sock.h"
#include "err.h"

int sock_init(sock_t *sock, char *cert, char *key)
{
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	int retval;

	/* Init socket */
	if((sock->conn.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
		fd_errck("socket");
        /* Unlock scoket mutex */
        pthread_mutex_unlock(&sock_mtx);
        return -1;
    }

	/* Setup s_host data */
	sock->host.sin_family = AF_INET;
	sock->host.sin_port = htons(sock->conn.port);
	sock->host.sin_addr.s_addr = (sock->conn.type == 's') ? INADDR_ANY : inet_addr(sock->conn.ip);

	/* Init SSL context */
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	if(sock->conn.type == 's')			/* Server CTX */
	{
		if((sock->conn.sslctx = SSL_CTX_new(TLS_server_method())) == NULL)
		{
			ssl_errck("SSL_CTX_new", 0);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		/* set the local certificate from CertFile */
		if((retval = SSL_CTX_use_certificate_file(sock->conn.sslctx, cert, SSL_FILETYPE_PEM)) <= 0)
		{
			ssl_errck("SSL_CTX_use_certificate_file", retval);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		/* set the private key from KeyFile (may be the same as CertFile) */
		if((retval = SSL_CTX_use_PrivateKey_file(sock->conn.sslctx, key, SSL_FILETYPE_PEM)) <= 0)
		{
			ssl_errck("SSL_CTX_use_PrivateKey_file", retval);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
		/* verify private key */
		if(!(retval = SSL_CTX_check_private_key(sock->conn.sslctx)))
		{
			ssl_errck("SSL_CTX_check_private_key", retval);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
	}
	else					/* Client CTX */
	{
		if ((sock->conn.sslctx = SSL_CTX_new(TLS_client_method())) == NULL) {
			ssl_errck("SSL_CTX_new", 0);
			/* Unlock socket mutex */
			pthread_mutex_unlock(&sock_mtx);
			return -1;
		}
	}

	/* Print created socket */
	printf("Socked created! Id: %d Ip: %s\n", sock->conn.fd, sock->conn.ip);
	fflush(stdout);

	/* Unlock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}

int sock_close(sock_t *sock)
{	
	/* Lock scoket mutex */
	pthread_mutex_lock(&sock_mtx);

	int fd = sock->conn.fd;

	/* Clear sockaddr_in struct */
	memset(&sock->host, 0, sizeof(struct sockaddr_in));
	

	/* Shutdown SSL */
	if(sock->conn.type == 'c')
	{
		SSL_shutdown(sock->conn.ssl);
		SSL_free(sock->conn.ssl);
	}
	SSL_CTX_free(sock->conn.sslctx);

	/* Close scoket s_fd */
	if(close(sock->conn.fd) == -1)
	{
		fd_errck("close");
		/* Unlock scoket mutex */
		pthread_mutex_unlock(&sock_mtx);
		return -1;
	}
	sock->conn.fd = 0;

	/* Unlock scoket mutex */
	pthread_mutex_unlock(&sock_mtx);
	return 0;
}
