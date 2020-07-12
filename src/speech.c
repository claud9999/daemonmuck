#include "copyright.h"
#include "config.h"

#include "prims.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "params.h"
#include "externs.h"
#include "money.h"
#include <ctype.h>

/* Commands which involve speaking */

int blank(char *s);

void do_say(__DO_PROTO) {
	dbref loc = 0l;

	if ((loc = getloc(player)) == NOTHING)
		return;

	/* notify everybody */
	notify(player, player, "You say, \"%s\"", argall);
	notify_except(player, loc, player, "%s says, \"%s\"", unparse_name(player),
			argall);
}

void do_whisper(__DO_PROTO) {
	dbref who = 0l;
	match_data md;

	init_match(player, arg1, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(player)) {
		match_absolute(&md);
		match_player(&md);
	}
	switch (who = match_result(&md)) {
	case NOTHING:
		notify(player, player, "Whisper to whom?");
		break;
	case AMBIGUOUS:
		notify(player, player, "I don't know who you mean!");
		break;
	default:
		if (!notify(player, who, "%s whispers, \"%s\"", unparse_name(player),
				arg2))
			notify(player, player, "%s is not connected.", unparse_name(who));
		else
			notify(player, player, "You whisper, \"%s\" to %s.", arg2,
					unparse_name(who));
		break;
	}
}

void do_pose(__DO_PROTO) {
	dbref loc = 0l;

	if ((loc = getloc(player)) == NOTHING)
		return;

	notify_except(player, loc, NOTHING, "%s %s", unparse_name(player), argall);
}

void notify_wizards(char *message, ...) {
	va_list args;
	descriptor_data *d = NULL;

	va_start(args, message);

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected && Wizard(d->player) && (FLAGS(d->player) & MUCKER)) {
			vqueue_string(d, message, args);
			queue_write(d, "\r\n", 2);
		}
	}
}

void do_wall(__DO_PROTO) {
	descriptor_data *d = 0l;

	if (!argall || !*argall)
		return;
	if (Wizard(player)) {
		log_status("WALL from %s(%d): %s\n", NAME(player), player, argall);
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected) {
				queue_string(d, "%s shouts, \"%s\"\r\n", unparse_name(player),
						argall);
				queue_write(d, "\r\n", 2);
			}
		}
	} else
		notify(player, player, "Permission denied.");
}

void do_gripe(__DO_PROTO) {
	dbref loc = 0l;

	if (!argall || !*argall)
		return;
	loc = DBFETCH(player)->location;

	if (o_notify_wiz) {
		notify_wizards("GRIPE entered from %s(%ld) in %s(%ld): %s",
				NAME(player), player, NAME(loc), loc, argall);
		log_gripe("GRIPE entered from %s(%ld) in %s(%ld): %s", NAME(player),
				player, NAME(loc), loc, argall);
		notify(player, player, "Your complaint has been duly noted.");
	}
}

/* doesn't really belong here, but I couldn't figure out where else */
void do_page(__DO_PROTO) {
	dbref target = 0l;

	if ((target = lookup_player(arg1)) == NOTHING) {
		notify(player, player, "I don't recognize that name.");
		return;
	}
	if (FLAGS(target) & HAVEN) {
		notify(player, player, "That player does not wish to be disturbed.");
		return;
	}
	if (blank(arg2)) {
		if (notify(player, target,
				"You sense that %s is looking for you in %s", unparse_name(
						player), unparse_name(DBFETCH(player)->location)))
			notify(player, player, "Your message has been sent.");
		else
			notify(player, player, "%s is not connected.", unparse_name(target));
	} else {
		if (notify(player, target, "%s pages from %s: \"%s\"", unparse_name(
				player), unparse_name(DBFETCH(player)->location), arg2))
			notify(player, player, "Your message has been sent.");
		else
			notify(player, player, "%s is not connected.", unparse_name(target));
	}
}

void vspawn_listener(dbref player, dbref source, dbref program, dbref location,
		char *msg, va_list args) {
	frame *fr = NULL;
	if (Typeof(program) != TYPE_PROGRAM)
		return;

	vsnprintf(match_args, BUFFER_LEN, msg, args);

	if ((fr = new_frame(player, program, source, location, 1))) {
		if (o_fast_exits) {
			if (FLAGS(source) & WIZARD) {
				fr->sleeptime = time(NULL);
				fr->status = STATUS_RUN;
			} else {
				fr->status = STATUS_SLEEP;
				fr->sleeptime = time(NULL) + 1;
			}
		} else {
			fr->status = STATUS_SLEEP;
			fr->sleeptime = time(NULL) + 1;
		}
		add_frame(fr);
	}
}

