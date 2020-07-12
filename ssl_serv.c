/* serv.cpp  -  Minimal ssleay server for Unix
   30.9.1996, Sampo Kellomaki <sampo@iki.fi> */


/* mangled to work with SSLeay-0.9.0b and OpenSSL 0.9.2b
   Simplified to be even more minimal
   12/98 - 4/99 Wade Scholine <wades@mail.cybg.com> */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslimits.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <openssl/rsa.h>       /* SSLeay stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <getopt.h>

// default cert and key file names
#define CERTF "./foo-cert.crt"
#define KEYF "./foo-cert.key"


#define CHK_NULL(x) if ((x) == NULL) exit (1)
#define CHK_ERR(err,s) if ((err) == -1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err) == -1) { ERR_print_errors_fp(stderr); exit(2); }

static struct option longopts[] = {
    {"cert", required_argument, NULL, 'c'},
    {"key", required_argument, NULL, 'k'},
    {"front_port", required_argument, NULL, 'f'},
    {"back_port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}
};

#define FD_INACTIVE '\0'
#define FD_FRONT_CONNECTED '\1'
#define FD_FRONT_SSL '\2'
#define FD_BACK_CONNECTED '\10'
#define FD_LISTEN_SOCKET '\20'

int main (int argc, char **argv) {
    char *cert_fname = CERTF, *key_fname = KEYF, opt_ch = '\0';
    int front_port = 1111, back_port = 12345, err = 0, listen_fd = 0;
    struct sockaddr_in sa_front, sa_front_client, sa_back;
    size_t client_len;
    SSL_CTX *ctx;
    char *str;
    char buf[4096];
    SSL_METHOD *meth;
    char active_fds[OPEN_MAX]; // a map of active sockets and their states
    int pair[OPEN_MAX]; // each SSL front-end connection has a pairing back-end client
    int max_fd = 0;
    SSL *ssl[OPEN_MAX];
    int num_clients = 0;
    fd_set readfds;

    memset(active_fds, '\0', sizeof(active_fds));
    memset(ssl, '\0', sizeof(ssl));
    memset(pair, '\0', sizeof(pair));

    while((opt_ch = getopt_long(argc, argv, "c:k:f:p:", longopts, NULL)) != -1) {
	switch(opt_ch) {
	    case 'c':
		cert_fname = strdup(optarg);
		break;
	    case 'k':
		key_fname = strdup(optarg);
		break;
	    case 'f':
		front_port = atoi(optarg);
		break;
	    case 'p':
		back_port = atoi(optarg);
		break;
	}
    }
  
    /* SSL preliminaries. We keep the certificate and key with the context. */

    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    meth = SSLv23_server_method();
    ctx = SSL_CTX_new (meth);
    if (!ctx) {
	ERR_print_errors_fp(stderr);
	exit(2);
    }
  
    if (SSL_CTX_use_certificate_file(ctx, cert_fname, SSL_FILETYPE_PEM) <= 0) {
	ERR_print_errors_fp(stderr);
	exit(3);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key_fname, SSL_FILETYPE_PEM) <= 0) {
	ERR_print_errors_fp(stderr);
	exit(4);
    }

    if (!SSL_CTX_check_private_key(ctx)) {
	fprintf(stderr,"Private key does not match the certificate public key\n");
	exit(5);
    }

    /* Prepare TCP socket for receiving connections */
    CHK_ERR(listen_fd = socket(AF_INET, SOCK_STREAM, 0), "socket");
  
    memset (&sa_front, '\0', sizeof(sa_front));
    sa_front.sin_family = AF_INET;
    sa_front.sin_addr.s_addr = INADDR_ANY;
    sa_front.sin_port = htons(front_port);
  
    CHK_ERR(err = bind(listen_fd, (struct sockaddr*) &sa_front, sizeof (sa_front)), "bind");

    CHK_ERR(err = listen(listen_fd, 5), "listen");

    active_fds[listen_fd] = FD_LISTEN_SOCKET;
    max_fd = listen_fd;
    FD_ZERO(&readfds);
    FD_SET(listen_fd, &readfds);
    while (select(max_fd + 1, &readfds, NULL, NULL, NULL)) {
	int i = 0;
	for(i = 0; i <= max_fd; i++) {
	    if (!active_fds[i]) continue;
	    if (!FD_ISSET(i, &readfds)) continue;
	    if (i == listen_fd) {
		int front_fd = 0, back_fd = 0;

		client_len = sizeof(sa_front_client);
		CHK_ERR(front_fd = accept (listen_fd, (struct sockaddr *)&sa_front_client, (socklen_t *)&client_len), "accept");

		printf ("Connection from %lx, port %x, fd %d\n", sa_front_client.sin_addr.s_addr, sa_front_client.sin_port, front_fd);
		CHK_NULL(ssl[front_fd] = SSL_new(ctx));
		SSL_set_fd(ssl[front_fd], front_fd);

		// should make acceptance a part of the cycle
		CHK_SSL(err = SSL_accept (ssl[front_fd]));
		active_fds[front_fd] = FD_FRONT_SSL;

		if (front_fd > max_fd) max_fd = front_fd;

		back_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		memset(&sa_back, '\0', sizeof(sa_back));
		sa_back.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &sa_back.sin_addr);
		sa_back.sin_port = htons(back_port);
		if (!connect(back_fd, (struct sockaddr *)&sa_back, sizeof(sa_back))) {
		    pair[front_fd] = back_fd;
		    pair[back_fd] = front_fd;
		    active_fds[back_fd] = FD_BACK_CONNECTED;

		    if (back_fd > max_fd) max_fd = back_fd;
		} else {
		    char *msg = "unable to connect to back\n";

		    printf(msg);
		    SSL_write(ssl[front_fd], msg, strlen(msg));

		    close(front_fd);
		    SSL_free(ssl[front_fd]);
		    active_fds[front_fd] = FD_INACTIVE;

		    if (front_fd >= max_fd) while(!active_fds[max_fd]) max_fd--;
		}
	    } else {
		int bytes_read = 0;
		switch(active_fds[i]) {
		    case FD_FRONT_CONNECTED:
			break;
		    case FD_FRONT_SSL:
			bytes_read = SSL_read (ssl[i], buf, sizeof(buf) - 1);
			if (bytes_read > 0) {
			    buf[bytes_read] = '\0';
			    //printf ("front %d -> (%d) '%s'\n", i, bytes_read, buf);
  
			    write(pair[i], buf, bytes_read);
			} else {
			    printf ("front closed %d\n", i);
			    close(i);
			    close(pair[i]);
			    active_fds[pair[i]] = FD_INACTIVE;
			    pair[pair[i]] = 0;
			    pair[i] = 0;
			    SSL_free(ssl[i]);
			    active_fds[i] = FD_INACTIVE;
			    ssl[i] = NULL;

			    // rewind max_fd if needed
			    if (i >= max_fd) while(!active_fds[max_fd]) max_fd--;
			}
			break;
		    case FD_BACK_CONNECTED:
			bytes_read = read(i, buf, sizeof(buf) - 1);
			if (bytes_read > 0) {
			    buf[bytes_read] = '\0';
			    //printf ("back %d -> (%d) '%s'\n", i, bytes_read, buf);

			    CHK_SSL(err = SSL_write(ssl[pair[i]], buf, bytes_read));
			} else {
			    printf ("back closed %d\n", i);
			    close(pair[i]);
			    SSL_free(ssl[pair[i]]);
			    ssl[pair[i]] = 0;
			    active_fds[pair[i]] = FD_INACTIVE;
			    pair[pair[i]] = 0;
			    pair[i] = 0;
			    close(i);
			    active_fds[i] = FD_INACTIVE;

			    // rewind max_fd if needed
			    if (i >= max_fd) while(!active_fds[max_fd]) max_fd--;
			}
			break;
		}

		if (ssl[i]) {
		}
	    }
	}

	FD_ZERO(&readfds);
	for(i = 0; i <= max_fd; i++) {
	    if (active_fds[i]) FD_SET(i, &readfds);
	}
    }

    SSL_CTX_free (ctx);
    close (listen_fd);
}
