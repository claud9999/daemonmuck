#include "copyright.h"
#include "config.h"

/* Commands that create new objects */

#include "db.h"
#include "params.h"
#include "interface.h"
#include "externs.h"
#include "match.h"
#include "money.h"
#include <ctype.h>

line *read_program(dbref i);
int link_exit(dbref player, dbref ext, char *dest_name, dbref *dest_list);

/* parse_linkable_dest()
 *
 * A utility for open and link which checks whether a given destination
 * string is valid.  It returns a parsed dbref on success, and NOTHING
 * on failure.
 */

static dbref parse_linkable_dest(dbref player, dbref exit1, char *dest_name) {
	dbref dobj = 0l; /* destination room/player/thing/link */
	match_data md;

	init_match(player, dest_name, NOTYPE, &md);
	match_absolute(&md);
	match_everything(&md);
	match_home(&md);

	if ((dobj = match_result(&md)) == NOTHING || dobj == AMBIGUOUS) {
		notify(player, player, "I couldn't find '%s'.", dest_name);
		return NOTHING;
#ifndef TELEPORT_TO_PLAYER
	}
	if (Typeof(dobj) == TYPE_PLAYER)
	{	notify(player, player, "You can't link to players.  Destination %s ignored.",
				unparse_object(player, dobj));
		return NOTHING;
#endif /* TELEPORT_TO_PLAYER */
	}
	if (!can_link(player, exit1)) {
		notify(player, player, "You can't link that.");
		return NOTHING;
	}
	if (!can_link_to(player, Typeof(exit1), dobj)) {
		notify(player, player, "You can't link to %s.",
				unparse_object(player, dobj));
		return NOTHING;
	} else
		return dobj;
}

/* exit_loop_check()
 *
 * Recursive check for loops in destinations of exits.  Checks to see
 * if any circular references are present in the destination chain.
 * Returns 1 if circular reference found, 0 if not.
 */
int exit_loop_check(dbref source, dbref dest) {
	int i = 0;

	if (source == dest)
		return 1; /* That's an easy one! */
	if (Typeof(dest) != TYPE_EXIT)
		return 0;
	for (i = 0; i < DBFETCH(dest)->sp.exit.ndest; i++) {
		if ((DBFETCH(dest)->sp.exit.dest)[i] == source)
			return 1; /* Found a loop! */
		if (Typeof((DBFETCH(dest)->sp.exit.dest)[i]) == TYPE_EXIT) {
			if (exit_loop_check(source, (DBFETCH(dest)->sp.exit.dest)[i]))
				return 1; /* Found one recursively */
		}
	}
	return 0; /* No loops found */
}

/* use this to create an exit */
void do_open(__DO_PROTO) {
	dbref loc = 0l, ext = 0l;
	dbref good_dest[MAX_LINKS];
	int i = 0l, ndest = 0l;

	if (!Builder(OWNER(player))) {
		notify(player, player,
				"That command is restricted to authorized builders.");
		return;
	}

	if ((loc = getloc(player)) == NOTHING)
		return;

	if (!*arg1) {
		notify(player, player,
				"You must specify a direction or action name to open.");
		return;
	} else if (!ok_name(arg1)) {
		notify(player, player, "That's a strange name for an exit!");
		return;
	}

	if (!controls(player, loc))
		notify(player, player, "Permission denied.");
	else if (!payfor(OWNER(player), EXIT_COST))
		notify(player, player,
				"Sorry, you don't have enough %s to open an exit.", PL_MONEY);
	else {
		/* create the exit */
		ext = new_object();

		/* initialize everything */
		DBSTORE(ext, name, dup_string(arg1));
		DBSTORE(ext, location, loc);
		DBSTORE(ext, owner, OWNER(player));
		add_ownerlist(ext);
		FLAGS(ext) = TYPE_EXIT;
		DBFETCH(ext)->sp.exit.ndest = 0;
		DBFETCH(ext)->sp.exit.dest = NULL;

		/* link it in */
		PUSH(ext, DBFETCH(loc)->exits);DBDIRTY(loc);

		/* and we're done */
		notify(player, player, "Exit opened with number %ld.", ext);

		/* check second arg to see if we should do a link */
		if (*arg2 != '\0') {
			notify(player, player, "Trying to link...");
			ndest = link_exit(player, ext, arg2, good_dest);
			DBFETCH(ext)->sp.exit.ndest = ndest;
			DBFETCH(ext)->sp.exit.dest
					= (dbref *) malloc(sizeof(dbref) * ndest);
			for (i = 0; i < ndest; i++)
				(DBFETCH(ext)->sp.exit.dest)[i] = good_dest[i];
			add_backlinks(ext);
			DBDIRTY(ext);
		}
	}
}

