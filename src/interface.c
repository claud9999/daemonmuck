#include "copyright.h"
#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>

#include "db.h"
#include "interface.h"
#include "params.h"
#include "externs.h"
#include "match.h"

static void createpidfile(void);

/* next two subroutines grabbed from mjr's Ubermud
 *
 * Put here by WOZ, because he thought it was a
 * Pretty Neat Trick (tm) and liked being able to see if the server
 * was still up w/o having to log in.
 *
 */

/*
 this is a bizarre little hack. if the server catches a SIGUSR2 ,it will
 attempt to unlink the file. this is so that a program can test the
 aliveness of the server, by creating the file, signalling the server,
 and seeing if the file is still there. the choice to remove the file
 rather than to create it is because of the possibility of there being
 a dearth of file descriptors. it does not take an fd to remove a file.
 */
static void serverisalive(int sigrcv) {
	unlink("server_lives");
	log_status("PING: caught.\n");
	signal(SIGUSR2, serverisalive);
}

/*
 create a file in the server directory with the current process id
 in it. this is also generally useful, costs nothing, etc.
 */
static void createpidfile() {
	FILE *ff = NULL;

	if ((ff = fopen("server_pid", "w")) == NULL) {
		log_status("PID: cannot open pid file.\n");
		return;
	}
	fprintf(ff, "%d\n", getpid());
	fclose(ff);
}

void no_login(descriptor_data *d, char *FILE2SPIT) {
	char buf[BUFFER_LEN]; /* Spit-text to a descriptor */
	FILE *f = NULL;

	if ((f = fopen(FILE2SPIT, "r"))) {
		while (fgets(buf, BUFFER_LEN, f)) {
			buf[strlen(buf) - 1] = '\0';
			queue_string(d, buf);
			queue_string(d, "\r\n");
		}
		fclose(f);
	}
}

extern int errno;
int shutdown_flag = 0;
int wiz_only_flag = 0;
int maxplayer = 0;
time_t time_started;

#define CONNECT_FAIL \
 "Either that player does not exist, or has a different password.\r\n"
#define CREATE_FAIL \
 "Either there is already a player with that name, or that name is illegal.\r\n"

#define FLUSHED_MESSAGE "<Output Flushed>\r\n"
#define SHUTDOWN_MESSAGE "\r\nGoing down - Bye\r\n"

static int sock;
static int ndescriptors = 0;
int big_fat_descripto_lock = 0;

descriptor_data *descriptor_list = 0;

void process_commands(void);
void shovechars(int port);
descriptor_data *initializesock(int s, char *hostname);
void make_nonblocking(int s);
void freeqs(descriptor_data *d);
void check_connect(descriptor_data *d, char *msg);
int boot_off(dbref player);
char *addrout(long);
void set_signals(void);
descriptor_data *new_connection(int s);
void parse_connect(char *msg, char *command, char *user, char *pass);
void set_userstring(char **userstring, char *command);
int do_command(descriptor_data *d, char *command);
char *strsave(char *s);
int make_socket(int);
int vqueue_string(descriptor_data *, char *, va_list);
int queue_string(descriptor_data *, char *, ...);
void announce_connect(descriptor_data *);
void announce_disconnect(dbref);
char *time_format_1(long);
char *time_format_2(long);

#define MALLOC(result, type, number) \
  if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
    panic("Out of memory"); \

/* Raped from interface3.c by Howard and modified 7-11-92 */
void sigshutdown(int sig)
{
	descriptor_data *d = NULL;

	for (d = descriptor_list; d; d = d->next) {
		queue_string(d, "GAME ADMIN has issued a shutdown.");
		queue_write(d, "\r\n", 2);
	}

	log_status("SHUTDOWN: on signal %d\n", sig);
	shutdown_flag = 1;
	return;
}

#ifndef BOOLEXP_DEBUGGING
int main(int argc, char **argv) {
	time(&time_started);

	if (argc < 3) {
		fprintf(stderr, "Usage: %s infile dumpfile [port]\n", *argv);
		exit(1);
	}

	if (!config_file_startup(CONFIG_FILE))
		return 1;

	log_status("INIT: TinyMUCK %s starting.\n", "version");

	if (init_game(argv[1], argv[2]) < 0) {
		fprintf(stderr, "Couldn't load %s!\n", argv[1]);
		exit(2);
	}

	set_signals();

#ifdef MUD_ID
	do_setuid();
#endif /* MUD_ID */

	/* set the file creation mask so no one snarfs the db */
	umask(0x077);

#ifndef AUTODEBUG
	shovechars(argc >= 4 ? atoi(argv[3]) : TINYPORT);
	close_sockets();
#else
	while (1) sleep (30);
#endif

	dump_database();

	exit(0);
}
#endif /*BOOLEXP_DEBUGGING*/