void vlistener_sweep(dbref player, dbref first, dbref location, char *msg, va_list args) {
	int count = 0;

	DOLIST (first, first) {
		if (FLAGS(first) & HAVEN)
			for (count = 0; count < DBFETCH(first)->sp.exit.ndest; count++)
				vspawn_listener(player, first,
						DBFETCH(first)->sp.exit.dest[count], location, msg, args);
	}
}

void listener_sweep(dbref player, dbref first, dbref location, char *msg, ...) {
	va_list args;

	va_start(args, msg);

	vlistener_sweep(player, first, location, msg, args);
}

int vnotify(dbref from, dbref to, char *msg, va_list args) {
	return vnotify_listener(from, to, DBFETCH(from)->location, msg, args);
}

int notify(dbref from, dbref to, char *msg, ...) {
	va_list args;
	va_start(args, msg);
	return vnotify(from, to, msg, args);
}

int vnotify_listener(dbref player, dbref listener, dbref location, char *msg,
		va_list args) {
	char *name = NULL, buf1[BUFFER_LEN], buf2[BUFFER_LEN];
	int retval = 1;

	if ((FLAGS(listener) & QUELL) && Typeof(listener) != TYPE_PLAYER)
		return retval;

	vsnprintf(buf1, BUFFER_LEN - 1, msg, args);
	if (FLAGS(listener) & NOSPOOF) {
		name = unparse_name(OWNER(player));
		if (stringn_compare(name, msg, strlen(name)) && (listener
				!= OWNER(player))) {
			/* uh oh, a spoof! */
			snprintf(buf2, BUFFER_LEN, "[%s] %s",
					unparse_object(listener, OWNER(player)), buf1);
			strcpy(buf1, buf2);
		}
	}
	retval = notify_nolisten(listener, buf1);
	listener_sweep(player, DBFETCH(listener)->exits, location, msg);
	return retval;
}

int notify_listener(dbref player, dbref listener, dbref location, char *msg,
		...) {
	va_list args;
	va_start(args, msg);
	return vnotify_listener(player, listener, location, msg, args);
}

void notify_except(dbref player, dbref location, dbref exception, char *msg,
		...) {
	va_list args;
	dbref first = 0l;

	va_start(args, msg);

	/* notify here... */
	vnotify_listener(player, location, location, msg, args);

	/* notify all objects/players in the player */
	if (DBFETCH(player)->location == location) {
		first = DBFETCH(player)->contents;

		DOLIST (first, first) {
			if (first != exception)
				vnotify(player, first, msg, args);
		}
	}

	/* notify all objects/players in the room */
	first = DBFETCH(location)->contents;

	DOLIST (first, first) {
		if (first != exception)
			vnotify(player, first, msg, args);
	}

	/* do all listen exits here */
	vlistener_sweep(player, DBFETCH(location)->exits, location, msg, args);

	/* do all listen exits on me */
	vlistener_sweep(player, DBFETCH(player)->exits, location, msg, args);
	/* do all listen exits on parents of here */
	first = DBFETCH(location)->location;
	while (first != NOTHING) {
		vlistener_sweep(player, DBFETCH(first)->exits, location, msg, args);
		first = DBFETCH(first)->location;
	}
}

int blank(char *s) {
	while (*s && isspace(*s))
		s++;

	return !(*s);
}

void vnotify_except_nolisten(dbref player, dbref location, dbref exception,
		char *msg, va_list args) {
	dbref first = 0l;

	if (DBFETCH(player)->location == location) {
		first = DBFETCH(player)->contents;
		DOLIST (first, first) {
			if (first != exception)
				vnotify_nolisten(first, msg, args);
		}
	}

	first = DBFETCH(location)->contents;
	DOLIST (first, first) {
		if (first != exception)
			vnotify_nolisten(first, msg, args);
	}
}

void notify_except_nolisten(dbref player, dbref location, dbref exception,
		char *msg, ...) {
	va_list args;
	va_start(args, msg);
	vnotify_except_nolisten(player, location, exception, msg, args);
}