/*
 * link_exit()
 *
 * This routine connects an exit to a bunch of destinations.
 *
 * 'player' contains the player's name.
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of exits.
 *
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.
 *
 */

int link_exit(dbref player, dbref ext, char *dest_name, dbref *dest_list) {
	char *p = NULL, *q = NULL;
	int prdest = 0;
	dbref dest = 0l;
	int ndest = 0;
	char qbuf[BUFFER_LEN];

	while (*dest_name) {
		while (isspace(*dest_name))
			dest_name++; /* skip white space */
		p = dest_name;
		while (*dest_name && (*dest_name != EXIT_DELIMITER))
			dest_name++;
		q = strncpy(qbuf, p, BUFFER_LEN); /* copy word */
		q[(dest_name - p)] = '\0'; /* terminate it */
		if (*dest_name)
			for (dest_name++; *dest_name && isspace(*dest_name); dest_name++)
				;

		if ((dest = parse_linkable_dest(player, ext, q)) == NOTHING)
			continue;

		switch (Typeof(dest)) {
		case TYPE_PLAYER:
		case TYPE_ROOM:
		case TYPE_PROGRAM:
			if (prdest) {
				notify(
						player,
						player,
						"Only one player, room, or program destination allowed. Destination %s ignored.",
						unparse_object(player, dest));
				continue;
			}
			prdest = 1;
		case TYPE_THING:
			dest_list[ndest++] = dest;
			break;
		case TYPE_EXIT:
			if (exit_loop_check(ext, dest)) {
				notify(player, player,
						"Destination %s would create a loop, ignored.",
						unparse_object(player, dest));
				continue;
			}
			dest_list[ndest++] = dest;
		}
		if (dest == HOME)
			notify(player, player, "Linked to HOME.");
		else
			notify(player, player, "Linked to %s.",
					unparse_object(player, dest));
		if (ndest >= MAX_LINKS) {
			notify(player, player, "Too many destinations, rest ignored.");
			break;
		}
	}
	return ndest;
}

/* do_link
 *
 * Use this to link to a room that you own.  It also sets home for
 * objects and things, and drop-to's for rooms.
 * It seizes ownership of an unlinked exit, and costs 1 penny
 * plus a penny transferred to the exit owner if they aren't you
 *
 * All destinations must either be owned by you, or be LINK_OK.
 */