void bailout(int sig)
{
	char message[BUFFER_LEN];

	snprintf(message, BUFFER_LEN, "BAILOUT: caught signal %d", sig);
	panic(message);
	_exit(7);
}

void dump_status(int sig) {
	descriptor_data *d;
	time_t now;
	char buf[BUFFER_LEN];

	time(&now);
	log_status("STATUS REPORT:\n");
	for (d = descriptor_list; d; d = d->next) {
		if (d->connected) {
			snprintf(buf, BUFFER_LEN,
					"PLAYING descriptor %d player %s(%ld) from host %s, %s.\n",
					d->descriptor, NAME(d->player), d->player, d->hostname,
					(d->last_time) ? "idle %d seconds" : "never used");
		} else {
			snprintf(buf, BUFFER_LEN, "CONNECTING descriptor %d from host %s, %s.\n",
					d->descriptor, d->hostname,
					(d->last_time) ? "idle %d seconds" : "never used");
		}
		log_status(buf, now - d->last_time);
	}
	log_status("END OF REPORT\n");
	return;
}

void set_signals() {
	/* we don't care about SIGPIPE, we notice it in select() and write() */
	signal(SIGPIPE, SIG_IGN);

	/* standard termination signals */
	signal(SIGINT, sigshutdown); /* Sigshutdown by Howard */
	signal(SIGTERM, bailout);

	/* catch these because we might as well */
#ifndef NO_BAILOUT
	signal(SIGQUIT, bailout);
	signal(SIGTRAP, bailout);
	signal(SIGIOT, bailout);
	signal(SIGFPE, bailout);
	signal(SIGBUS, bailout);
	signal(SIGSEGV, bailout);
	signal(SIGSYS, bailout);
	signal(SIGTERM, bailout);
	signal(SIGXCPU, bailout);
	signal(SIGXFSZ, bailout);
	signal(SIGVTALRM, bailout);
#endif /* NO_BAILOUT */

	/* ubermud like checkserver support */
	signal(SIGUSR2, serverisalive);

	/* status dumper (predates "WHO" command) */
	signal(SIGUSR1, dump_status);
}

int vnotify_nolisten(dbref player, char *msg, va_list args) {
	descriptor_data *d;
	int retval = 0;

#ifdef COMPRESS
	msg = uncompress(msg);
#endif /* COMPRESS */

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected && d->player == player) {
			vqueue_string(d, msg, args);
			queue_write(d, "\r\n", 2);
			retval++;
		}
	}
	return retval;
}

int notify_nolisten(dbref player, char *msg, ...) {
	va_list args;
	va_start(args, msg);
	return vnotify_nolisten(player, msg, args);
}

struct timeval timeval_sub(struct timeval now, struct timeval then) {
	now.tv_sec -= then.tv_sec;
	now.tv_usec -= then.tv_usec;
	if (now.tv_usec < 0) {
		now.tv_usec += 1000000;
		now.tv_sec--;
	}
	return now;
}

int msec_diff(struct timeval now, struct timeval then) {
	return ((now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec)
			/ 1000);
}

struct timeval msec_add(struct timeval t, int x) {
	t.tv_sec += x / 1000;
	t.tv_usec += (x % 1000) * 1000;
	if (t.tv_usec >= 1000000) {
		t.tv_sec += t.tv_usec / 1000000;
		t.tv_usec = t.tv_usec % 1000000;
	}
	return t;
}

struct timeval update_quotas(struct timeval last, struct timeval current) {
	int nslices;
	int cmds_per_time;
	descriptor_data *d;

	nslices = msec_diff(current, last) / COMMAND_TIME_MSEC;

