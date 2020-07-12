#include "copyright.h"
#include "config.h"

#include "db.h"
#include "params.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "money.h"

static char buf[BUFFER_LEN];

void moveto(dbref what, dbref where) {
	dbref loc = 0l;

	if (where == HOME)
		where = DBFETCH(what)->link;

	if (parent_loop_check(what, where))
		return;

	if ((loc = DBFETCH(what)->location) != NOTHING) {
		if (Typeof(what) == TYPE_EXIT) {
			DBSTORE(loc, exits, remove_first(DBFETCH(loc)->exits, what));
		} else {
			DBSTORE(loc, contents, remove_first(DBFETCH(loc)->contents, what));
		}
	}

	/* now put what in where */
	if (where != NOTHING) {
		if (Typeof(what) == TYPE_EXIT) {
			PUSH(what, DBFETCH(where)->exits);
		} else {
			PUSH(what, DBFETCH(where)->contents);
		}DBDIRTY(where);
	}
	DBSTORE(what, location, where);
}

dbref reverse(dbref);
void send_contents(dbref loc, dbref dest) {
	dbref first = 0l;
	dbref rest = 0l;

	first = DBFETCH(loc)->contents;
	DBSTORE(loc, contents, NOTHING);

	while (first != NOTHING) {
		rest = DBFETCH(first)->next;
		if ((Typeof(first) != TYPE_THING) && (Typeof(first) != TYPE_PROGRAM))
			moveto(first, loc);
		else
			moveto(first, FLAGS(first) & STICKY ? HOME : dest);
		first = rest;
	}

	DBSTORE(loc, contents, reverse(DBFETCH(loc)->contents));
}

void maybe_dropto(dbref loc, dbref dropto) {
	dbref thing = 0l;

	if (loc == dropto)
		return; /* bizarre special case */

	/* check for players */
	DOLIST(thing, DBFETCH(loc)->contents) {
		if (Typeof(thing) == TYPE_PLAYER)
			return;
	}

	/* no players, send everything to the dropto */
	send_contents(loc, dropto);
}

int parent_loop_check(dbref source, dbref dest) {
	dbref obj = 0l;
	int count = 0;
	if (dest == NOTHING)
		return 0;
	for (obj = dest; (obj != NOTHING) && (obj != source) && (count < 10000); obj
			= DBFETCH(obj)->location, count++)
		;
	return (obj != NOTHING);
}

void enter_room(dbref player, dbref loc, dbref exit1) {
	dbref old = 0l;
	dbref dropto = 0l;

	/* check for room == HOME */
	if (loc == HOME)
		loc = DBFETCH(player)->link; /* home */

	/* get old location */
	old = DBFETCH(player)->location;

	/* check for self-loop */
	/* self-loops don't do move or other player notification */
	/* but you still get autolook and penny check */
	if (loc != old) {
		if (old != NOTHING) {
			/* notify others unless DARK */
			if ((!Dark(old) && !Dark(player) && (Typeof(exit1) != TYPE_EXIT))
					|| ((Typeof(exit1) == TYPE_EXIT) && !Dark(exit1)
							&& !(GET_OSUCC(exit1))))
				notify_except(player, old, player, "%s has left.", unparse_name(player));
		}

		/* go there */
		moveto(player, loc);

		/* if old location has STICKY dropto, send stuff through it */
		if (old != NOTHING && (dropto = DBFETCH(old)->link) != NOTHING
				&& (FLAGS(old) & STICKY))
			maybe_dropto(old, dropto);

		/* tell other folks in new location if not DARK */
		if ((!Dark(loc) && !Dark(player) && (Typeof(exit1) != TYPE_EXIT))
				|| ((Typeof(exit1) == TYPE_EXIT) && !Dark(exit1)
						&& !(GET_ODROP(exit1))))
			notify_except(player, loc, player, "%s has arrived.", unparse_name(player));
	}

	/* autolook */
	look_room(player, loc);

	if (o_penny_rate) {
		/* check for pennies */
		if (!controls(player, loc) && DBFETCH(player)->pennies <= MAX_PENNIES
				&& random() % PENNY_RATE == 0) {
			notify(player, player, "You found a %s!", S_MONEY);
			DBFETCH(player)->pennies++;
			DBDIRTY(player);
		}
	}
}

