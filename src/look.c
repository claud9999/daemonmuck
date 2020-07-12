#include "copyright.h"
#include "config.h"
#include "version.h"

/* commands which look at things */

#include <ctype.h>
#include "db.h"
#include "params.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "money.h"

#define EXEC_SIGNAL '@'    /* Symbol which tells us what we're looking at
                            * is an execution order and not a message.    */

/* prints owner of something */
static void print_owner(dbref player, dbref thing) {
	switch (Typeof(thing)) {
	case TYPE_PLAYER:
		notify(player, player, "%s is a player.", unparse_name(thing));
		break;
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_EXIT:
	case TYPE_PROGRAM:
		notify(player, player, "Owner: %s",
				unparse_object_full(player, OWNER(thing)));
		break;
	case TYPE_GARBAGE:
		notify(player, player, "%s is garbage.", unparse_name(thing));
		break;
	}
}

void exec_or_notify(dbref player, dbref thing, char *message) {
	frame *fr = NULL;
	char *p = NULL;
#ifdef COMPRESS
	p = uncompress(message);
#else /* !COMPRESS */
	p = message;
#endif /* COMPRESS */
	if (*p == EXEC_SIGNAL) {
		long i;
		i = atol(++p);
		for (; *p && !isspace(*p); p++)
			;
		if (*p)
			p++;
		if (i < 0 || i >= db_top || (Typeof(i) != TYPE_PROGRAM)) {
			if (*p)
				notify(player, player, p);
			else
				notify(player, player, "You'll probably want to look again...");
		} else {
			char PREV_match_args[BUFFER_LEN];
			strcpy(PREV_match_args, match_args);
			strcpy(match_args, p);
			fr = new_frame(player, i, thing, DBFETCH(player)->location, 1);
			run_frame(fr, 1);
			if (fr && (fr->status != STATUS_SLEEP))
				free_frame(fr);
			strcpy(match_args, PREV_match_args);
		}
	} else
		notify(player, player, p);
}

static void look_contents(dbref player, dbref loc, char *contents_name) {
	dbref thing = 0l;
	dbref can_see_loc = 0l;
	int titleflag = 0;

	/* check to see if he can see the location */
	can_see_loc = ((!Dark(loc) || controls(player, loc))
#ifdef SILENT_PLAYERS
			&& (!(FLAGS(player) & STICKY))
#endif
			);

	/* check to see if there is anything there */
	DOLIST(thing, DBFETCH(loc)->contents) {
		if ((thing != player) && (Typeof(thing) != TYPE_ROOM) && (can_see(
				player, thing, can_see_loc))) {
			/* something exists!  show him everything */
			if (!titleflag) {
				titleflag = 1;
				notify(player, player, contents_name);
			}
			notify(player, player, unparse_object(player, thing));
		}
	}
}

static void look_simple(dbref player, dbref thing) {
#ifdef TIMESTAMPS
	if (Typeof(thing) != TYPE_PLAYER)
		DBFETCH(thing)->time_used = time((long *) NULL);
#endif

	if (GET_DESC(thing))
		exec_or_notify(player, thing, GET_DESC(thing));
	else
		notify(player, player, "You see nothing special.");
}

void look_room(dbref player, dbref loc) {
	if (can_move(player, "look")) {
		do_move(player, "", "", "look");
		return;
	}

	/* tell him the name, and the number if he can link to it */
	notify(player, player, unparse_object(player, loc));
	{
		if (GET_DESC(loc)) /* tell him the description */
		{
			exec_or_notify(player, loc, GET_DESC(loc));
#ifdef TIMESTAMPS
			DBFETCH(loc)->time_used = time(NULL);
#endif
		}
	}
	/* tell him the appropriate messages if he has the key */
	can_doit(player, loc, 0);
	/* tell him the contents */
	look_contents(player, loc, "Contents:");
}

void do_look_around(dbref player) {
	dbref loc = 0l;

	if ((loc = getloc(player)) == NOTHING)
		return;
	look_room(player, loc);
}

void do_look_at(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	if (*arg1 == '\0') {
		if ((thing = getloc(player)) != NOTHING)
			look_room(player, thing);
	} else {
		/* look at a thing here */
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		if (Wizard(player)) {
			match_absolute(&md);
			match_player(&md);
		}
		match_here(&md);
		match_me(&md);

		if ((thing = noisy_match_result(&md)) != NOTHING) {
			switch (Typeof(thing)) {
			case TYPE_ROOM:
				if (getloc(player) != thing && !can_link_to(player, TYPE_ROOM,
						thing))
					notify(player, player, "Permission denied.");
				else
					look_room(player, thing);
				break;
			default:
				look_simple(player, thing);
				look_contents(player, thing, "Carrying:");
				break;
			}
		}
	}
}