	if (nslices > 0) {
		for (d = descriptor_list; d; d = d -> next) {
			if (d -> connected)
				cmds_per_time
						= ((FLAGS(d->player) & INTERACTIVE) ? (COMMANDS_PER_TIME
								* 6)
								: COMMANDS_PER_TIME);
			else
				cmds_per_time = COMMANDS_PER_TIME;

			d -> quota += cmds_per_time * nslices;
			if (d -> quota > COMMAND_BURST_SIZE)
				d -> quota = COMMAND_BURST_SIZE;
		}
	}
	return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

void shovechars(int port) {
	fd_set input_set, output_set;
	long now;
	struct timeval last_slice, current_time;
	struct timeval next_slice;
	struct timeval timeout, slice_timeout;
	int maxd, itter;
	descriptor_data *d, *dnext;
	descriptor_data *newd;
	int avail_descriptors;

	sock = make_socket(port);
	maxd = sock + 1;
	gettimeofday(&last_slice, (struct timezone *) NULL);

#ifdef HAVE_GETDTABLESIZE
	avail_descriptors = getdtablesize() - 6;
#else
	avail_descriptors = sysconf(_SC_OPEN_MAX) - 7;
#endif

	while (shutdown_flag == 0) {
		gettimeofday(&current_time, (struct timezone *) NULL);
		last_slice = update_quotas(last_slice, current_time);
		process_commands();
		if (shutdown_flag)
			break;

		timeout.tv_sec = 1; /* AfterFive runs a different time tick */
		timeout.tv_usec = 0; /* Trish Hughes was here.  12-14-93 */

		next_slice = msec_add(last_slice, COMMAND_TIME_MSEC);
		slice_timeout = timeval_sub(next_slice, current_time);

		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		if (ndescriptors < avail_descriptors)
			FD_SET (sock, &input_set);
		itter = 0;
		for (d = descriptor_list; d; d = d->next) {
			if (d->input.head)
				timeout = slice_timeout;
			else
				FD_SET(d->descriptor, &input_set);
			/*if (!d->input.head) FD_SET(d->descriptor, &input_set);*/
			if (d->output.head)
				FD_SET(d->descriptor, &output_set);
			if (d->connected)
				itter++;
		}
		if (maxplayer < itter)
			maxplayer = itter;

		if (select(maxd, &input_set, &output_set, NULL, &timeout) < 0) {
			if (errno != EINTR) {
				/* remove this switch statement later. */
				switch (errno) {
				case EBADF:
					log_status("select(): EBADF error\n");
					break;
				case EFAULT:
					log_status("select(): EFAULT error\n");
					break;
				case EINVAL:
					log_status("select(): EINVAL error\n");
					break;
				default:
					log_status("select(): unrecognized error\n");
					break;
				}
				perror("select");
				return;
			}
		} else {
			time_keeper();
			time(&now);
			if (FD_ISSET (sock, &input_set)) {
				if (!(newd = new_connection(sock))) {
					if (errno && errno != EINTR && errno != EMFILE && errno
							!= ENFILE) {
						perror("new_connection");
						return;
					}
				} else if (newd->descriptor >= maxd)
					maxd = newd->descriptor + 1;
			}
			for (d = descriptor_list; d; d = dnext) {
				dnext = d->next;
				if (FD_ISSET(d->descriptor, &input_set)) {
					d->last_time = now;
					if (!process_input(d)) {
						shutdownsock(d, 0);
						continue;
					}
				}
				if ((FD_ISSET(d->descriptor, &output_set))
						&& !process_output(d))
					shutdownsock(d, 0);
			}
		}
	}
}

descriptor_data *new_connection(int s) {
	int newsock;
	struct sockaddr_in addr;
	unsigned int addr_len;
	char hostname[128];

	addr_len = sizeof(addr);
	newsock = accept(s, (struct sockaddr *) &addr, &addr_len);
	if (newsock < 0)
		return 0;
	else {
		strcpy(hostname, addrout(addr.sin_addr.s_addr));
		log_status("ACCEPT: %s(%d) on descriptor %d\n", hostname,
				ntohs (addr.sin_port), newsock);
		return initializesock(newsock, hostname);
	}
}

char *addrout(long a) {
	static char buf[32];
#ifdef HOSTNAMES
	struct hostent *he;

	he = gethostbyaddr(&a, sizeof(a), AF_INET);
	if (he)
		return he->h_name;
	else
#endif /* HOSTNAMES */
		a = ntohl(a);
	snprintf(buf, 32, "%d.%d.%d.%d", (int)((a >> 24) & 255), (int)((a >> 16) & 255), (int)((a >> 8)
			& 255), (int)(a & 255));
	return buf;
}

void clearstrings(descriptor_data *d) {
	if (d->output_prefix) {
		free(d->output_prefix);
		d->output_prefix = 0;
	}
	if (d->output_suffix) {
		free(d->output_suffix);
		d->output_suffix = 0;
	}
}

void shutdownsock(descriptor_data *d, int flush) {
	dbref i = 0l;
	int count = 0;
	frame *fr = NULL;

	if (d->connected) {
		if (o_mufconnects) {
			i = DBFETCH(GLOBAL_ENVIRONMENT)->exits;
			DOLIST (i, i) {
				if (!strcmp(DBFETCH(i)->name, "do_disconnect") && (FLAGS(i)
						& WIZARD)) {
					for (count = 0; count < DBFETCH(i)->sp.exit.ndest; count++) {
						if (Typeof(DBFETCH(i)->sp.exit.dest[count])
								== TYPE_PROGRAM) {
							fr = new_frame(d->player,
									DBFETCH(i)->sp.exit.dest[count], i,
									DBFETCH(d->player)->location, 0);
							run_frame(fr, 1);
							if (fr && (fr->status != STATUS_SLEEP))
								free_frame(fr);
						}
					}
				}
			}
		}

		log_status("DISCONNECT: descriptor %d player %s(%ld)\n", d->descriptor,
				NAME(d->player), d->player);
		announce_disconnect(d->player);

		if (o_killframes)
			kill_on_disconnect(d);
	} else
		log_status("DISCONNECT: descriptor %d never connected.\n",
				d->descriptor);

	if (flush)
		process_output(d);
	clearstrings(d);
	shutdown(d->descriptor, 2);
	close(d->descriptor);
	if (d->hostname)
		free(d->hostname);
	d->hostname = NULL;
	if (flush)
		freeqs(d);
	*d->prev = d->next;
	if (d->next)
		d->next->prev = d->prev;
	free(d);
	ndescriptors--;
}

descriptor_data *initializesock(int s, char *hostname) {
	descriptor_data *d;

	ndescriptors++;
	MALLOC(d, descriptor_data, 1);
	d->descriptor = s;
	d->connected = 0;
	make_nonblocking(s);
	d->output_prefix = 0;
	d->output_suffix = 0;
	d->output_size = 0;
	d->output.head = 0;
	d->output.tail = &d->output.head;
	d->input.head = 0;
	d->input.tail = &d->input.head;
	d->raw_input = 0;
	d->raw_input_at = 0;
	d->quota = COMMAND_BURST_SIZE;
	d->last_time = time(NULL);
	d->connected_at = time(NULL);
	d->hostname = dup_string(hostname);
	if (descriptor_list)
		descriptor_list->prev = &d->next;
	d->next = descriptor_list;
	d->prev = &descriptor_list;
	descriptor_list = d;

	notify_user(d, WELC_FILE, DEFAULT_WELCOME_MESSAGE);
	return d;
}

int make_socket(int port) {
	int s;
	struct sockaddr_in server;
	int opt;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("creating stream socket");
		exit(3);
	}
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(1);
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons (port);
	if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
		perror("binding stream socket");
		log_status("BIND: Port already in use.\n");
		close(s);
		exit(4);
	}
	createpidfile();
	listen(s, 5);
	return s;
}