void send_home(dbref thing) {
	switch (Typeof(thing)) {
	case TYPE_PLAYER:
		/* send his possessions home first! */
		/* that way he sees them when he arrives */
		send_contents(thing, HOME);
		enter_room(thing, DBFETCH(thing)->link, DBFETCH(thing)->location);
		break;
	case TYPE_THING:
		moveto(thing, DBFETCH(thing)->link);
		break;
	case TYPE_PROGRAM:
		moveto(thing, OWNER(thing));
		break;
	}
}

/*
 * trigger()
 *
 * This procedure triggers a series of actions, or meta-actions
 * which are contained in the 'dest' field of the exit.
 * Locks other than the first one are over-ridden.
 *
 * `player' is the player who triggered the exit
 * `exit' is the exit triggered
 * `pflag' is a flag which indicates whether player and room exits
 * are to be used (non-zero) or ignored (zero).  Note that
 * player/room destinations triggered via a meta-link are
 * ignored.
 *
 */

void trigger(dbref player, dbref exit1, int pflag) {
	frame *fr = NULL;
	int i = 0;
	dbref dest = 0l;
	int sobjact = 0; /* sticky object action flag, sends home source obj */
	int succ = 0;

	for (i = 0; i < DBFETCH(exit1)->sp.exit.ndest; i++) {
		dest = (DBFETCH(exit1)->sp.exit.dest)[i];
		if (dest == HOME)
			dest = DBFETCH(player)->link;
		if ((FLAGS(dest) & ENTER_OK) || (Typeof(dest) == TYPE_ROOM)) {
			if (pflag && !parent_loop_check(player, dest)) {
				if (GET_DROP(exit1))
					exec_or_notify(player, exit1, GET_DROP(exit1));
				if (GET_ODROP(exit1) && !Dark(player))
					notify_except(player, dest, player, "%s %s", unparse_name(player),
							pronoun_substitute(player, GET_ODROP(exit1)));
				enter_room(player, dest, exit1);
				succ = 1;
			}
		} else {
			switch (Typeof(dest)) {
			case TYPE_THING:
				if (Typeof(DBFETCH(exit1)->location) == TYPE_THING) {
					moveto(dest, DBFETCH(DBFETCH(exit1)->location)->location);
					if (!(FLAGS(exit1) & STICKY))
						sobjact = 1;
				} else
					moveto(dest, DBFETCH(exit1)->location);
				if (GET_SUCC(exit1))
					succ = 1;
				break;
			case TYPE_EXIT: /* It's a meta-link(tm)! */
				trigger(player, (DBFETCH(exit1)->sp.exit.dest)[i], 0);
				if (GET_SUCC(exit1))
					succ = 1;
				break;
			case TYPE_PLAYER:
				if (pflag && DBFETCH(dest)->location != NOTHING) {
					succ = 1;
					if (FLAGS(dest) & JUMP_OK) {
						if (GET_DROP(exit1))
							exec_or_notify(player, exit1, GET_DROP(exit1));
						if (GET_ODROP(exit1) && !Dark(player))
							notify_except(player, DBFETCH(dest)->location,
									player, "%s %s",
									unparse_name(player),
									pronoun_substitute(player, GET_ODROP(exit1)));
						enter_room(player, DBFETCH(dest)->location, exit1);
					} else
						notify(player, player,
								"That player does not wish to be disturbed.");
				}
				break;
			case TYPE_PROGRAM:
				fr = new_frame(player, dest, exit1, DBFETCH(player)->location,
						0);
				add_frame(fr);
				run_frame(fr, 0);
				succ = 1;
			}
		}
	}
	if (sobjact)
		send_home(DBFETCH(exit1)->location);
	if (!succ && pflag)
		notify(player, player, "Done.");
}

int can_move(dbref player, char *direction) {
	match_data md;

	if (!string_compare(direction, "home"))
		return 1;

	/* otherwise match on exits */
	init_match(player, direction, TYPE_EXIT, &md);
	match_all_exits(&md);
	return (last_match_result(&md) != NOTHING);
}