void do_link(__DO_PROTO) {
	dbref thing = 0l, dest = 0l, good_dest[MAX_LINKS];
	match_data md;

	int ndest = 0, i = 0;

	init_match(player, arg1, TYPE_EXIT, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	if (Wizard(player)) {
		match_absolute(&md);
		match_player(&md);
	}

	if ((thing = noisy_match_result(&md)) == NOTHING)
		return;

	switch (Typeof(thing)) {
	case TYPE_EXIT:
		/* we're ok, check the usual stuff */
		if (!controls(player, thing)) {
			notify(player, player, "Permission denied.");
			return;
		}

		if (DBFETCH(thing)->sp.exit.ndest != 0) {
			notify(player, player, "Exit already linked.");
			return;
		}

		/* handle costs */

		/* link has been validated and paid for; do it */
		ndest = link_exit(player, thing, arg2, good_dest);
		if (ndest == 0) {
			notify(player, player, "No destinations linked.");
			if (!Wizard(player))
				DBFETCH(OWNER(player))->pennies += LINK_COST; /* Refund! */
			DBDIRTY(OWNER(player));
			break;
		}
		DBSTORE(thing, sp.exit.ndest, ndest)
		;
		DBSTORE(thing, sp.exit.dest, (dbref *)malloc(sizeof(dbref)*ndest))
		;
		for (i = 0; i < ndest; i++)
			(DBFETCH(thing)->sp.exit.dest)[i] = good_dest[i];
		add_backlinks(thing);
		break;
	case TYPE_THING:
	case TYPE_PROGRAM:
	case TYPE_PLAYER:
		init_match(player, arg2, TYPE_ROOM, &md);
		match_neighbor(&md);
		match_absolute(&md);
		match_here(&md);
		match_me(&md);
		if ((dest = noisy_match_result(&md)) == NOTHING)
			return;
		if (!controls(player, thing) || !can_link_to(player, Typeof(thing),
				dest)) {
			notify(player, player, "Permission denied.");
			return;
		}
		remove_backlinks(thing);
		DBSTORE(thing, link, dest)
		;
		add_backlinks(thing);
		notify(player, player, "Home set.");
		break;
	case TYPE_ROOM: /* room dropto's */
		init_match(player, arg2, TYPE_ROOM, &md);
		match_neighbor(&md);
		match_absolute(&md);
		match_home(&md);
		if ((dest = noisy_match_result(&md)) == NOTHING)
			break;
		if (!controls(player, thing) || !can_link_to(player, Typeof(thing),
				dest) || (thing == dest))
			notify(player, player, "Permission denied.");
		else {
			remove_backlinks(thing);
			DBSTORE(thing, link, dest);
			add_backlinks(thing);
			notify(player, player, "Dropto set.");
		}
		break;
	}DBDIRTY(thing);
	return;
}

/*
 * do_dig
 *
 * Use this to create a room.
 */
void do_dig(__DO_PROTO) {
	dbref room = 0l, parent = 0l;
	match_data md;

	if (!Builder(OWNER(player))) {
		notify(player, player,
				"That command is restricted to authorized builders.");
		return;
	}

	if (*arg1 == '\0') {
		notify(player, player, "You must specify a name for the room.");
		return;
	}

	if (!ok_name(arg1)) {
		notify(player, player, "That's a silly name for a room!");
		return;
	}

	if (!payfor(OWNER(player), ROOM_COST)) {
		notify(player, player,
				"Sorry, you don't have enough %s to dig a room.", PL_MONEY);
		return;
	}

	room = new_object();

	FLAGS(room) = TYPE_ROOM | (FLAGS(player) & JUMP_OK);

	parent = DBFETCH(DBFETCH(player)->location)->location;

	if (*arg2) {
		init_match(player, arg2, TYPE_ROOM, &md);
		match_absolute(&md);
		match_here(&md);
		parent = noisy_match_result(&md);
	}

	if ((parent == NOTHING) || (parent == AMBIGUOUS) || (!can_link_to(player,
			Typeof(room), parent)) || (room == parent))
		parent = GLOBAL_ENVIRONMENT;

	/* Initialize everything */
	DBSTORE(room, name, dup_string(arg1));
	DBSTORE(room, location, parent);
	DBSTORE(room, owner, OWNER(player));
	add_ownerlist(room);
	DBSTORE(room, exits, NOTHING);
	DBSTORE(room, link, NOTHING);
	PUSH(room, DBFETCH(parent)->contents);DBDIRTY(room);DBDIRTY(parent);

	notify(player, player, "%s created with room number %ld, parent %ld.",
			arg1, room, parent);
}

/*
 Use this to create a program.
 First, find a program that matches that name.  If there's one,
 then we put him into edit mode and do it.
 Otherwise, we create a new object for him, and call it a program.
 */
void do_program(__DO_PROTO) {
	dbref i = 0l;
	match_data md;

	if (!Mucker(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, player, "You're no programmer!");
		return;
	}

	if (!*arg1) {
		notify(player, player, "No program name given.");
		return;
	}

	init_match(player, arg1, TYPE_PROGRAM, &md);

	match_possession(&md);
	match_neighbor(&md);
	match_absolute(&md);

	if ((i = match_result(&md)) == NOTHING) {
		char buf[BUFFER_LEN];
		i = new_object();

		DBSTORE(i, name, dup_string(arg1));
		snprintf(buf, BUFFER_LEN, "A scroll containing a spell called %s", arg1);
		DBSTORE(i, desc, dup_string(buf));
		DBSTORE(i, location, player);
		DBSTORE(i, link, player);
		add_backlinks(i);
		DBSTORE(i, owner, player);
		add_ownerlist(i);
		FLAGS(i) = TYPE_PROGRAM;

		DBSTORE(i, sp.program.first, 0);
		DBSTORE(i, sp.program.curr_line, 0);
		DBSTORE(i, sp.program.siz, 0);
		DBSTORE(i, sp.program.code, 0);
		DBSTORE(i, sp.program.start, 0);
		DBSTORE(i, sp.program.editlocks, NULL);

		DBSTORE(player, curr_prog, i);

		PUSH(i, DBFETCH(player)->contents);DBDIRTY(i);DBDIRTY(player);
		notify(player, player, "Program %s created with number %ld.", arg1, i);
		notify(player, player, "Entering editor.");
	} else if (i == AMBIGUOUS) {
		notify(player, player, "I don't know which one you mean!");
		return;
	} else {
		if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
			notify(player, player, "Permission denied!");
			return;
		}

		DBSTORE(i, sp.program.first, read_program(i));
		DBSTORE(player, curr_prog, i);
		notify(player, player, "Entering editor.");
		/* list current line */
		do_list(player, i, 0, 0);
		DBDIRTY(i);
	}
	DBSTORE(i, sp.program.editlocks,
			dbreflist_add(DBFETCH(i)->sp.program.editlocks, player));
	FLAGS(player) |= INTERACTIVE;
	DBDIRTY(player);
}