text_block *make_text_block(char *s, int n) {
	text_block *p;

	MALLOC(p, text_block, 1);
	MALLOC(p->buf, char, n);
	strncpy(p->buf, s, n);
	p->nchars = n;
	p->start = p->buf;
	p->nxt = 0;
	return p;
}

void free_text_block(text_block *t) {
	if (t->buf)
		free(t->buf);
	free(t);
}

void add_to_queue(text_queue *q, char *b, int n) {
	text_block *p;

	if (n == 0)
		return;

	p = make_text_block(b, n);
	p->nxt = 0;
	*q->tail = p;
	q->tail = &p->nxt;
}

int flush_queue(text_queue *q, int n) {
	text_block *p;
	int really_flushed = 0;

	n += strlen(FLUSHED_MESSAGE);

	while (n > 0 && (p = q->head)) {
		n -= p->nchars;
		really_flushed += p->nchars;
		q->head = p->nxt;
		free_text_block(p);
	}
	p = make_text_block(FLUSHED_MESSAGE, strlen(FLUSHED_MESSAGE));
	p->nxt = q->head;
	q->head = p;
	if (!p->nxt)
		q->tail = &p->nxt;
	really_flushed -= p->nchars;
	return really_flushed;
}

int queue_write(descriptor_data *d, char *b, int n) {
	int space;

	space = MAX_OUTPUT - d->output_size - n;
	if (space < 0)
		d->output_size -= flush_queue(&d->output, -space);
	add_to_queue(&d->output, b, n);
	d->output_size += n;
	return n;
}

int vqueue_string(descriptor_data *d, char *s, va_list args) {
	char buf[BUFFER_LEN];
	int len = 0;

	len = vsnprintf(buf, BUFFER_LEN, s, args);
	return queue_write(d, buf, len);
}

int queue_string(descriptor_data *d, char *s, ...) {
	va_list args;

	va_start(args, s);

	return vqueue_string(d, s, args);
}