void do_earthquake(__DO_PROTO) {
	dbref victim = 0, temp = 0;

	if (!Wizard(player)) {
		notify_nolisten(player, "Ha! You _wish_ you had that much clout!");
	} else {

		/* Use this in extreme cases of duress.  Should sanitize the crap out
		 *  of your database.  Not responsible for any damage.  Read directions
		 *  carefully.  Do not take internally.  In case of eye contact, flush well.
		 *  In case of ingestion, induce vomiting and contact a doctor immediately.
		 */

		/* nuke the victims contents, exits, and next fields */
		/* and sanitize their location, link, and destinations */
		/* also do a call to sanitize the locks on the object */
		for (victim = (dbref) 0; victim < db_top; victim++) {

			if ((victim % 1000) == 0)
				notify_nolisten(player, "Sanitizing: %ld", victim);

			DBSTORE(victim, contents, NOTHING);
			DBSTORE(victim, exits, NOTHING);
			DBSTORE(victim, next, NOTHING);

			temp = DBFETCH(victim)->location;
			if (temp < 0 || temp >= db_top)
				DBSTORE(victim, location,
						GLOBAL_ENVIRONMENT);
			temp = DBFETCH(victim)->link;
			if (temp < 0 || temp >= db_top || temp == HOME)
				DBSTORE(victim, link, NOTHING);

			sanitize_lock(DBFETCH(victim)->key);

		}

		/* now move them all back where they belong. */

		/* a little braindamaged, but the global environment shouldn't */
		/* be inside itself :) */

		DBSTORE(GLOBAL_ENVIRONMENT, location, NOTHING);

		for (victim = (dbref) 0; victim < db_top; victim++) {
			if (victim % 1000 == 0)
				notify_nolisten(player, "Sanitizing: %ld", victim);

			if (Typeof(victim) != TYPE_GARBAGE)
				moveto(victim, DBFETCH(victim)->location);
		}

		/* reset all the backlinks and backlocks too... */
		reset_lists();
	}
}

void do_move(__DO_PROTO) {
	dbref exit1 = 0l;
	dbref loc = 0l;
	match_data md;

	if (!string_compare(argall, "home")) {
		/* send him home */
		/* but steal all his possessions */
		if ((loc = DBFETCH(player)->location) != NOTHING) {
			/* tell everybody else */
			if (!Dark(player))
				notify_except(player, loc, player, "%s goes home.", unparse_name(player));
		}
		/* give the player the messages */
		notify(player, player,
				"You wake up back home, without your possessions.");
		send_home(player);
	} else {
		/* find the exit */
		init_match_check_keys(player, argall, TYPE_EXIT, &md);
		match_all_exits(&md);
		switch (exit1 = match_result(&md)) {
		case NOTHING:
			notify(player, player, "You can't go that way.");
			break;
		case AMBIGUOUS:
			notify(player, player, "I don't know which way you mean!");
			break;
		default:
			/* we got one */
			/* check to see if we got through */
			loc = DBFETCH(player)->location;
#ifdef TIMESTAMPS
			DBFETCH(exit1)->time_used = time((long *) NULL);
#endif
			if (can_doit(player, exit1, "You can't go that way."))
				trigger(player, exit1, 1);
		}
	}
}

void do_get(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	init_match_check_keys(player, arg1, TYPE_THING, &md);
	match_neighbor(&md);
	if (Wizard(player))
		match_absolute(&md); /* the wizard has long fingers */

	if ((thing = noisy_match_result(&md)) != NOTHING) {
#ifdef TIMESTAMPS
		DBFETCH(thing)->time_used = time((long *) 0);
#endif
		if (DBFETCH(thing)->location == player) {
			notify(player, player, "You already have that!");
			return;
		}
		switch (Typeof(thing)) {
		case TYPE_THING:
		case TYPE_PROGRAM:
			if (can_doit(player, thing, "You can't pick that up.")) {
				moveto(thing, player);
				notify(player, player, "Taken.");
			}
			break;
		default:
			notify(player, player, "You can't take that!");
			break;
		}
	}
}

