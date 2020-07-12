#include "copyright.h"
#include "config.h"

/* commands which set parameters */
#include <stdio.h>
#include <ctype.h>

#include "db.h"
#include "params.h"
#include "match.h"
#include "interface.h"
#include "externs.h"

#ifdef COMPRESS
#define alloc_compressed(x) dup_string(compress(x))
#else /* COMPRESS */
#define alloc_compressed(x) dup_string(x)
#endif /* COMPRESS */

dbref match_controlled(dbref player, char *name) {
	dbref match;
	match_data md;

	init_match(player, name, NOTYPE, &md);
	match_everything(&md);

	match = noisy_match_result(&md);
	if (match != NOTHING && !controls(player, match)) {
		notify(player, player, "Permission denied.");
		return NOTHING;
	} else
		return match;
}

void do_name(__DO_PROTO) {
	dbref thing;
	char *password;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		/* check for bad name */
		if (*arg2 == '\0') {
			notify(player, player, "Give it what new name?");
			return;
		}

		/* check for renaming a player */
		if (Typeof(thing) == TYPE_PLAYER) {
			/* split off password */
			for (password = arg2; *password && !isspace(*password); password++)
				;
			/* eat whitespace */
			if (*password) {
				*password++ = '\0'; /* terminate name */
				while (*password && isspace(*password))
					password++;
			}

			/* check for null password */
			if (!*password) {
				notify(player, player,
						"You must specify a password to change a player's name.");
				notify(player, player, "E.g.: name player = newname password");
				return;
			}

			if (check_password(password, thing)) {
				notify(player, player, "Incorrect password.");
				return;
			}

			if (!ok_player_name(arg2, thing)) {
				notify(player, player, "You can't give a player that name.");
				return;
			}

			if (o_taboonames) {
				if (!ok_taboo_name(thing, arg2, 1) == 1 && !Wizard(player)) {
					notify(player, player,
							"Sorry, That name is not allowed on this MUD!");
					return;
				}
			}

			/* everything ok, notify */
			log_status("NAME CHANGE: %s(#%d) to %s\n", NAME(thing), thing, arg2);
			if (o_notify_wiz)
				notify_wizards("%s did a name change: %s -> %s", unparse_name(
						player), unparse_name(thing), arg2);
			delete_player(thing);
			if (NAME(thing))
				free(NAME(thing));
			NAME(thing) = dup_string(arg2);
			add_player(thing);
			DBSTORE(thing, time_modified, time(NULL));
			notify(player, player, "Name set.");
			return;
		} else {
			if (!ok_name(arg2)) {
				notify(player, player, "That is not a reasonable name.");
				DBSTORE(thing, time_modified, time(NULL));
				return;
			}
		}

		/* everything ok, change the name */
		if (NAME(thing))
			free(NAME(thing));
		NAME(thing) = dup_string(arg2);
		notify(player, player, "Name set.");
		DBDIRTY(thing);
	}
}