char buf[BUFFER_LEN];
char *flag_description(dbref thing) {
	// No user-defined %s below, so as long as buf is long enough...no risk of overflow
	strcpy(buf, "  Type: ");
	switch (Typeof(thing)) {
	case TYPE_ROOM:
		strcat(buf, "ROOM");
		break;
	case TYPE_EXIT:
		strcat(buf, "EXIT/ACTION");
		break;
	case TYPE_THING:
		strcat(buf, "THING");
		break;
	case TYPE_PLAYER:
		strcat(buf, "PLAYER");
		break;
	case TYPE_PROGRAM:
		strcat(buf, "PROGRAM");
		break;
	case TYPE_GARBAGE:
		strcat(buf, "GARBAGE");
		break;
	default:
		strcat(buf, "***UNKNOWN TYPE***");
		break;
	}

	if (FLAGS(thing) & ~TYPE_MASK) {
		/* print flags */
		strcat(buf, "  Flags:");
		if (FLAGS(thing) & GOD)
			strcat(buf, " GOD");
		if (FLAGS(thing) & WIZARD)
			strcat(buf, " WIZARD");
		if (FLAGS(thing) & QUELL)
			strcat(buf, " QUELL");
		if (FLAGS(thing) & STICKY) {
			if (Typeof(thing) == TYPE_PROGRAM)
				strcat(buf, " SETUID");
			else if (Typeof(thing) == TYPE_PLAYER)
				strcat(buf, " SILENT");
			else
				strcat(buf, " STICKY");
		}
		if (FLAGS(thing) & SAFE)
			strcat(buf, " SAFE");
		if (FLAGS(thing) & AUDIBLE)
			strcat(buf, " AUDIBLE");
		if (FLAGS(thing) & DARK)
			strcat(buf,
					(Typeof(thing) != TYPE_PROGRAM && !Puppet(thing)) ? " DARK"
							: " DEBUGGING");
		if (FLAGS(thing) & LINK_OK)
			strcat(buf, " LINK_OK");
		if (FLAGS(thing) & MUCKER)
			strcat(buf, ((FLAGS(thing) & WIZARD) && (Typeof(thing)
					== TYPE_PLAYER)) ? " MONITOR" : " MUCKER");
		if (FLAGS(thing) & BUILDER)
			strcat(buf, " BUILDER");
#ifdef PLAYER_CHOWN
		if (FLAGS(thing) & CHOWN_OK)
			strcat(buf, " CHOWN_OK");
#endif /* PLAYER_CHOWN */
		if (FLAGS(thing) & JUMP_OK)
			strcat(buf, " JUMP_OK");
		if (FLAGS(thing) & HAVEN)
			strcat(buf, (Typeof(thing) == TYPE_EXIT) ? " HEAR" : " HAVEN");
		if (FLAGS(thing) & ABODE) {
			if (Typeof(thing) == TYPE_EXIT)
				strcat(buf, " AUTOSTART");
			else if (Typeof(thing) == TYPE_PLAYER)
				strcat(buf, " AUTHOR");
			else
				strcat(buf, " ABODE");
		}
		if (FLAGS(thing) & ENTER_OK)
			strcat(buf, " ENTER_OK");
		if (FLAGS(thing) & NOSPOOF)
			strcat(buf, " NOSPOOF");
		if (FLAGS(thing) & UNFIND)
			strcat(buf, " UNFINDABLE");
		if (FLAGS(thing) & VISUAL)
			strcat(buf, " VISUAL");
		if (FLAGS(thing) & VERBOSE)
			strcat(buf, " VERBOSE");
		if (FLAGS(thing) & INTERACTIVE)
			strcat(buf, " INTERACTIVE");
	}

	return buf;
}