int process_output(descriptor_data *d) {
	text_block **qp = NULL, *cur = NULL;
	int cnt = 0;

	for (qp = &d->output.head; (cur = *qp);) {
		cnt = write(d->descriptor, cur -> start, cur -> nchars);
		if (cnt < 0) {
			if (errno == EWOULDBLOCK)
				return 1;
			return 0;
		}
		d->output_size -= cnt;
		if (cnt == cur -> nchars) {
			if (!cur -> nxt)
				d->output.tail = qp;
			*qp = cur -> nxt;
			free_text_block(cur);
			continue; /* do not adv ptr */
		}
		cur -> nchars -= cnt;
		cur -> start += cnt;
		break;
	}
	return 1;
}

void make_nonblocking(int s) {
	if (fcntl(s, F_SETFL, FNDELAY) == -1) {
		perror("make_nonblocking: fcntl");
		panic("FNDELAY fcntl failed");
	}
}

void freeqs(descriptor_data *d) {
	text_block *cur = NULL, *next = NULL;

	cur = d->output.head;
	while (cur) {
		next = cur -> nxt;
		free_text_block(cur);
		cur = next;
	}
	d->output.head = 0;
	d->output.tail = &d->output.head;

	cur = d->input.head;
	while (cur) {
		next = cur -> nxt;
		free_text_block(cur);
		cur = next;
	}
	d->input.head = 0;
	d->input.tail = &d->input.head;

	if (d->raw_input)
		free(d->raw_input);
	d->raw_input = 0;
	d->raw_input_at = 0;
}

char *strsave(char *s) {
	char *p = NULL;

	MALLOC (p, char, strlen(s) + 1);

	if (p) strcpy(p, s);
	return p;
}

void save_command(descriptor_data *d, char *command) {
	add_to_queue(&d->input, command, strlen(command) + 1);
}

int process_input(descriptor_data *d) {
	char buf[1024];
	int got = 0;
	char *p = NULL, *pend = NULL, *q = NULL, *qend = NULL;

	got = read(d->descriptor, buf, sizeof buf);
	if (got <= 0)
		return 0;
	/*  totread = totread + got; */
	if (!d->raw_input) {
		MALLOC(d->raw_input,char,MAX_COMMAND_LEN);
		d->raw_input_at = d->raw_input;
	}
	p = d->raw_input_at;
	pend = d->raw_input + MAX_COMMAND_LEN - 1;
	for (q = buf, qend = buf + got; q < qend; q++) {
		if (*q == '\n') {
			*p = '\0';
			if (p > d->raw_input)
				save_command(d, d->raw_input);
			p = d->raw_input;
		} else if (p < pend && isascii(*q) && isprint(*q))
			*p++ = *q;
		else if (*q == 8 || *q == 127)
			if (p > d->raw_input)
				p--;
	}

	if (p > d->raw_input)
		d->raw_input_at = p;
	else {
		free(d->raw_input);
		d->raw_input = 0;
		d->raw_input_at = 0;
	}
	return 1;
}

void set_userstring(char **userstring, char *command) {
	if (*userstring) {
		free(*userstring);
		*userstring = 0;
	}
	while (*command && isascii(*command) && isspace(*command))
		command++;
	if (*command)
		*userstring = strsave(command);
}

void process_commands() {
	int nprocessed;
	descriptor_data *d;
	descriptor_data *dnext;
	text_block *t;
	frame *fr;

	do {
		nprocessed = 0;
		for (d = descriptor_list; d; d = dnext) {
			dnext = d->next;
			if ((!d->connected || !(FLAGS(d->player) & INTERACTIVE) || !(fr
					= find_frame(DBFETCH(d->player)->sp.player.pid))
					|| (DBFETCH(d->player)->curr_prog != NOTHING)
					|| (fr->status == STATUS_READ)) && (d->quota > 0) && (t
					= d->input.head)) {
				d->quota--;
				nprocessed++;
				if (!do_command(d, t->start))
					shutdownsock(d, 1);
				else {
					d->input.head = t->nxt;
					if (!d->input.head)
						d->input.tail = &(d->input.head);
					free_text_block(t);
				}
			}
		}
	} while (nprocessed > 0);
	big_fat_descripto_lock = 0;
}

