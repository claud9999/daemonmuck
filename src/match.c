#include "copyright.h"
#include "config.h"
#include "interface.h"

/* Routines for parsing arguments */
#include <ctype.h>

#include "db.h"
#include "params.h"
#include "match.h"
#include "externs.h"

extern char *uppercase, *lowercase;
#define DOWNCASE(x) (lowercase[x])

char match_args[BUFFER_LEN]; /* remaining text */

void init_match(dbref player, char *name, int type, match_data *md) {
	md->exact_match = md->last_match = NOTHING;
	md->match_count = 0;
	md->match_who = player;
	md->match_name = name;
	md->check_keys = 0;
	md->preferred_type = type;
	md->longest_match = 0;
}

void init_match_check_keys(dbref player, char *name, int type, match_data *md) {
	init_match(player, name, type, md);
	md->check_keys = 1;
}

static dbref choose_thing(dbref thing1, dbref thing2, match_data *md) {
	int has1 = 0;
	int has2 = 0;
	int preferred = md->preferred_type;

	if (thing1 == NOTHING)
		return thing2;
	else if (thing2 == NOTHING)
		return thing1;

	if (preferred != NOTYPE) {
		if (Typeof(thing1) == preferred) {
			if (Typeof(thing2) != preferred)
				return thing1;
		} else if (Typeof(thing2) == preferred)
			return thing2;
	}

	if (md->check_keys) {
		has1 = could_doit(md->match_who, thing1);
		has2 = could_doit(md->match_who, thing2);

		if (has1 && !has2)
			return thing1;
		else if (has2 && !has1)
			return thing2;
		/* else fall through */
	}

	return (random() % 2 ? thing1 : thing2);
}

void match_player(match_data *md) {
	dbref match = 0l;
	char *p = NULL;

	if (*(md->match_name) == LOOKUP_TOKEN) {
		for (p = (md->match_name) + 1; isspace(*p); p++)
			;
		if ((match = lookup_player(p)) != NOTHING)
			md->exact_match = match;
	}
}

/* returns nnn if name = #nnn, else NOTHING */
dbref absolute_name(match_data *md) {
	dbref match = 0l;

	if (*(md->match_name) == NUMBER_TOKEN) {
		match = parse_dbref((md->match_name) + 1);
		if (match < 0 || match >= db_top)
			return NOTHING;
		else
			return match;
	} else
		return NOTHING;
}

void match_absolute(match_data *md) {
	dbref match = 0l;

	if ((match = absolute_name(md)) != NOTHING) {
		md->exact_match = match;
	}
}

void match_me(match_data *md) {
	if (!string_compare(md->match_name, "me")) {
		md->exact_match = md->match_who;
	}
}

void match_here(match_data *md) {
	if (!string_compare(md->match_name, "here")
			&& DBFETCH(md->match_who)->location != NOTHING) {
		md->exact_match = DBFETCH(md->match_who)->location;
	}
}

void match_home(match_data *md) {
	if (!string_compare(md->match_name, "home"))
		md->exact_match = HOME;
}

void match_list(dbref first, match_data *md) {
	dbref absolute = 0l;

	absolute = absolute_name(md);
	if (!controls(md->match_who, absolute))
		absolute = NOTHING;

	DOLIST(first, first) {
		if (first == absolute) {
			md->exact_match = first;
			return;
		} else if (!string_compare(NAME(first), md->match_name)) {
			/* if there are multiple exact matches, randomly choose one */
			md->exact_match = choose_thing(md->exact_match, first, md);
		} else if (string_match(NAME(first), md->match_name)) {
			md->last_match = first;
			(md->match_count)++;
		}
	}
}

void match_possession(match_data *md) {
	match_list(DBFETCH(md->match_who)->contents, md);
}

void match_neighbor(match_data *md) {
	dbref loc = 0l;

	if ((loc = DBFETCH(md->match_who)->location) != NOTHING) {
		match_list(DBFETCH(loc)->contents, md);
	}
}

/*
 * match_exits matches a list of exits, starting with 'first'.
 * It is will match exits of players, rooms, or things.
 */