void examine_object(dbref player, dbref thing) {
	int i = 0;

	if (Typeof(thing) == TYPE_GARBAGE) {
		notify(player, player, unparse_object(player, thing));
		return;
	}

	if (!controls(player, thing) && !Visual(thing))
		print_owner(player, thing);
	else
		notify(player, player, unparse_object_full(player, thing));

	if (!controls(player, thing) && !Visual(thing))
		return;

	notify(player, player, "  Location: %s",
			unparse_object_full(player, DBFETCH(thing)->location));

	notify(player, player, "  Owner: %s",
			unparse_object_full(player, OWNER(thing)));

	notify(player, player, "  Money: %ld", DBFETCH(thing)->pennies);
	notify(player, player, flag_description(thing));
	if (GET_DESC(thing))
		notify(player, player, "  Description: %s", GET_DESC(thing));

	notify(player, player, "  Key: %s", unparse_boolexp(player,
			DBFETCH(thing)->key));

	if (GET_SUCC(thing))
		notify(player, player, "  Success: %s", GET_SUCC(thing));

	if (GET_FAIL(thing))
		notify(player, player, "  Fail: %s", GET_FAIL(thing));

	if (GET_DROP(thing))
		notify(player, player, "  Drop: %s", GET_DROP(thing));

	if (GET_OSUCC(thing))
		notify(player, player, "  Osuccess: %s", GET_OSUCC(thing));

	if (GET_OFAIL(thing))
		notify(player, player, "  Ofail: %s", GET_OFAIL(thing));

	if (GET_ODROP(thing))
		notify(player, player, "  Odrop: %s", GET_ODROP(thing));

	if (DBFETCH(thing)->link != NOTHING)
		notify(player, player, "  Link: %s",
				unparse_object_full(player, DBFETCH(thing)->link));

	switch (Typeof(thing)) {
	case TYPE_EXIT:
		/* print destinations */
		if (DBFETCH(thing)->sp.exit.ndest > 0) {
			notify(player, player, "Destinations:");
			for (i = 0; i < DBFETCH(thing)->sp.exit.ndest; i++) {
				notify(player, player, unparse_object_full(player,
						(DBFETCH(thing)->sp.exit.dest)[i]));
			}
		}
		break;
	case TYPE_PROGRAM:
		if (DBFETCH(thing)->sp.program.siz)
			notify(player, player, "Program compiled size: %d",
					DBFETCH(thing)->sp.program.siz);
		else
			notify(player, player, "Program not compiled");
	}
}