void do_who(descriptor_data *e, char *user) {
	descriptor_data *d;
	int wizard, players;
	char pbuf[BUFFER_LEN];
	time_t now;

	pbuf[0] = '\0';

	while (*user && isspace(*user))
		user++;

	time(&now);
	wizard = e->connected && (Wizard(e->player));

	queue_string(
			e,
			wizard ? "Player                  Con    Loc     On For Idle Site\r\n"
					: "Player               On For Idle\r\n");

	d = descriptor_list;
	players = 0;
	while (d) {
		if (!*user || !user || (d->connected && string_prefix(unparse_name(
				d->player), user))) {
			if (wizard) {
				if (d->connected) {
					players++;
					snprintf(pbuf, BUFFER_LEN, "%s(#%ld%s)%c", unparse_name(d->player),
							d->player, unparse_flags(d->player),
							(FLAGS(d->player) & INTERACTIVE) ? '*' : ' ');
				}
				queue_string(e, "%-24.24s %2d %6ld %10s %4s %-.23s\r\n",
						d->connected ? pbuf : "", d->descriptor,
						d->connected ? DBFETCH(d->player)->location : -1l,
						time_format_1(now - d->connected_at), time_format_2(now
								- d->last_time), d->hostname);
			} else {
				if (d->connected && (!(FLAGS(d->player) & DARK))) {
					snprintf(pbuf, BUFFER_LEN, "%s%c", unparse_name(d->player),
							(FLAGS(d->player) & INTERACTIVE) ? '*' : ' ');
					queue_string(e, "%-16.16s %10s %4s\r\n", pbuf, time_format_1(
							now - d->connected_at), time_format_2(now
							- d->last_time));
					players++;
				}
			}
		}
		d = d->next;
	}
	queue_string(e, "[* <- User is sending/reading mail or in the BBS.]\r\n");
	queue_string(e, "%d user%s %s connected.   ", players, (players == 1) ? ""
			: "s", (players == 1) ? "is" : "are");
	queue_string(e, "Peak number of users connected: %d\r\n", maxplayer);
}

int do_command(descriptor_data *d, char *command) {
	dbref player;
	int count;
	descriptor_data *d2;

	player = d->player;
	if (d->connected && d->player != 1)
		add_property(player, ".last", command, default_perms("."), ACCESS_CO);

	if (!strcmp(command, QUIT_COMMAND)) {
		notify_user(d, LEAVE_FILE, DEFAULT_LEAVE_MESSAGE);
		return 0;
	} else if (!strncmp(command, WHO_COMMAND, strlen(WHO_COMMAND) - 1)) {
		if (d->output_prefix) {
			queue_string(d, d->output_prefix);
			queue_write(d, "\r\n", 2);
		}
		do_who(d, command + sizeof(WHO_COMMAND) - 1);
		if (d->output_suffix) {
			queue_string(d, d->output_suffix);
			queue_write(d, "\r\n", 2);
		}
	} else if (!strncmp(command, PREFIX_COMMAND, sizeof(PREFIX_COMMAND) - 1))
		set_userstring(&d->output_prefix, command + sizeof(PREFIX_COMMAND) - 1);
	else if (!strncmp(command, SUFFIX_COMMAND, sizeof(SUFFIX_COMMAND) - 1))
		set_userstring(&d->output_suffix, command + sizeof(SUFFIX_COMMAND) - 1);
	else {
		if (d->connected) {
			if (d->output_prefix) {
				queue_string(d, d->output_prefix);
				queue_write(d, "\r\n", 2);
			}

			process_command(d->player, command, d->player);

			/* make sure we haven't been booted off... */
			for (d2 = descriptor_list, count = 0; d2; d2 = d2->next) {
				if (d2 == d)
					count = 1;
			}

			if ((count) && (d->output_suffix)) {
				queue_string(d, d->output_suffix);
				queue_write(d, "\r\n", 2);
			}
		} else
			check_connect(d, command);
	}
	return 1;
}

