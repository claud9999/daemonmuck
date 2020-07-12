#include "copyright.h"
#include "config.h"
#include <sys/time.h>
#include <sys/resource.h>

#include "db.h"
#include "params.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "money.h"

extern time_t time_started;
extern frame *frame_list;

void do_backlinks(__DO_PROTO) {
	dbref object;
	dbref_list *drl;
	match_data md;

	/* get victim, destination */
	if (*arg1 == '\0')
		object = player;
	else {
		init_match(player, arg1, NOTYPE, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_player(&md);

		if ((object = noisy_match_result(&md)) == NOTHING)
			return;
	}

	if (!controls(player, object)) {
		notify(player, player, "Permission denied.");
		return;
	}
	notify(player, player, "Backlinks:");
	for (drl = DBFETCH(object)->backlinks; drl; drl = drl->next)
		notify(player, player, unparse_object(player, drl->object));
}

void do_backlocks(__DO_PROTO) {
	dbref object;
	dbref_list *drl;
	match_data md;

	/* get victim, destination */
	if (*arg1 == '\0')
		object = player;
	else {
		init_match(player, arg1, NOTYPE, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_player(&md);

		if ((object = noisy_match_result(&md)) == NOTHING)
			return;
	}

	if (!controls(player, object)) {
		notify(player, player, "Permission denied.");
		return;
	}

	notify(player, player, "Backlocks:");
	for (drl = DBFETCH(object)->backlocks; drl; drl = drl->next)
		notify(player, player, unparse_object(player, drl->object));
}

void do_teleport(__DO_PROTO) {
	dbref victim;
	dbref destination;
	char *to;
	match_data md;

	/* get victim, destination */
	if (*arg2 == '\0') {
		victim = player;
		to = arg1;
	} else {
		init_match(player, arg1, NOTYPE, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_player(&md);

		if ((victim = noisy_match_result(&md)) == NOTHING)
			return;
		to = arg2;
	}

	if (Typeof(victim) == TYPE_EXIT) {
		notify(player, player, "Permission denied.  Use @attach instead.");
		return;
	}

	/* get destination */
	init_match(player, to, NOTYPE, &md);
	match_here(&md);
	match_home(&md);
	match_absolute(&md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(player))
		match_player(&md);

	destination = match_result(&md);

	switch (destination) {
	case NOTHING:
		notify(player, player, "Send it where?");
		return;
	case AMBIGUOUS:
		notify(player, player, "I don't know which destination you mean!");
		return;
	case HOME:
		destination = DBFETCH(victim)->link;
	}

	if (parent_loop_check(victim, destination)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (Typeof(victim) == TYPE_GARBAGE) {
		notify(player, player, "Can't teleport garbage.");
		return;
	}

	if (((FLAGS(player) & WIZARD) || ((can_link_to(player, NOTYPE, destination)
			|| (FLAGS(destination) & JUMP_OK)) && controls(player, victim)))
			&& Typeof(destination) != TYPE_PROGRAM) {
		notify(victim, victim, "You feel a wrenching sensation...");
		switch (Typeof(victim)) {
		case TYPE_THING:
		case TYPE_PROGRAM:
			/* check for non-sticky dropto */
			if ((Typeof(destination) == TYPE_ROOM)
					&& (DBFETCH(destination)->link != NOTHING)
					&& !(FLAGS(destination) & STICKY))
				destination = DBFETCH(destination)->link;
		default:
			enter_room(victim, destination, DBFETCH(victim)->location);
			notify(player, player, "Teleported.");
		}
	} else {
		notify(player, player, "Permission denied.");
	}
}

void do_force(__DO_PROTO) {
	dbref victim;
	match_data md;

	if (*arg1 == '\0')
		victim = player;
	else {
		init_match(player, arg1, NOTYPE, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_player(&md);

		if ((victim = noisy_match_result(&md)) == NOTHING)
			return;
	}

	if (!controls(player, victim)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (!arg2 || !*arg2)
		return;
	log_status("FORCED: %s(%ld) by %s(%ld): %s\n", NAME(victim), victim,
			NAME(player), player, arg2);
	/* force victim to do command */
	if (Typeof(victim) == TYPE_PLAYER) {
		notify(victim, victim, "%s forced you to type %s.", unparse_name(player), arg2);
	}

	if (Typeof(victim) == TYPE_PLAYER) {
		if (o_notify_wiz) notify_wizards("%s forced %s: %s", unparse_name(player), unparse_name(victim), arg2);
	}
	process_command(victim, arg2, victim);
}

void do_stats(__DO_PROTO) {
	int rooms = 0, exits = 0, things = 0, players = 0, programs = 0, total = 0,
			garbage = 0, frames = 0;
	frame *fr;
	dbref i, obj;
	match_data md;
	int pid, psize;
#ifdef HAVE_GETRUSAGE
	struct rusage usage;
#endif
	char *sitename;

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

		if (!controls(player, obj)) {
			notify(player, player, "Permission denied.");
			return;
		}

		switch (Typeof(obj)) {
		case TYPE_PLAYER:
			for (i = obj; i != NOTHING; i = DBFETCH(i)->nextowned) {
				total++;
				switch (Typeof(i)) {
				case TYPE_ROOM:
					rooms++;
					break;
				case TYPE_EXIT:
					exits++;
					break;
				case TYPE_THING:
					things++;
					break;
				case TYPE_PLAYER:
					players++;
					break;
				case TYPE_PROGRAM:
					programs++;
					break;
				case TYPE_GARBAGE:
					garbage++;
					break;
				}
			}

			for (fr = frame_list, frames = 0; fr; fr = fr->next) {
				if (fr->player == obj)
					frames++;
			}

			notify(player, player, "Stats for %s:", unparse_name(obj));

			notify(player, player, " Rooms   : (%6.2f%%) %5d   Exits    : (%6.2f%%) %5d",
					DO_PERCENT(rooms), DO_PERCENT(exits));

			notify(player, player, " Things  : (%6.2f%%) %5d   Programs : (%6.2f%%) %5d",
					DO_PERCENT(things), DO_PERCENT(programs));

			notify(player, player, " Players : (%6.2f%%) %5d   Processes: (       ) %5d",
					DO_PERCENT(players),frames);

			notify(player, player, " Garbage : (%6.2f%%) %5d   Total    : (100.00%%) %5d",
					DO_PERCENT(garbage),total);

			notify(player, player, " ");
			break;
		}
#ifdef TIMESTAMPS
		notify(player, player, "CREATED       : %s", asctime(localtime(
				&DBFETCH(obj)->time_created)));
		notify(player, player, "LAST MODIFIED : %s", asctime(localtime(
				&DBFETCH(obj)->time_modified)));
		notify(player, player, "LAST USED     : %s", asctime(localtime(
				&DBFETCH(obj)->time_used)));
#endif
	} else {
		for (i = 0; i < db_top; i++) {
			switch (Typeof(i)) {
			case TYPE_ROOM:
				total++;
				rooms++;
				break;
			case TYPE_EXIT:
				total++;
				exits++;
				break;
			case TYPE_THING:
				total++;
				things++;
				break;
			case TYPE_PLAYER:
				total++;
				players++;
				break;
			case TYPE_PROGRAM:
				total++;
				programs++;
				break;
			case TYPE_GARBAGE:
				total++;
				garbage++;
				break;
			}
		}
		for (fr = frame_list, frames = 0; fr; fr = fr->next, frames++)
			;
		notify(player, player, "%s stats:", (sitename = get_property_data((dbref) 0,
				RWHO_NAME, ACCESS_WI)) ? sitename : "Universe");
		notify(player, player, " Rooms   : (%6.2f%%) %5d   Exits    : (%6.2f%%) %5d",
				DO_PERCENT(rooms), DO_PERCENT(exits));
		notify(player, player, " Things  : (%6.2f%%) %5d   Programs : (%6.2f%%) %5d",
				DO_PERCENT(things), DO_PERCENT(programs));
		notify(player, player, " Players : (%6.2f%%) %5d   Processes: (       ) %5d",
				DO_PERCENT(players),frames);
		notify(player, player, " Garbage : (%6.2f%%) %5d   Total    : (100.00%%) %5d",
				DO_PERCENT(garbage),total);
		notify(player, player, " ");
		notify(player, player, "Up since: %s", asctime(localtime(&time_started)));

		if (Wizard(player)) {
			pid = getpid();
#ifdef HAVE_GETPAGESIZE
			psize = getpagesize();
#endif
			notify(player, player, "Peak number of players...........%d", maxplayer);
			notify(player, player, "Process ID.......................%ld", pid);
#ifdef HAVE_GETRUSAGE
			if (getrusage(RUSAGE_SELF, &usage) != -1) {
				notify(player, player, "User time used (CPU secs)........%ld.%ld",
						usage.ru_utime.tv_sec, usage.ru_utime.tv_usec / 10000);
				notify(player, player, "System time used(CPU secs).......%ld.%ld",
						usage.ru_stime.tv_sec, usage.ru_stime.tv_usec / 10000);
#ifdef HAVE_GETPAGESIZE
				notify(player, player, "Resident memory (bytes)..........%ld",
						usage.ru_maxrss * psize);
#endif
				notify(player, player, "Page faults (No I/O).............%ld",
						usage.ru_minflt);
				notify(player, player, "Page faults (I/O)................%ld",
						usage.ru_majflt);
				notify(player, player, "Swapped..........................%ld",
						usage.ru_nswap);
				notify(player, player, "Input services...................%ld",
						usage.ru_inblock);
				notify(player, player, "Output services..................%ld",
						usage.ru_oublock);
				notify(player, player, "Messages/Bytes sent..............%ld/%ld",
						usage.ru_msgsnd, 0);
				notify(player, player, "Messages/Bytes received..........%ld/%ld",
						usage.ru_msgrcv, 0);
				notify(player, player, "Signals received.................%ld",
						usage.ru_nsignals);
				notify(player, player, "Voluntarily context switches.....%ld",
						usage.ru_nvcsw);
				notify(player, player, "Involuntarily context switches...%ld",
						usage.ru_nivcsw);
			}
#endif
		}
	}
}

void do_boot(__DO_PROTO) {
	dbref victim;
	match_data md;

	init_match(player, arg1, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	match_absolute(&md);
	match_player(&md);

	victim = last_match_result(&md);
	victim = (victim == NOTHING) ? lookup_player(arg1) : victim;

	if (victim == NOTHING) {
		notify(player, player, "That player does not exist.");
		return;
	}

	if (!controls(player, victim) || (Typeof(victim) != TYPE_PLAYER)) {
		notify(player, player, "Permission denied.");
		return;
	}

	notify(victim, victim, "You have been booted off of the game by %s.", unparse_name(
			player));
	if (boot_off(victim)) {
		if (o_notify_wiz)
			notify_wizards("%s booted %s.", unparse_name(player), unparse_name(victim));
		log_status("BOOTED: %s(%ld) by %s(%ld)\n", NAME(victim), victim,
				NAME(player), player);
		if (victim != player)
			notify(player, player, "You booted %s off!", unparse_name(victim));
	} else
		notify(player, player, "%s is not connected.", unparse_name(victim));
}

void toad(dbref player, dbref recipient) {
	dbref stuff = 0l;
	dbref_list *list = NULL, *tmp = NULL;
	char buf[BUFFER_LEN];

	boot_off(player);

	/* change ownership on all the stuff */
	for (stuff = player; stuff != NOTHING; stuff = DBFETCH(stuff)->nextowned) {
		DBSTORE(stuff, owner, recipient);
	}

	for (stuff = recipient; DBFETCH(stuff)->nextowned != NOTHING; stuff
			= DBFETCH(stuff)->nextowned)
		;
	DBSTORE(stuff, nextowned, player);

	/* link all objects now linked to the player to the recipient */
	while (DBFETCH(player)->backlinks
			&& ((Typeof(DBFETCH(player)->backlinks->object)) == TYPE_THING)) {
		tmp = DBFETCH(player)->backlinks;
		DBSTORE(player, backlinks, tmp->next);
		DBSTORE(tmp->object, link, recipient);
		add_backlinks(tmp->object);
		free(tmp);
	}

	for (list = DBFETCH(player)->backlinks; list && list->next; list
			= list->next) {
		if (Typeof(list->next->object) == TYPE_THING) {
			tmp = list->next;
			list->next = tmp->next;
			DBSTORE(tmp->object, link, recipient);
			add_backlinks(tmp->object);
			free(tmp);
		}
	}

	/* and now send all objects currently on the player to their home */
	send_contents(player, HOME);

	/* may as well reclaim the password property */

	if (DBFETCH(player)->sp.player.password) {
		free(DBFETCH(player)->sp.player.password);
		DBSTORE(player, sp.player.password, NULL);
	}

	/* thou art now a toad!  ZZZzzzzzZZap! */
	FLAGS(player) = TYPE_THING;

	/* make the toad an invaluable object */
	DBFETCH(player)->pennies = 1;

	/* remove the player from the hash table */
	delete_player(player);

	/* change the name */
	snprintf(buf, BUFFER_LEN, "a slimy toad named %s", unparse_name(player));
	free(NAME(player));
	DBSTORE(player, name, dup_string(buf));
}

void do_toad(__DO_PROTO) {
	dbref victim;
	dbref recipient;
	match_data md;

	init_match(player, arg1, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_player(&md);
	victim = last_match_result(&md);
	victim = (victim == NOTHING) ? lookup_player(arg1) : victim;

	if (victim == NOTHING) {
		notify(player, player, "No such player.");
		return;
	}

#ifdef GOD_PRIV
	if (!(FLAGS(player) & GOD)) {
		notify(player, player, "Permission denied.");
		return;
	}
#endif

	if ((Typeof(victim) != TYPE_PLAYER) || (!controls(player, victim))
			|| (player == victim)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (!*arg2)
		recipient = NOTHING;
	else {
		init_match(player, arg2, NOTYPE, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_player(&md);
		recipient = last_match_result(&md);
		recipient = (recipient == NOTHING) ? lookup_player(arg2) : recipient;
	}

	if (recipient == NOTHING) {
		notify(player, player, "Recipient not found...making you recipient.");
		recipient = player;
	}

	if ((Typeof(recipient) != TYPE_PLAYER) || (!controls(player, recipient))) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (Wizard(victim)) {
		notify(player, player, "You can't turn a Wizard into a toad.");
		return;
	}

	notify(victim, victim, "You have been turned into a toad by %s!",
			unparse_name(player));
	notify(player, player, "You turned %s into a toad!", unparse_name(victim));

	if (o_notify_wiz) notify_wizards("%s toaded %s.", unparse_name(player), unparse_name(victim));

	log_status("TOADED: %s(%ld) by %s(%ld)\n", NAME(victim), victim,
			NAME(player), player);
	toad(victim, recipient);
}

void do_trimdb(__DO_PROTO) {
	dbref old_db_top = db_top;

	if (!(Wizard(player))) {
		notify_nolisten(player, "Permission denied.");
	} else {
		for (db_top--; Typeof(db_top) == TYPE_GARBAGE; db_top--)
			;
		db_top++;
		notify_nolisten(player, "Trimming database from %ld to %ld.", old_db_top, db_top);
	}
}

void do_newpassword(__DO_PROTO) {
	dbref victim;
	match_data md;

	init_match(player, arg1, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_player(&md);
	victim = last_match_result(&md);
	victim = (victim == NOTHING) ? lookup_player(arg1) : victim;
	if ((Typeof(victim) != TYPE_PLAYER) || !Wizard(player)) {
		notify(player, player, "Permission denied.");
		return;
	}

	/* nuke .last property on player */
	add_property(player, ".last", "@newpassword", default_perms(".last"),
			ACCESS_CO);

	if (!ok_password(arg2)) {
		notify(player, player, "Bad password.");
		return;
	}

	if (DBFETCH(victim)->sp.player.password)
		free(DBFETCH(victim)->sp.player.password);

	DBSTORE(victim, sp.player.password, make_password(arg2));

	notify(player, player, "Password changed.");
	notify(victim, victim, "Your password has been changed by %s.", unparse_name(player));

	if (o_notify_wiz)
		notify_wizards("%s newpassworded %s.", unparse_name(player), unparse_name(victim));

	log_status("NEWPASS'ED: %s(%ld) by %s(%ld)\n", NAME(victim), victim,
			NAME(player), player);
}

void do_pcreate(__DO_PROTO) {
	dbref newguy;

	if (!(FLAGS(player) &
#ifdef GOD_PRIV
			GOD))
#else
	WIZARD))
#endif
	{
		notify(player, player, "Permission denied.");
		return;
	}

	newguy = create_player(arg1, arg2, player);
	if (newguy == NOTHING)
		notify(player, player, "Create failed.");
	else {
		add_property(player, ".last", "@pcreate", default_perms(".last"),
				ACCESS_CO);

		if (o_notify_wiz) {
			notify_wizards("%s has created player %s", unparse_name(player), unparse_name(newguy));
			log_status("PCREATED %s(%ld) by %s(%ld)\n", NAME(newguy), newguy,
					NAME(player), player);
			notify(player, player, "Player %s created as object #%ld.", NAME(newguy),
					newguy);
		}
		if (o_player_user_functions) {
			FLAGS(newguy) |= NOCOMMAND;
			DBDIRTY(newguy);
		}
	}
}

void do_login(__DO_PROTO) /* Allow/Disallow logins in the server     */
{ /* Quick function done by Howard  Feel     */
	if (!Wizard(player)) { /* free to fancy it up as much as you like */
		notify(player, player, "Permission denied.");
		return;
	}

	if (!strncmp(argall, "off", 3)) {
		wiz_only_flag = 1;
		notify(player, player, "Non Wizard logins have been DISABLED.");
	} else if (!strncmp(argall, "on", 2)) {
		wiz_only_flag = 0;
		notify(player, player, "Non Wizard logins are now ENABLED.");
	} else {
		if (wiz_only_flag == 1)
			notify(player, player, "Non Wizard login are currently DISABLED.");
		else
			notify(player, player, "Non Wizard login are currently ENABLED.");
	}
}

void do_reset(__DO_PROTO) {
	if (Wizard(player)) {
		notify(player, player, "Resetting lists.  This will take a moment.");
		reset_lists();
		notify(player, player, "Done.");
	} else
		notify(player, player, "Permission denied.");
}