void do_mlist(__DO_PROTO) {
	match_and_list(player, arg1, arg2);
}

void do_edit(__DO_PROTO) {
	dbref i = 0l;
	match_data md;

	if (!Mucker(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, player, "You're no programmer!");
		return;
	}

	if (!*arg1) {
		notify(player, player, "No program name given.");
		return;
	}

	init_match(player, arg1, TYPE_PROGRAM, &md);

	match_possession(&md);
	match_neighbor(&md);
	match_absolute(&md);

	if ((i = noisy_match_result(&md)) == NOTHING || i == AMBIGUOUS)
		return;

	if ((Typeof(i) != TYPE_PROGRAM) || !controls(player, i)) {
		notify(player, player, "Permission denied!");
		return;
	}

	DBSTORE(i, sp.program.first, read_program(i));
	DBSTORE(player, curr_prog, i);
	notify(player, player, "Entering editor.");
	/* list current line */
	do_list(player, i, 0, 0);
	FLAGS(player) |= INTERACTIVE;
	DBSTORE(i, sp.program.editlocks,
			dbreflist_add(DBFETCH(i)->sp.program.editlocks, player));
}

/*
 * do_create
 *
 * Use this to create an object.
 */
void do_create(__DO_PROTO) {
	dbref thing = 0l;
	int cost = 0;

	if (!Builder(OWNER(player))) {
		notify(player, player,
				"That command is restricted to authorized builders.");
		return;
	}

	if (*arg1 == '\0') {
		notify(player, player, "Create what?");
		return;
	} else if (!ok_name(arg1)) {
		notify(player, player, "That's a silly name for a thing!");
		return;
	}

	cost = atol(arg2);

	if (cost < OBJECT_COST)
		cost = OBJECT_COST;

	if (!payfor(OWNER(player), cost))
		notify(player, player, "Sorry, you don't have enough %s.", PL_MONEY);
	else {
		/* create the object */
		thing = new_object();

		/* initialize everything */
		DBSTORE(thing, name, dup_string(arg1));
		DBSTORE(thing, location, player);
		DBSTORE(thing, owner, OWNER(player));
		add_ownerlist(thing);
		DBSTORE(thing, pennies, OBJECT_ENDOWMENT(cost));
		DBSTORE(thing, exits, NOTHING);
		DBSTORE(thing, link, player);
		add_backlinks(thing);
		FLAGS(thing) = TYPE_THING;

		/* endow the object */
		if (DBFETCH(thing)->pennies > MAX_OBJECT_ENDOWMENT) {
			DBSTORE(thing, pennies, MAX_OBJECT_ENDOWMENT);
		}

		DBSTORE(thing, pennies, cost);

		/* link it in */
		PUSH(thing, DBFETCH(player)->contents);DBDIRTY(player);

		/* and we're done */
		notify(player, player, "%s created with number %ld.", arg1, thing);
		DBDIRTY(thing);
	}
}