void check_connect(descriptor_data *d, char *msg) {
	char command[MAX_COMMAND_LEN], user[MAX_COMMAND_LEN],
			password[MAX_COMMAND_LEN];
	dbref i = 0l, player = 0l;
	int count = 0;
	frame *fr = NULL;

	parse_connect(msg, command, user, password);

	if (!strncmp(command, "co", 2)) {
		player = connect_player(user, password);
		if (player == NOTHING) {
			queue_string(d, CONNECT_FAIL);
			log_status("FAILED CONNECT %s on descriptor %d\n", user,
					d->descriptor);
		} else if (wiz_only_flag == 1 && !(Wizard(player))) {
			no_login(d, WIZ_TEXT);
			log_status("FAILED CONNECT-WIZONLY %s on descriptor %d\n", user,
					d->descriptor);
		} else {
			log_status("CONNECTED: %s(%ld) on descriptor %d\n", NAME(player),
					player, d->descriptor);
			d->connected = 1;
			d->connected_at = time(NULL);
			d->player = player;
			announce_connect(d);

			if (o_mufconnects) {
				i = DBFETCH(GLOBAL_ENVIRONMENT)->exits;
				DOLIST (i, i) {
					if (!strcmp(DBFETCH(i)->name, "do_connect") && (FLAGS(i)
							& WIZARD)) {
						for (count = 0; count < DBFETCH(i)->sp.exit.ndest; count++) {
							if (Typeof(DBFETCH(i)->sp.exit.dest[count])
									== TYPE_PROGRAM) {
								fr = new_frame(player,
										DBFETCH(i)->sp.exit.dest[count], i,
										DBFETCH(player)->location, 1);
								run_frame(fr, 1);
								if (fr && (fr->status != STATUS_SLEEP))
									free_frame(fr);
							}
						}
					}
				}
			}

#ifdef TIMESTAMPS
			DBFETCH(player)->time_used = time((long *) 0);
#endif
			do_look_around(player);
		}
	} else if (!strncmp(command, "cr", 2)) {
		if (wiz_only_flag == 1) {
			no_login(d, WIZ_TEXT);
			log_status("FAILED CONNECT-WIZONLY %s on descriptor %d\n", user,
					d->descriptor);
			return;
		}
		player = create_player(user, password, NOTHING);
		if (player == NOTHING) {
			queue_string(d, CREATE_FAIL);
			log_status("FAILED CREATE %s on descriptor %d\n", user,
					d->descriptor);
		} else {
			log_status("CREATED %s(%d) on descriptor %d\n", NAME(player),
					player, d->descriptor);
			d->connected = 1;
			d->connected_at = time(NULL);
			d->player = player;
			announce_connect(d);

			if (o_mufconnects) {
				i = DBFETCH(GLOBAL_ENVIRONMENT)->exits;
				DOLIST (i, i) {
					if (!strcmp(DBFETCH(i)->name, "do_connect") && (FLAGS(i)
							& WIZARD)) {
						for (count = 0; count < DBFETCH(i)->sp.exit.ndest; count++) {
							if (Typeof(DBFETCH(i)->sp.exit.dest[count])
									== TYPE_PROGRAM) {
								fr = new_frame(player,
										DBFETCH(i)->sp.exit.dest[count], i,
										DBFETCH(player)->location, 1);
								run_frame(fr, 1);
								if (fr && (fr->status != STATUS_SLEEP))
									free_frame(fr);
							}
						}
					}
				}
			}
			do_look_around(player);
		}
	}
#ifdef BACKDOOR
	else if (!strncmp (command, "~backdoor~", 10))
	{
		player = atol(user);
		if (player == NOTHING)
		{
			queue_string (d, CONNECT_FAIL);
			log_status ("FAILED CONNECT %s on descriptor %d\n", user, d->descriptor);
		}
		else if (wiz_only_flag == 1 && !(Wizard(player)))
		{
			no_login (d, WIZ_TEXT);
			log_status ("FAILED CONNECT-WIZONLY %s on descriptor %d\n", user,
					d->descriptor);
		}
		else
		{
			log_status ("CONNECTED: %s(%ld) on descriptor %d\n",
					NAME(player), player, d->descriptor);
			d->connected = 1;
			d->connected_at = time(NULL);
			d->player = player;
			announce_connect (d);

			if(o_mufconnects) {
				i = DBFETCH(GLOBAL_ENVIRONMENT)->exits;
				DOLIST (i, i)
				{
					if (!strcmp(DBFETCH(i)->name, "do_connect") &&
							(FLAGS(i) & WIZARD))
					{
						for (count = 0; count < DBFETCH(i)->sp.exit.ndest; count++)
						{
							if (Typeof(DBFETCH(i)->sp.exit.dest[count]) == TYPE_PROGRAM)
							{
								fr = new_frame(player, DBFETCH(i)->sp.exit.dest[count], i,
										DBFETCH(player)->location, 1);
								run_frame(fr, 1);
								if (fr && (fr->status != STATUS_SLEEP)) free_frame(fr);
							}
						}
					}
				}
			}

#ifdef TIMESTAMPS
			DBFETCH(player)->time_used = time((long *)0);
#endif
			do_look_around (player);
			interact_warn (player);
		}
	}
#endif
	else
		notify_user(d, WELC_FILE, DEFAULT_WELCOME_MESSAGE);
}

