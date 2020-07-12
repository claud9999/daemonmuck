#include "prims.h"
#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct muf_data {
	int descriptor, connected, port;
	long connected_at, last_time;
	dbref uid;
	char *hostname;
	FILE *fd;
	struct muf_data *next, **prev;
} muf_data;

static int nmufs = 0; /* For tracking MUF TCP sockets */
muf_data *muf_list = 0; /* Init MUF TCP socket linked list */
#define MUF_READ 1
#define MUF_WRITE 0

/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
extern int p_nargs;
extern dbref p_ref;
extern char p_buf[BUFFER_LEN];
static descriptor_data *dd;
extern int errno;

int desc_count() {
	descriptor_data *d;
	int returnval = 0;
	for (d = descriptor_list; d; d = d->next, returnval++)
		;
	return returnval;
}

descriptor_data *desc_num(int n) {
	descriptor_data *return_desc;
	for (return_desc = descriptor_list; return_desc; return_desc
			= return_desc->next)
		if (return_desc->descriptor == n)
			break;
	return return_desc;
}

/****************************************
 * concount ( -- i ) - get number of connections to a MUCK
 ****************************************/
void prims_concount(__P_PROTO) {
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow!");
	p_result = desc_count();
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/****************************************
 * connections ( -- i1...iN N ) - get list of descriptors
 ****************************************/
void prims_connections(__P_PROTO) {
	for (dd = descriptor_list, p_result = 0; dd; dd = dd->next) {
		if (*top >= STACK_SIZE)
			abort_interp("Stack Overflow!");
		if (!dd->connected || (!(FLAGS(dd->player) & DARK) || fr->wizard)) {
			push(arg, top, PROG_INTEGER, MIPSCAST &(dd->descriptor));
			p_result++;
		}
	}
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow!");
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/****************************************
 * condbref ( i -- d ) - get dbref of a certain descriptor
 ****************************************/
void prims_condbref(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Argument not an integer.");
	dd = desc_num(p_oper1->data.number);
	if (!dd)
		abort_interp("Illegal descriptor #.");
	p_ref = (dd->connected) ? dd->player : NOTHING;
	CLEAR(p_oper1);
	push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

/****************************************
 * conidle ( i -- i ) - get idle time for a connection
 ****************************************/
void prims_conidle(__P_PROTO) {
	long now;
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Argument not an integer.");
	p_result = p_oper1->data.number;
	dd = desc_num(p_result);
	if (!dd)
		abort_interp("Incorrect descriptor #.");
	(void) time(&now);
	p_result = now - dd->last_time;
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/****************************************
 * contime ( i -- i ) - get connection time for a descriptor
 ****************************************/
void prims_contime(__P_PROTO) {
	long now;
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Argument not an integer.");
	dd = desc_num(p_oper1->data.number);
	if (!dd)
		abort_interp("Incorrect descriptor #.");
	(void) time(&now);
	p_result = now - dd->connected_at;
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/****************************************
 * conhost ( i -- s ) - get connection site for a descriptor
 ****************************************/
void prims_conhost(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Argument not an integer.");
	dd = desc_num(p_oper1->data.number);
	if (!fr->wizard && (!dd->connected || !controls(fr->euid, dd->player)))
		abort_interp("Permission denied.");
	if (!dd)
		abort_interp("Invalid connection #.");
	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(dd->hostname));
}

/****************************************
 * conboot ( i -- ) - boot off a connection
 ****************************************/
void prims_conboot(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Argument not an integer.");
	dd = desc_num(p_oper1->data.number);
	if (!fr->wizard && (!dd->connected || !controls(fr->euid, dd->player)))
		abort_interp("Permission denied.");
	if (!dd)
		abort_interp("Invalid connection #.");
	process_output(dd);
	shutdownsock(dd, 1);
	CLEAR(p_oper1);
}

/****************************************
 * connotify ( i s -- ) - notify a connection
 ****************************************/
void prims_connotify(__P_PROTO) {
	CHECKOP(2);
	p_oper2 = POP();
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Argument not an integer.");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Argument not an string.");
	dd = desc_num(p_oper1->data.number);
	if (!fr->wizard && (!dd->connected || !controls(fr->euid, dd->player)))
		abort_interp("Permission denied.");
	if (!dd)
		abort_interp("Invalid connection #.");
	queue_string(dd, p_oper2->data.string);
	queue_write(dd, "\r\n", 2);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

/****************************************
 * connnected? ( i -- i ) - returns 1 if descriptor is connected
 ****************************************/
void prims_connected(__P_PROTO) {
	int res = 1;
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	dd = desc_num(p_oper1->data.number);

	if (!dd)
		res = 0;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &res);
}

void prims_awakep(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	for (dd = descriptor_list, p_result = 0; dd; dd = dd->next) {
		if (*top >= STACK_SIZE)
			abort_interp("Stack Overflow!");
		if (dd->connected && (dd->player == p_oper1->data.objref)
				&& (!(FLAGS(dd->player) & DARK) || fr->wizard))
			p_result++;
	}
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_conlast(__P_PROTO) {
	descriptor_data *tempd;
	p_result = -1;

	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid object.");
	if (Typeof(p_oper1->data.objref) != TYPE_PLAYER)
		abort_interp("Non-Player argument.");

	for (tempd = descriptor_list; tempd; tempd = tempd->next)
		if (tempd->connected && tempd->player == p_oper1->data.objref) {
			p_result = tempd->descriptor;
			break;
		}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_pconnectors(__P_PROTO) {
	descriptor_data *tempd;
	int i = 0;
	p_result = -1;

	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid object.");
	if (Typeof(p_oper1->data.objref) != TYPE_PLAYER)
		abort_interp("Non-Player argument.");

	for (tempd = descriptor_list; tempd; tempd = tempd->next) {
		if (tempd->connected && tempd->player == p_oper1->data.objref) {
			p_result = tempd->descriptor;
			push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
			i++;
		}
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &i);
}

muf_data *check_muf_desc(int i) {
	muf_data *d;

	for (d = muf_list; d; d = d->next) {
		if (d->descriptor == i)
			return d;
	}
	return NULL;
}

int close_muf(muf_data *d) {
	int i;
	log_status("CLOSE MUF: descriptor %d\n", d->descriptor);
	i = close(d->descriptor) + 1;
	if (d->hostname)
		free(d->hostname);
	fclose(d->fd);
	*d->prev = d->next;
	if (d->next)
		d->next->prev = d->prev;
	if (d)
		free(d);
	nmufs--;
	return i;
}

int wread(int fd, int iobit) {
	fd_set writebits, readbits;
	struct timeval timer;
	char garbage[1];
	int ret;

	timer.tv_sec = 0;
	timer.tv_usec = 100;
	FD_ZERO(&writebits);
	FD_ZERO(&readbits);
	if (iobit == MUF_WRITE)
		FD_SET(fd, &writebits);
	else if (iobit == MUF_READ)
		FD_SET(fd, &readbits);

	if (select(fd + 1, &readbits, &writebits, (fd_set *) NULL, &timer) < 0)
		if (errno != EINTR)
			panic("Select failed in wread.");

	ret = recv(fd, garbage, 0, MSG_PEEK); /* select() lies on */
	if (ret == -1 && errno != EINTR) /* some machines    */
		return 0;

	if (iobit == MUF_WRITE) {
		if (FD_ISSET(fd, &writebits))
			return 1;
	} else if (iobit == MUF_READ) {
		if (FD_ISSET(fd, &readbits))
			return 1;
	}
	return 0;
}

int get_host_address(char *name, struct in_addr * addr) { /* Taken from Tinytalk version 117 */
	struct hostent *blob;
	union { /* %#@!%!@%#!@ idiot who designed */
		long signed_thingy; /* the inetaddr routine.... */
		unsigned long unsigned_thingy;
	} thingy;

	if (!*name)
		return (0);

	if ((*name >= '0') && (*name <= '9')) { /* IP address. */
		addr->s_addr = inet_addr(name);
		thingy.unsigned_thingy = addr->s_addr;
		if (thingy.signed_thingy == -1) {
			return (0);
		}
	} else { /* Host name. */
		blob = gethostbyname(name);

		if (blob == NULL) {
			return (0);
		}
		bcopy(blob->h_addr, (char *) addr, sizeof(struct in_addr));
	}
	return (1); /* Success. */
}

void prims_socket(__P_PROTO) {
	muf_data *d;
	int s;

	if (!fr->wizard)
		abort_interp("Permission denied.");

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		abort_interp("Socket could not be created.");
	} else {
		nmufs++;
		if (!(d = (muf_data *) malloc(sizeof(muf_data))))
			panic("Out of memory in prims_socket.");
		make_nonblocking(s);
		d->descriptor = s; /* Fill out the structure with info */
		d->connected_at = 0;
		d->port = 0;
		d->last_time = 0;
		d->uid = fr->euid;
		d->hostname = NULL;
		d->fd = fdopen(s, "r");
		if (muf_list)
			muf_list->prev = &d->next;
		d->next = muf_list;
		d->prev = &muf_list;
		muf_list = d;
		p_result = s;
	}
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_connect(__P_PROTO) {
	struct in_addr host_address;
	struct sockaddr_in socket_address;
	muf_data *d;
	int err;

	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper2->data.string)
		abort_interp("NULL string argument (2)");
	if (p_oper1->type != PROG_INTEGER || p_oper3->type != PROG_INTEGER)
		abort_interp("Non integer argument");
	if (!(d = check_muf_desc(p_oper3->data.number)))
		abort_interp("Invalid socket.");
	if (p_oper1->data.number <= 0)
		abort_interp("Invalid port.");
	if (!get_host_address(p_oper2->data.string, &host_address))
		abort_interp("Invalid host address.");

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(p_oper1->data.number);
	bcopy((char *) &host_address, (char *) &socket_address.sin_addr,
			sizeof(struct in_addr));
	err = connect(p_oper3->data.number,
			(const struct sockaddr *) &socket_address,
			(socklen_t) sizeof(struct sockaddr_in));

	if (err < 0 && errno != EINPROGRESS) {
		close_muf(d);
		p_result = 0;
	} else {
		d->connected_at = time(NULL);
		d->hostname = dup_string(p_oper2->data.string);
		d->port = p_oper1->data.number;
		p_result = 1;
	}

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

void prims_close(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if ((d = check_muf_desc(p_oper1->data.number))) {
		p_result = close_muf(d);
	} else
		p_result = -1;

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

void prims_ready_write(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		p_result = -1;
	else {
		p_result = wread(p_oper1->data.number, MUF_WRITE);
	}

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

void prims_ready_read(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		p_result = -1;
	else {
		p_result = wread(p_oper1->data.number, MUF_READ);
	}

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

void prims_socket_read(__P_PROTO) {
	muf_data *d;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument (2)");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Non-integer argument (1)");
	if (p_oper1->data.number <= 0)
		abort_interp("Invalid number of bytes (2)");
	if (!(d = check_muf_desc(p_oper2->data.number))) {
		abort_interp("Invalid socket");
	} else {
		if (!wread(p_oper2->data.number, MUF_READ))
			abort_interp("Socket not ready for read");
		if (p_oper1->data.number > BUFFER_LEN)
			abort_interp("Buffer is too big!");

		for (p_result = 0; p_result < BUFFER_LEN; p_result++)
			p_buf[p_result] = '\0';

		p_result = read(p_oper2->data.number, p_buf, p_oper1->data.number);

		if (p_result == -1)
			close_muf(d);
		else
			d->last_time = time(NULL);
	}
	push(arg, top, PROG_STRING, MIPSCAST &p_buf);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

int write_data(int s, char *buffer, int len);

void prims_socket_write(__P_PROTO) {
	muf_data *d;
	char tmp[BUFFER_LEN + 1];
	char tmp2[BUFFER_LEN + 1];
	char *ptr1, *ptr2;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (1)");
	if (!p_oper1->data.string)
		abort_interp("NULL string argument (2)");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Non-integer argument (2)");
	if (!(d = check_muf_desc(p_oper2->data.number)))
		abort_interp("Invalid socket.");
	if (!wread(p_oper2->data.number, MUF_WRITE))
		abort_interp("Socket not ready for write");

	strncpy(tmp, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
	ptr2 = tmp;
	ptr1 = tmp2;
	while (*ptr2) {
		if (*ptr2 == '%') {
			ptr2++;
			if (*ptr2 == 'r')
				*ptr1++ = '\r';
			else if (*ptr2 == 'n')
				*ptr1++ = '\n';
			else
				*ptr1++ = *ptr2;
		} else
			*ptr1++ = *ptr2;
		ptr2++;
	}
	*ptr1++ = '\0';

	/*     p_result=write(p_oper2->data.number, tmp2, strlen(tmp2)); */
	p_result = write_data(p_oper2->data.number, tmp2, strlen(tmp2));
	if (p_result == -1 && (errno != EWOULDBLOCK))
		close_muf(d);
	else
		d->last_time = time(NULL);

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_socket_last(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		p_result = -1;
	else {
		p_result = d->last_time;
	}

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

void prims_socket_connected_at(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		p_result = -1;
	else {
		p_result = d->connected_at;
	}

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

void prims_socket_connected(__P_PROTO) {
	struct in_addr host_address;
	struct sockaddr_in socket_address;
	int err;
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		abort_interp("Invalid socket");

	if (!get_host_address(d->hostname, &host_address))
		abort_interp("Invalid host address.  THIS SHOULD NOT HAPPEN!");

	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(d->port);
	bcopy((char *) &host_address, (char *) &socket_address.sin_addr,
			sizeof(struct in_addr));
	err = connect(d->descriptor, (const struct sockaddr *)&socket_address, sizeof(struct sockaddr_in));

	if (err < 0) {
		switch (errno) {
		case EALREADY:
		case EINPROGRESS:
		case EINTR:
			p_result = 0;
			break;
		case EISCONN:
			p_result = 1;
			break;
		default:
			close_muf(d);
			p_result = -1;
			break;
		}
	} else
		p_result = 1;

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

void prims_socket_host(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		strcpy(p_buf, "");
	else {
		if (d->hostname)
			strcpy(p_buf, d->hostname);
		else
			strcpy(p_buf, "");
	}

	push(arg, top, PROG_STRING, MIPSCAST &p_buf);
	CLEAR(p_oper1);
}

void prims_socket_fgets(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if (!(d = check_muf_desc(p_oper1->data.number)))
		abort_interp("Invalid descriptor.");

	if (!d->fd)
		abort_interp("Couldn't open stream.");
	if (!fgets(p_buf, sizeof(p_buf), d->fd))
		strcpy(p_buf, "");

	push(arg, top, PROG_STRING, MIPSCAST &p_buf);
}

void prims_is_socket(__P_PROTO) {
	muf_data *d;
	CHECKOP(1);
	p_oper1 = POP();

	p_result = 0;
	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	if ((d = check_muf_desc(p_oper1->data.number)))
		p_result = 1;

	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	CLEAR(p_oper1);
}

int write_data(int s, char *buffer, int len) {
	int numwritten = 0;

	while (len > 0) {
		if ((numwritten = write(s, buffer, len)) == -1) {
			if (errno == EWOULDBLOCK) {
				numwritten = 0;
				/*                wread(d->descriptor); */
			} else
				return -1; /* ACK!  Better be in a child! */
		}
		len -= numwritten;
		buffer += numwritten;
	}
	return numwritten;
}