/*
 * parse_source()
 *
 * This is a utility used by do_action and do_attach.  It parses
 * the source string into a dbref, and checks to see that it
 * exists.
 *
 * The return value is the dbref of the source, or NOTHING if an
 * error occurs.
 *
 */
dbref parse_source(dbref player, char *source_name) {
	dbref source = 0l;
	match_data md;

	init_match(player, source_name, NOTYPE, &md); /* source type can be any */
	match_neighbor(&md);
	match_me(&md);
	match_here(&md);
	match_possession(&md);
	if (Wizard(player))
		match_absolute(&md);
	source = noisy_match_result(&md);

	if (source == NOTHING)
		return NOTHING;

	/* You can only attach actions to things you control */
	if (!controls(player, source)) {
		notify(player, player, "Permission denied.");
		return NOTHING;
	}
	return source;
}

/*
 * do_action()
 *
 * This routine attaches a new existing action to a source object,
 * where possible.
 * The action will not do anything until it is LINKed.
 *
 */
void do_action(__DO_PROTO) {
	dbref action = 0l, source = 0l;

	if (!Builder(OWNER(player))) {
		notify(player, player,
				"That command is restricted to authorized builders.");
		return;
	}

	if (!*arg1 || !*arg2) {
		notify(player, player,
				"You must specify an action name and a source object.");
		return;
	} else if (!ok_name(arg1)) {
		notify(player, player, "That's a strange name for an action!");
		return;
	}
	if (((source = parse_source(player, arg2)) == NOTHING))
		return;

	action = new_object();

	DBSTORE(action, name, dup_string(arg1));
	DBSTORE(action, location, NOTHING);
	DBSTORE(action, owner, OWNER(player));
	add_ownerlist(action);
	DBSTORE(action, sp.exit.ndest, 0);
	DBSTORE(action, sp.exit.dest, NULL);
	FLAGS(action) = TYPE_EXIT;

	moveto(action, source);
	notify(player, player, "Action created with number %ld and attached.", action);
	DBDIRTY(action);
}

/*
 * do_attach()
 *
 * This routine attaches a previously existing action to a source object.
 * The action will not do anything unless it is LINKed.
 *
 */
void do_attach(__DO_PROTO) {
	dbref action = 0l, source = 0l, loc = 0l; /* player's current location */
	match_data md;

	if ((loc = DBFETCH(player)->location) == NOTHING)
		return;

	if (!Builder(OWNER(player))) {
		notify(player, player,
				"That command is restricted to authorized builders.");
		return;
	}

	if (!*arg1 || !*arg2) {
		notify(player, player,
				"You must specify an action name and a source object.");
		return;
	}

	init_match(player, arg1, TYPE_EXIT, &md);
	match_all_exits(&md);
	if (Wizard(player))
		match_absolute(&md);

	if ((action = noisy_match_result(&md)) == NOTHING)
		return;

	if (Typeof(action) != TYPE_EXIT) {
		notify(player, player, "That's not an action!");
		return;
	} else if (!controls(player, action)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (((source = parse_source(player, arg2)) == NOTHING) || Typeof(source)
			== TYPE_PROGRAM)
		return;

	moveto(action, source);
	notify(player, player, "Action re-attached.");
}