void do_describe(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_DESC(thing))
			free(DBFETCH(thing)->desc);
		DBSTORE(thing, desc, alloc_compressed(arg2));

		notify(player, player, "Description set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

void do_fail(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_FAIL(thing))
			free(DBFETCH(thing)->fail);
		DBSTORE(thing, fail, alloc_compressed(arg2));

		notify(player, player, "Message set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

void do_success(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_SUCC(thing))
			free(DBFETCH(thing)->succ);
		DBSTORE(thing, succ, alloc_compressed(arg2));

		notify(player, player, "Message set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

/* sets the drop message for player */
void do_drop_message(__DO_PROTO) {
	dbref thing;
	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_DROP(thing))
			free(DBFETCH(thing)->drop);
		DBSTORE(thing, drop, alloc_compressed(arg2));

		notify(player, player, "Message set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

void do_osuccess(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_OSUCC(thing))
			free(DBFETCH(thing)->osucc);
		DBSTORE(thing, osucc, alloc_compressed(arg2));

		notify(player, player, "Message set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

void do_ofail(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_OFAIL(thing))
			free(DBFETCH(thing)->ofail);
		DBSTORE(thing, ofail, alloc_compressed(arg2));

		notify(player, player, "Message set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

void do_odrop(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		if (GET_ODROP(thing))
			free(DBFETCH(thing)->odrop);
		DBSTORE(thing, odrop, alloc_compressed(arg2));

		notify(player, player, "Message set.");

#ifdef TIMESTAMPS
		DBSTORE(thing, time_modified, time(NULL));
#endif
	}
}

void do_lock(__DO_PROTO) {
	dbref thing;
	boolexp *key;
	match_data md;

	init_match(player, arg1, NOTYPE, &md);
	match_everything(&md);

	switch (thing = match_result(&md)) {
	case NOTHING:
		notify(player, player, "I don't see what you want to lock!");
		return;
	case AMBIGUOUS:
		notify(player, player, "I don't know which one you want to lock!");
		return;
	default:
		if (!controls(player, thing)) {
			notify(player, player, "You can't lock that!");
			return;
		}
		break;
	}

#ifdef TIMESTAMPS
	DBSTORE(thing, time_modified, time(NULL));
#endif

	key = parse_boolexp(player, arg2);
	if (key == TRUE_BOOLEXP)
		notify(player, player, "I don't understand that key.");
	else {
		/* everything ok, do it */
		remove_backlocks_parse(thing, DBFETCH(thing)->key);
		free_boolexp(DBFETCH(thing)->key);
		DBSTORE(thing, key, key);
		add_backlocks_parse(thing, DBFETCH(thing)->key);
		notify(player, player, "Locked.");
	}
}

void do_unlock(__DO_PROTO) {
	dbref thing;

	if ((thing = match_controlled(player, arg1)) != NOTHING) {
		remove_backlocks_parse(thing, DBFETCH(thing)->key);
		free_boolexp(DBFETCH(thing)->key);
		DBSTORE(thing, key, TRUE_BOOLEXP);
		notify(player, player, "Unlocked.");
		DBSTORE(thing, time_modified, time(NULL));
	}
}

void do_unlink(__DO_PROTO) {
	dbref exit;
	match_data md;

	init_match(player, arg1, TYPE_EXIT, &md);
	match_all_exits(&md);
	match_here(&md);
	if (Wizard(player))
		match_absolute(&md);

	switch (exit = match_result(&md)) {
	case NOTHING:
		notify(player, player, "Unlink what?");
		break;
	case AMBIGUOUS:
		notify(player, player, "I don't know which one you mean!");
		break;
	default:
		if (!controls(player, exit))
			notify(player, player, "Permission denied.");
		else {
			switch (Typeof(exit)) {
			case TYPE_EXIT:
				if (DBFETCH(exit)->sp.exit.ndest != 0) {
					remove_backlinks(exit);
					if (!Wizard(player))
						DBFETCH(OWNER(exit))->pennies += LINK_COST;
					DBDIRTY(OWNER(exit));
				}
				DBSTORE(exit, sp.exit.ndest, 0)
				;
				if (DBFETCH(exit)->sp.exit.dest) {
					free((void *) DBFETCH(exit)->sp.exit.dest);
					DBSTORE(exit, sp.exit.dest, NULL);
				}
				notify(player, player, "Unlinked.");
				DBSTORE(exit, time_modified, time(NULL))
				;
				break;
			case TYPE_ROOM:
				remove_backlinks(exit);
				DBSTORE(exit, link, NOTHING)
				;
				notify(player, player, "Dropto removed.");
				DBSTORE(exit, time_modified, time(NULL))
				;
				break;
			default:
				notify(player, player, "You can't unlink that!");
				break;
			}
		}
	}
}

void do_chown(__DO_PROTO) {
	dbref thing;
	dbref owner;
	match_data md;

#ifndef PLAYER_CHOWN
	if(!Wizard(player))
	{
		notify(player, player, "Permission denied.");
		return;
	}
#endif /* PLAYER_CHOWN */

	if (!*arg1) {
		notify(player, player,
				"You must specify what you want to take ownership of.");
		return;
	}

	init_match(player, arg1, NOTYPE, &md);
	match_everything(&md);
	if ((thing = noisy_match_result(&md)) == NOTHING)
		return;

	if (*arg2 && string_compare(arg2, "me")) {
		if ((owner = lookup_player(arg2)) == NOTHING) {
			notify(player, player, "I couldn't find that player.");
			return;
		}
	} else
		owner = player;

	if ((!Wizard(player) && player != owner) || (Typeof(thing) == TYPE_PLAYER)
			|| (Typeof(thing) == TYPE_GARBAGE) || (Typeof(player)
			!= TYPE_PLAYER)
			|| (!Wizard(player) && (!(FLAGS(thing) & CHOWN_OK)))) {
		notify(player, player, "Permission denied.");
		return;
	}

	DBSTORE(thing, time_modified, time(NULL));
	remove_ownerlist(thing);
	DBSTORE(thing, owner, owner);
	add_ownerlist(thing);

	if (owner == player)
		notify(player, player, "Owner changed to you.");
	else
		notify(player, player, "Owner changed to %s.",
				unparse_object(player, owner));
}

/* Note: Gender code taken out.  All gender references are now to be handled
 by property lists...
 Setting of flags and property code done here.  Note that the PROP_DELIMITER
 identifies when you're setting a property.
 A @set <thing>= :
 will clear all properties.
 A @set <thing>= type:
 will remove that property.
 A @set <thing>= type: class
 will add that property or replace it.                                    */

void do_set(__DO_PROTO) {
	dbref thing;
	char *p;
	object_flag_type f;
	FLAG *fst;

	/* find thing */
	if ((thing = match_controlled(player, arg1)) == NOTHING)
		return;
	if ((*arg2 == PROP_MUF) && !Wizard(player)) {
		notify(player, player, "Can't set/remove a MUF property.");
		return;
	}

#ifdef TIMESTAMPS
	DBSTORE(thing, time_modified, time(NULL));
#endif

	/* move p past NOT_TOKEN if present */
	for (p = arg2; *p && (*p == NOT_TOKEN || isspace(*p)); p++)
		;

	/* Now we check to see if it's a property reference */
	/* if this gets changed, please also modify boolexp.c */
	if (strchr(arg2, PROP_DELIMITER)) {
		char *type;
		char *class;
		char *x; /* to preserve string location so we can free it */
		char *temp;

		type = dup_string(arg2);
		class = (char *) strchr(type, PROP_DELIMITER);
		x = type;

		while (isspace(*type) && (*type != PROP_DELIMITER))
			type++;
		if (*type == PROP_DELIMITER) {
			if (strlen(type) > 1) /* @set obj = :blah */
			{
				notify(player, player, "Invalid property name.");
				return;
			} else /* @set obj = : */
			{
				/* clear all properties */
				if (Safe(thing)) {
					notify(player, player, "That object is set SAFE.");
					return;
				}
				burn_proptree(DBFETCHPROP(thing));
				DBSTOREPROP(thing, NULL);
				notify(player, player, "ALL properties removed.");
				return;
			}
		} else /* @set obj = prop:val */
		{
			/* get rid of trailing spaces */
			for (temp = class - 1; isspace(*temp); temp--)
				;
			temp++;
			*temp = '\0';
		}
		class++; /* move to next character */
		while (isspace(*class) && *class)
			class++;
		if (!(*class)) {
			switch (remove_property(thing, type, access_rights(player, thing,
					NOTHING))) {
			case 0:
				notify(player, player, "Property removed.");
				break;
			case 1:
				notify(player, player, "Property not found.");
				break;
			case 2:
				notify(player, player, "Permission denied.");
				break;
			}
		} else {
			if (add_property(thing, type, class, default_perms(type),
					access_rights(player, thing, NOTHING))) {
				notify(player, player, "Property set.");
			} else
				notify(player, player, "Permission denied.");
		}
		free((void *) x);
		return;
	}

	/* identify flag */
	if (*p == '\0') {
		notify(player, player, "You must specify a flag to set.");
		return;
	}

	if ((fst = flag_lookup(p, thing)) == NULL) {
		notify(player, player,
				"I don't recognize that flag for that type object.");
		return;
	}

	f = fst->flag;

	/* check for restricted flag */
	if (restricted(player, thing, f)) {
		notify(player, player, "Permission denied.");
		return;
	}

	if ((f & GOD) && (*arg2 == NOT_TOKEN) && (thing == player)) {
		notify(player, player, "Permission denied.");
	}

	if ((f & WIZARD) && *arg2 == NOT_TOKEN && thing == player) {
		notify(player, player, "Permission denied.");
		return;
	}

	/* else everything is ok, do the set */
	if (*arg2 == NOT_TOKEN) {
		if (!(FLAGS(thing) & f)) {
			notify(player, player, "That object was not set %s.", fst->name);
			return;
		}
		/* reset the flag */
		FLAGS(thing) &= ~f;
		notify(player, player, "Flag reset.");
	} else {
		if (FLAGS(thing) & f) {
			notify(player, player, "That object is already set %s.", fst->name);
			return;
		}
		/* set the flag */
		FLAGS(thing) |= f;
		notify(player, player, "Flag set.");
	}DBDIRTY(thing);
}

void do_perms(__DO_PROTO) {
	dbref thing;
	propdir *p;
	char *val;

	if ((thing = match_controlled(player, arg1)) == NOTHING)
		return;

	val = (char *) strchr(arg2, PROP_DELIMITER);
	if (val) {
		*val++ = '\0';
		change_perms(thing, arg2, parse_perms(val), access_rights(player,
				thing, NOTHING));
	}
	p = find_property(thing, arg2, access_rights(player, thing, NOTHING));
	if (p)
		notify(player, player, "Permissions set to:(0%o)%s", p->perms,
				unparse_perms(p->perms));
	else {
		if (find_property(thing, arg2, ACCESS_WI))
			notify(player, player, "Permission denied.");
		else
			notify(player, player, "Property not found.");
	}
}

int ok_taboo_name(dbref player, char *name, int flag) {
	char buf[100]; /*  Match player name against a black list */
	FILE *fp; /*  Written  by Howard                   */

	if ((fp = fopen(TABOO_FILE, "r")) != NULL) {
		while (fgets(buf, 100, fp) != NULL) {
			buf[(strlen(buf) - 1)] = '\0';
			if (wild_match(buf, name)) {
				if (flag)
					log_status("NAME CHANGE FAILED from %s to %s\n",
							NAME(player), name);
				else
					log_status("NAME CREATE FAILED %s\n", name);
				fclose(fp);
				return 0;
			}
		}
		fclose(fp);
		return 1;
	}
	log_status("Can't open TABOO_FILE!!!!!\n");
	fprintf(stderr, "Unable to open %s!\n", TABOO_FILE);
	return 1;
}