void examine_properties(dbref player, dbref thing, char *match, int titleflag) {
	if (!controls(player, thing) && !Visual(thing)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (DBFETCHPROP(thing) || titleflag) {
		notify(player, player, "Properties:");
		notify_propdir(player, thing, match, access_rights(player, thing,
				NOTHING), 0);
	}
}

void examine_contents(dbref player, dbref thing, char *match, int titleflag) {
	dbref content = 0l;

	if (!controls(player, thing) && !Visual(thing)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if ((DBFETCH(thing)->contents != NOTHING) || titleflag) {
		notify(player, player, "Contents:");
		DOLIST(content, DBFETCH(thing)->contents) {
			if (!match || !*match || string_match(NAME(content), match))
				notify(player, player, unparse_object_full(player, content));
		}
	}
}

void examine_exits(dbref player, dbref thing, char *match, int titleflag) {
	dbref exit1 = 0l;

	if (!controls(player, thing) && !Visual(thing)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if ((DBFETCH(thing)->exits != NOTHING) || titleflag) {
		notify(player, player, "Exits:");
		DOLIST(exit1, DBFETCH(thing)->exits) {
			if (!match || !*match || string_match(NAME(exit1), match))
				notify(player, player, unparse_object_full(player, exit1));
		}
	}
}

void do_examine(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	if (*arg1 == '\0') {
		if ((thing = getloc(player)) == NOTHING)
			return;
	} else {
		/* look it up */
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		match_absolute(&md);
		/* only Wizards can examine other players */
		if (Wizard(player))
			match_player(&md);
		match_here(&md);
		match_me(&md);

		/* get result */
		if ((thing = noisy_match_result(&md)) == NOTHING)
			return;
	}

	examine_object(player, thing);
	if (controls(player, thing) || Visual(thing)) {
		examine_properties(player, thing, arg2, 0);
		examine_contents(player, thing, arg2, 0);
		examine_exits(player, thing, arg2, 0);
	}
}

void do_at_examine(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	if (*arg1 == '\0') {
		if ((thing = getloc(player)) == NOTHING)
			return;
	} else {
		/* look it up */
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		match_absolute(&md);
		/* only Wizards can examine other players */
		if (Wizard(player))
			match_player(&md);
		match_here(&md);
		match_me(&md);

		/* get result */
		if ((thing = noisy_match_result(&md)) == NOTHING)
			return;
	}
	examine_object(player, thing);
}

void do_properties(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	if (*arg1 == '\0') {
		if ((thing = getloc(player)) == NOTHING)
			return;
	} else {
		/* look it up */
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		match_absolute(&md);
		/* only Wizards can examine other players */
		if (Wizard(player))
			match_player(&md);
		match_here(&md);
		match_me(&md);

		/* get result */
		if ((thing = noisy_match_result(&md)) == NOTHING)
			return;
	}
	examine_properties(player, thing, arg2, 1);
}

void do_contents(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	if (*arg1 == '\0') {
		if ((thing = getloc(player)) == NOTHING)
			return;
	} else {
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		match_absolute(&md);
		if (Wizard(player))
			match_player(&md);
		match_here(&md);
		match_me(&md);

		/* get result */
		if ((thing = noisy_match_result(&md)) == NOTHING)
			return;
	}
	examine_contents(player, thing, arg2, 1);
}

void do_exits(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	if (*arg1 == '\0') {
		if ((thing = getloc(player)) == NOTHING)
			return;
	} else {
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		match_absolute(&md);
		if (Wizard(player))
			match_player(&md);
		match_here(&md);
		match_me(&md);

		if ((thing = noisy_match_result(&md)) == NOTHING)
			return;
	}
	examine_exits(player, thing, arg2, 1);
}

void do_score(__DO_PROTO) {
	notify(player, player, "You have %ld %s.", DBFETCH(player)->pennies,
			DBFETCH(player)->pennies == 1 ? S_MONEY : PL_MONEY); /* money.h */
}

void do_inventory(__DO_PROTO) {
	dbref thing = 0l;

	if ((thing = DBFETCH(player)->contents) == NOTHING)
		notify(player, player, "You aren't carrying anything.");
	else {
		notify(player, player, "You are carrying:");
		DOLIST(thing, thing) {
			notify(player, player, unparse_object(player, thing));
		}
	}

	do_score(player, "", "", "");
}

void do_find(__DO_PROTO) {
	dbref i = 0l, current_flags = 0l;

	current_flags = FLAGS(player);
	FLAGS(player) &= (~STICKY);

	if (FLAGS(player) & WIZARD) {
		for (i = 0; i < db_top; i++) {
			if ((Typeof(i) != TYPE_GARBAGE) && (!*arg1 || wild_match(arg1,
					NAME(i)) || string_match(NAME(i), arg1)))
				notify(player, player, unparse_object_full(player, i));
		}
	} else {
		for (i = player; i != NOTHING; i = DBFETCH(i)->nextowned) {
			if ((Typeof(i) != TYPE_GARBAGE) && (!*arg1 || wild_match(arg1,
					NAME(i)) || string_match(NAME(i), arg1)))
				notify(player, player, unparse_object_full(player, i));
		}
	}
	notify(player, player, "***End of List***");

	FLAGS(player) = current_flags;
}

void do_owned(__DO_PROTO) {
	dbref obj = 0l, i = 0l;
	match_data md;

	if (strlen(arg1)) {
		init_match(player, arg1, NOTYPE, &md);
		match_all_exits(&md);
		match_neighbor(&md);
		match_possession(&md);
		if (Wizard(player)) {
			match_absolute(&md);
			match_player(&md);
		}
		match_here(&md);
		match_me(&md);

		if (((obj = noisy_match_result(&md)) == AMBIGUOUS) || (obj == NOTHING))
			return;
	} else
		obj = player;

	if (!Wizard(player) && (player != OWNER(obj))) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (Typeof(obj) == TYPE_PLAYER) {
		for (i = obj; i != NOTHING; i = DBFETCH(i)->nextowned) {
			if ((Typeof(i) != TYPE_GARBAGE) && (!strlen(arg2) || string_match(
					DBFETCH(i)->name, arg2)))
				notify(player, player, unparse_object_full(player, i));
		}
		notify(player, player, "***End of List***");
	} else
		notify(player, player, "Object not a player.");
}

void do_trace(__DO_PROTO) {
	dbref thing = 0l;
	int i = 0, depth = 0;
	match_data md;

	depth = atoi(arg2);

	init_match(player, arg1, NOTYPE, &md);
	match_absolute(&md);
	match_here(&md);
	match_me(&md);
	match_neighbor(&md);
	match_possession(&md);
	if ((thing = noisy_match_result(&md)) == NOTHING || thing == AMBIGUOUS)
		return;

	for (i = 0; (!depth || i < depth) && thing != NOTHING; i++) {
		if (controls(player, thing) || can_link_to(player, NOTYPE, thing))
			notify(player, player, unparse_object(player, thing));
		else
			notify(player, player, "**Missing**");
		thing = DBFETCH(thing)->location;
	}
	notify(player, player, "***End of List***");
}

void do_version(__DO_PROTO) {
	char buffer1[BUFFER_LEN];

	snprintf(buffer1, BUFFER_LEN, "This DaemonMUCK is running version %s",
			VERSION);
	notify(player, player, buffer1);
}