void do_drop(__DO_PROTO) {
	dbref loc = 0l;
	dbref thing = 0l;
	match_data md;

	if ((loc = getloc(player)) == NOTHING)
		return;

	init_match(player, arg1, NOTYPE, &md);
	match_possession(&md);
	if ((thing = noisy_match_result(&md)) == NOTHING || thing == AMBIGUOUS)
		return;

	if (!controls(player, DBFETCH(player)->location) && ((Typeof(thing)
			== TYPE_EXIT) || (Typeof(thing) == TYPE_ROOM))) {
		notify(player, player, "Permission denied.");
		return;
	}
#ifdef TIMESTAMPS
	DBFETCH(thing)->time_used = time(NULL);
#endif
	if (DBFETCH(thing)->location != player) {
		/* Shouldn't ever happen. */
		notify(player, player, "You can't drop that.");
	}
	if ((FLAGS(thing) & STICKY) && Typeof(thing) == TYPE_THING)
		send_home(thing);
	else {
		int immediate_dropto;

		immediate_dropto = (DBFETCH(loc)->link != NOTHING && !(FLAGS(loc)
				& STICKY));

		moveto(thing, immediate_dropto ? DBFETCH(loc)->link : loc);
		look_room(thing, loc);
	}
	if (GET_DROP(thing))
		exec_or_notify(player, thing, GET_DROP(thing));
	else
		notify(player, player, "Dropped.");

	if (GET_DROP(loc))
		exec_or_notify(player, loc, GET_DROP(loc));

	if (GET_ODROP(thing)) {
		strcpy(buf, unparse_name(player));
		strcat(buf, " ");
		strcat(buf, pronoun_substitute(player, GET_ODROP(thing)));
	} else {
		strcpy(buf, unparse_name(player));
		strcat(buf, " drops ");
		strcat(buf, unparse_name(thing));
	}
	notify_except(player, loc, player, buf);

	if (GET_ODROP(loc)) {
		strcpy(buf, unparse_name(thing));
		strcat(buf, " ");
		strcat(buf, pronoun_substitute(thing, GET_ODROP(loc)));
		notify_except(player, loc, player, buf);
	}
}

void recycle_backlocks(dbref d, boolexp *b) {
	if (b) {
		switch (b->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			recycle_backlocks(d, b->sub2);
		case BOOLEXP_NOT:
			recycle_backlocks(d, b->sub1);
			break;
		case BOOLEXP_CONST:
			if (b->thing == d)
				b->thing = NOTHING;
		}
	}
}

void recycle_locks(dbref d, boolexp *b) {
	if (b) {
		switch (b->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			recycle_locks(d, b->sub2);
		case BOOLEXP_NOT:
			recycle_locks(d, b->sub1);
			break;
		case BOOLEXP_CONST:
			if (b->thing == NOTHING)
				return;
			DBSTORE(b->thing, backlocks,
					dbreflist_remove(DBFETCH(b->thing)->backlocks, d))
			;
		}
	}
}