void match_exits(dbref thing, match_data *md) {
	dbref exit1 = 0l, absolute = 0l;
	char *exitname = NULL, *p = NULL;
	int i = 0, exitprog = 0;

	if ((thing == NOTHING) || (DBFETCH(thing)->exits == NOTHING))
		return;
	if ((DBFETCH(md->match_who)->location) == NOTHING)
		return;

	absolute = absolute_name(md); /* parse #nnn entries */
	if (!controls(md->match_who, absolute))
		absolute = NOTHING;

	DOLIST(exit1, DBFETCH(thing)->exits) {
		if (exit1 == absolute) {
			md->exact_match = exit1;
			for (p = (char *) md->match_name; p && (*p != ' '); p++)
				;
			if (p)
				strcpy(match_args, p + 1);
			else
				*match_args = '\0';
			continue;
		}

		if ((Typeof(thing) == TYPE_PLAYER) && !(FLAGS(exit1) & JUMP_OK)
				&& (md->match_who != thing)
				&& (DBFETCH(md->match_who)->location != thing))
			continue;

		exitprog = 0;
		if (DBFETCH(exit1)->sp.exit.dest) {
			for (i = 0; i < DBFETCH(exit1)->sp.exit.ndest; i++) {
				if (Typeof((DBFETCH(exit1)->sp.exit.dest)[i])
						== TYPE_PROGRAM)
					exitprog = 1;
			}
		}
		exitname = NAME(exit1);
		while (*exitname) /* for all exit aliases */
		{
			for (p = (char *) md->match_name; /* check out 1 alias */
			*p && DOWNCASE((int)*p) == DOWNCASE((int)*exitname) && *exitname
					!= EXIT_DELIMITER; p++, exitname++)
				;
			/* did we get a match on this alias? */
			if (*p == '\0' || (*p == ' ' && exitprog)) {
				/* make sure there's nothing afterwards */
				while (isspace(*exitname))
					exitname++;

				if (*exitname == '\0' || *exitname == EXIT_DELIMITER) {
					/* we got a match on this alias */
					if (strlen(md->match_name) - strlen(p) > md->longest_match) {
						md->exact_match = exit1;
						md->longest_match = strlen(md->match_name) - strlen(p);
						if (*p == ' ') {
							strcpy(match_args, p + 1);
						} else {
							*match_args = '\0';
						}
					} else if (strlen(md->match_name) - strlen(p)
							== md->longest_match) {
						md->exact_match = choose_thing(md->exact_match, exit1,
								md);
						if (md->exact_match == exit1) {
							if (*p == ' ') {
								strcpy(match_args, p + 1);
							} else {
								*match_args = '\0';
							}
						}
					}
					goto next_exit;
				}
			}
			/* we didn't get it, go on to next alias */
			while (*exitname && *exitname++ != EXIT_DELIMITER)
				;
			while (isspace(*exitname))
				exitname++;
		} /* end of while alias string matches */
		next_exit: ;
	}
}

/*
 * match_room_exits
 * Matches exits and actions attached to player's current room.
 * Formerly 'match_exit'.
 */
void match_room_exits(dbref loc, match_data *md) {
	if (DBFETCH(loc)->exits != NOTHING)
		match_exits(loc, md);
}

/*
 * match_invobj_actions
 * matches actions attached to objects in inventory
 */
void match_invobj_actions(match_data *md) {
	dbref thing = 0l;

	if (DBFETCH(md->match_who)->contents == NOTHING)
		return;
	DOLIST(thing, DBFETCH(md->match_who)->contents) {
		match_exits(thing, md);
	}
}

/*
 * match_roomobj_actions
 * matches actions attached to objects in the room
 */
void match_roomobj_actions(match_data *md) {
	dbref thing = 0l, loc = 0l;

	if ((loc = DBFETCH(md->match_who)->location) == NOTHING)
		return;
	if (DBFETCH(loc)->contents == NOTHING)
		return;
	DOLIST(thing, DBFETCH(loc)->contents) {
		if (Typeof(thing) != TYPE_ROOM)
			match_exits(thing, md);
	}
}

/*
 * match_player_actions
 * matches actions attached to player
 */
void match_player_actions(match_data *md) {
	if (Typeof(md->match_who) != TYPE_PLAYER)
		return;
	match_exits(md->match_who, md);
}

/*
 * match_all_exits
 * Matches actions on player, objects in room, objects in inventory,
 * and room actions/exits (in reverse order of priority order).
 */
void match_all_exits(match_data *md) {
	dbref loc = 0l;

	strcpy(match_args, "\0");
	if ((loc = DBFETCH(md->match_who)->location) != NOTHING)
		match_room_exits(loc, md);
	if (md->exact_match == NOTHING)
		match_invobj_actions(md);
	if (md->exact_match == NOTHING)
		match_roomobj_actions(md);
	if (md->exact_match == NOTHING)
		match_player_actions(md);
	while (md->exact_match == NOTHING && (loc = DBFETCH(loc)->location)
			!= NOTHING)
		match_room_exits(loc, md);
	if ((md->exact_match == NOTHING) && (Wizard(md->match_who)))
		match_absolute(md);
}

void match_everything(match_data *md) {
	match_all_exits(md);
	match_neighbor(md);
	match_possession(md);
	match_me(md);
	match_here(md);
	match_absolute(md);
	match_player(md);
}

dbref match_result(match_data *md) {
	if (md->exact_match != NOTHING) {
		return (md->exact_match);
	} else {
		switch (md->match_count) {
		case 0:
			return NOTHING;
		case 1:
			return (md->last_match);
		default:
			return AMBIGUOUS;
		}
	}
}

/* use this if you don't care about ambiguity */
dbref last_match_result(match_data *md) {
	if (md->exact_match != NOTHING)
		return (md->exact_match);
	else
		return (md->last_match);
}

dbref noisy_match_result(match_data *md) {
	dbref match = 0l;

	switch (match = match_result(md)) {
	case NOTHING:
		notify(md->match_who, md->match_who, NOMATCH_MESSAGE);
		return NOTHING;
	case AMBIGUOUS:
		notify(md->match_who, md->match_who, AMBIGUOUS_MESSAGE);
		return NOTHING;
	default:
		return match;
	}
}

void match_rmatch(dbref arg1, match_data *md) {
	if (arg1 == NOTHING)
		return;
	match_list(DBFETCH(arg1)->contents, md);
	match_exits(arg1, md);
}