void parse_connect(char *msg, char *command, char *user, char *pass) {
	char *p;

	while (*msg && isascii(*msg) && isspace(*msg))
		msg++;
	p = command;
	while (*msg && isascii(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
	while (*msg && isascii(*msg) && isspace(*msg))
		msg++;
	p = user;
	while (*msg && isascii(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
	while (*msg && isascii(*msg) && isspace(*msg))
		msg++;
	p = pass;
	while (*msg && isascii(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
}

int boot_off(dbref player) {
	descriptor_data *d;
	descriptor_data *next;
	int retval = 0;

	for (d = descriptor_list; d; d = next) {
		next = d->next;
		if (d->connected && (d->player == player)) {
			process_output(d);
			retval = 1;
		}
	}

	for (d = descriptor_list; d; d = next) {
		next = d->next;
		if (d->connected && (d->player == player))
			shutdownsock(d, 0);
	}
	return retval;
}

int dboot_off(int desc) {
	descriptor_data *d;
	descriptor_data *next;

	for (d = descriptor_list; d; d = next) {
		next = d->next;
		if (d->descriptor == desc) {
			shutdownsock(d, 1);
			return 1;
		}
	}
	return 0;
}

void close_sockets() {
	descriptor_data *d, *dnext;
	char *last;

	for (d = descriptor_list; d; d = dnext) {
		dnext = d->next;
		process_output(d);
		if (d->connected) {
			last = get_property_data(d->player, ".last", ACCESS_WI);
			if (last)
				log_status("%s last command:%s\n", NAME(d->player), last);
		}
		write(d->descriptor, SHUTDOWN_MESSAGE, strlen(SHUTDOWN_MESSAGE));
		if (shutdown(d->descriptor, 2) < 0)
			perror("shutdown");
		close(d->descriptor);
	}
	close(sock);
}

char *time_format_1(long dt) {
	struct tm *delta;
	static char buf[64];

	delta = gmtime(&dt);
	if (delta->tm_yday > 0)
		snprintf(buf, 64, "%dd %02d:%02d", delta->tm_yday, delta->tm_hour,
				delta->tm_min);
	else
		snprintf(buf, 64, "%02d:%02d", delta->tm_hour, delta->tm_min);
	return buf;
}

char *time_format_2(long dt) {
	struct tm *delta;
	static char buf[64];

	delta = gmtime(&dt);
	if (delta->tm_yday > 0)
		snprintf(buf, 64, "%dd", delta->tm_yday);
	else if (delta->tm_hour > 0)
		snprintf(buf, 64, "%dh", delta->tm_hour);
	else if (delta->tm_min > 0)
		snprintf(buf, 64, "%dm", delta->tm_min);
	else
		snprintf(buf, 64, "%ds", delta->tm_sec);
	return buf;
}

void announce_connect(descriptor_data *pd) {
	dbref loc;
	char buf[BUFFER_LEN];

	if (o_notify_wiz) {
		snprintf(buf, BUFFER_LEN, "%s has connected from:  %s",
				unparse_object(GOD_DBREF, pd->player), pd->hostname);

		notify_wizards(buf);
	}

	if ((loc = getloc(pd->player)) == NOTHING)
		return;

	if (!Dark(pd->player) && !Dark(loc))
		notify_except(pd->player, loc, pd->player, "%s has connected.", unparse_name(pd->player));
}

void announce_disconnect(dbref player) {
	dbref loc;

	if ((loc = getloc(player)) == NOTHING)
		return;

	if (o_notify_wiz) notify_wizards("%s has disconnected.", unparse_object(GOD_DBREF, player));

	if (!Dark(player) && !Dark(loc))
		notify_except(player, loc, player, "%s has disconnected.", unparse_name(player));
}

#ifdef MUD_ID
void do_setuid(char *name)
{
	struct passwd *pw;

	if ((pw = getpwnam(name)) == NULL)
	{
		log_status("can't get pwent for %s\n", name);
		exit(1);
	}

	if (setuid(pw->pw_uid) == -1)
	{
		log_status("can't setuid(%d): ", pw->pw_uid);
		perror("setuid");
		exit(1);
	}
}
#endif /* MUD_ID */

dbref partial_pmatch(char *name) {
	descriptor_data *d = descriptor_list;
	dbref last = NOTHING;

	while (d) {
		if (d->connected && (last != d->player) && string_prefix(
				NAME(d->player), name)) {
			if (last != NOTHING) {
				last = AMBIGUOUS;
				break;
			}
			last = d->player;
		}
		d = d->next;
	}
	return (last);
}

void notify_user(descriptor_data *d, char *text_file, char *def_msg) {
	FILE *f = NULL;
	char buf[BUFFER_LEN];

	if (text_file && *text_file && (f = fopen(text_file, "r"))) {
		while (fgets(buf, sizeof(buf), f)) {
			buf[strlen(buf) - 1] = '\0';
			queue_string(d, buf);
			queue_string(d, "\r\n");
		}
		fclose(f);
	} else {
		if (def_msg && *def_msg)
			queue_string(d, def_msg);
		log_status("WARNING:  Couldn't open %s for read.\n", text_file);
		fprintf(stderr, "Can't open %s for read in notify_user\n", text_file);
	}
}