void recycle(dbref player, dbref thing) {
	dbref first = 0l, rest = 0l;
	dbref_list *drl = NULL, *drl_temp = NULL;
	int i = 0, j = 0;

#ifdef FLUSH_OWNERSHIP
	/* change ownership on all the stuff */
	for (stuff = thing; stuff != NOTHING; stuff = DBFETCH(stuff)->nextowned)
	{
		DBSTORE(stuff, owner, OWNER(player));
	}

	for(stuff = OWNER(player); DBFETCH(stuff)->nextowned != NOTHING;
			stuff = DBFETCH(stuff)->nextowned);
	DBSTORE(stuff, nextowned, thing);

	/* link all objects now linked to the thing to the player */
	while (DBFETCH(thing)->backlinks &&
			((Typeof(DBFETCH(thing)->backlinks->object)) == TYPE_THING))
	{
		tmp = DBFETCH(thing)->backlinks;
		DBSTORE(thing, backlinks, tmp->next);
		DBSTORE(tmp->object, link, OWNER(player));
		DBSTORE(tmp->object, owner, OWNER(player));
		add_backlinks(tmp->object);
		free(tmp);
	}

	for (list = DBFETCH(thing)->backlinks; list && list->next; list = list->next)
	{
		if (Typeof(list->next->object) == TYPE_THING)
		{
			tmp = list->next;
			list->next = tmp->next;
			DBSTORE(tmp->object, link, OWNER(player));
			add_backlinks(tmp->object);
			free(tmp);
		}
	}

	/* and now send all objects currently on the thing to their home */
	send_contents(thing, HOME);
#endif

	remove_ownerlist(thing);
	for (drl = DBFETCH(thing)->backlocks; drl; drl = drl->next)
		recycle_backlocks(thing, DBFETCH(drl->object)->key);
	recycle_locks(thing, DBFETCH(thing)->key);
	DBSTORE(thing, backlocks, NULL);
	for (drl = DBFETCH(thing)->backlinks; drl; drl = drl->next) {
		rest = drl->object;
		switch (Typeof(rest)) {
		case TYPE_ROOM:
			if (rest != HOME) {
				DBSTORE(rest, link, NOTHING);
			}
			break;
		case TYPE_EXIT:
			for (i = j = 0; i < DBFETCH(rest)->sp.exit.ndest; i++) {
				if ((DBFETCH(rest)->sp.exit.dest)[i] != thing)
					(DBFETCH(rest)->sp.exit.dest)[j++]
							= (DBFETCH(rest)->sp.exit.dest)[i];
			}
			if (j < DBFETCH(rest)->sp.exit.ndest) {
				if (!Wizard(player))
					DBFETCH(player)->pennies += LINK_COST;
				DBSTORE(rest, sp.exit.ndest, j);DBDIRTY(player);
			}
			break;
		default:
			if (rest == HOME)
				rest = DBFETCH(OWNER(thing))->link;
			if (DBFETCH(rest)->link == thing) {
				DBSTORE(rest, link, PLAYER_START);
			} else {
				DBSTORE(rest, link, DBFETCH(rest)->link);
			}
			add_backlinks(rest);
		}
	}

	remove_backlinks(thing);

	for (first = DBFETCH(thing)->exits; first != NOTHING; first = rest) {
		rest = DBFETCH(first)->next;
		if (DBFETCH(first)->location == NOTHING || DBFETCH(first)->location
				== thing)
			recycle(player, first);
	}

	for (first = DBFETCH(thing)->contents; first != NOTHING; first = rest) {
		rest = DBFETCH(first)->next;
		if (Typeof(first) == TYPE_PLAYER) {
			notify_except(first, thing, NOTHING,
					"You feel a wrenching sensation...");
			enter_room(first, HOME, DBFETCH(thing)->location);
		} else
			moveto(first, HOME);
	}

	switch (Typeof(thing)) {
	case TYPE_ROOM:
		if (!Wizard(OWNER(thing)))
			DBFETCH(OWNER(thing))->pennies += ROOM_COST;
		DBDIRTY(OWNER(thing));
		break;
	case TYPE_THING:
		if (!Wizard(OWNER(thing)))
			DBFETCH(OWNER(thing))->pennies += DBFETCH(thing)->pennies;
		DBDIRTY(OWNER(thing));
		break;
	case TYPE_EXIT:
		if (!Wizard(OWNER(thing)))
			DBFETCH(OWNER(thing))->pennies += EXIT_COST;
		if (((DBFETCH(thing)->sp.exit.ndest) != 0) && (!Wizard(OWNER(thing))))
			DBFETCH(OWNER(thing))->pennies += LINK_COST;
		DBDIRTY(OWNER(thing));
		break;
	case TYPE_PROGRAM:
		for (drl = DBFETCH(thing)->sp.program.editlocks; drl; drl = drl_temp) {
			drl_temp = drl->next;
			if (drl->object != player)
				notify(drl->object, drl->object, "Program %s recycled by %s.", unparse_name(thing), unparse_name(player));
			FLAGS(drl->object) &= ~INTERACTIVE;
			DBSTORE(drl->object, curr_prog, NOTHING);
			free(drl);
		}
		bump_frames(buf, thing, player);
		snprintf(buf, BUFFER_LEN, "muf/%ld.m", thing);
		unlink(buf);
	}

	DBSTORE(thing, owner, NOTHING);
	moveto(thing, NOTHING);
	db_free_object(thing);
	db_clear_object(thing);
	DBSTORE(thing, name, COMPOST_NAME);
	DBSTORE(thing, desc, COMPOST_DESC);
	DBSTORE(thing, key, TRUE_BOOLEXP);
	FLAGS(thing) = TYPE_GARBAGE;

	add_compost(thing);
	DBDIRTY(thing);
}

void do_recycle(__DO_PROTO) {
	dbref thing = 0l;
	match_data md;

	init_match(player, arg1, NOTYPE, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	match_here(&md);
	match_absolute(&md);

	if ((thing = noisy_match_result(&md)) == NOTHING) {
		notify(player, player, "Recycle what?");
		return;
	}

	if (Typeof(thing) == TYPE_GARBAGE) {
		notify(player, player, "Garbage is already recycled.");
		return;
	}

	if (o_wiz_recycle) {
		if (!controls(player, thing)) {
			notify(player, player, "Permission denied.");
			return;
		} else {
			if (OWNER(thing) != player) {
				notify(player, player, "Permission denied.");
				return;
			}
		}
	}

	if (Safe(thing)) {
		notify(player, player, "Object is set SAFE.");
		return;
	}

	switch (Typeof(thing)) {
	case TYPE_ROOM:
		if (thing == PLAYER_START || thing == GLOBAL_ENVIRONMENT) {
			notify(player, player, "This room may not be recycled.");
			return;
		}
		break;
	case TYPE_PLAYER:
		notify(player, player, "You can't recycle a player!");
		return;
	}
	recycle(player, thing);
	notify(player, player, "Thank you for recycling.");
}
