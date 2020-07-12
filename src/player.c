#include "copyright.h"
#include "config.h"

#include "db.h"
#include "params.h"
#include "interface.h"
#include "externs.h"

static char buf[BUFFER_LEN];
static hash_tab player_table[PLAYER_HASH_SIZE];

typedef struct player_match {
	char *name;
	struct player_match *next;
	dbref d;
} player_match;

static player_match *player_list = NULL, *pm = NULL, *pm_next = NULL;

dbref lookup_player(char *name) {
	dbref player = NOTHING;
	hash_data *hd = NULL;
	char *current = NULL, *next = NULL;

	for (current = name; current; current = next) {
		next = (char *) strchr(current, EXIT_DELIMITER);
		if (next) {
			strncpy(buf, current, next - current);
			buf[next++ - current] = '\0';
		} else
			strcpy(buf, current);
		if ((hd = find_hash(buf, player_table, PLAYER_HASH_SIZE)))
			player = (player == NOTHING) ? hd->dbval : AMBIGUOUS;
	}
	return player;
}

dbref connect_player(char *name, char *password) {
	dbref player = 0l;

	if ((player = lookup_player(name)) == NOTHING)
		return NOTHING;
	if (DBFETCH(player)->sp.player.password
			&& *DBFETCH(player)->sp.player.password && check_password(password,
			player))
		return NOTHING;
	return player;
}

dbref create_player(char *name, char *password, dbref doer) {
	dbref player = 0l;

	if (!ok_player_name(name, NOTHING) || !ok_password(password))
		return NOTHING;
	if (o_taboonames) {
		if (doer == NOTHING) {
			if (!ok_taboo_name(NOTHING, name, 0))
				return NOTHING;
			else if (!ok_taboo_name(NOTHING, name, 0) && !Wizard(doer))
				return NOTHING;
		}
	}

	/* else he doesn't already exist, create him */
	player = new_object();

	/* initialize everything */
	NAME(player) = dup_string(name);
	DBSTORE(player, location, PLAYER_START);
	DBSTORE(player, link, PLAYER_START);
	add_backlinks(player);
	DBSTORE(player, nextowned, NOTHING);
#ifdef DEFAULT_PFLAGS
	DBSTORE(player, flags, TYPE_PLAYER | DEFAULT_PFLAGS);
#else /* !DEFAULT_PFLAGS */
	DBSTORE(player, flags, TYPE_PLAYER);
#endif /* DEFAULT_PFLAGS */
	DBSTORE(player, owner, player);
	DBSTORE(player, exits, NOTHING);
	DBSTORE(player, pennies, DEFAULT_PENNIES);
	DBSTORE(player, sp.player.password, dup_string(password));
	DBSTORE(player, sp.player.password,
			make_password(DBFETCH(player)->sp.player.password));
	DBSTORE(player, curr_prog, NOTHING);
	DBSTORE(player, sp.player.insert_mode, 0);
	DBSTORE(player, sp.player.pid, 0);

	/* link him to PLAYER_START */
	PUSH(player, DBFETCH(PLAYER_START)->contents);
	add_player(player);
	DBDIRTY(PLAYER_START);

	return player;
}

void do_password(__DO_PROTO) {
	/* nuke .last property */
	add_property(player, ".last", "@password", PERMS_COREAD | PERMS_COWRITE,
			ACCESS_CO);

	if (!DBFETCH(player)->sp.player.password || check_password(arg1, player))
		notify(player, player, "Sorry");
	else if (!ok_password(arg2))
		notify(player, player, "Bad new password.");
	else {
		free(DBFETCH(player)->sp.player.password);
		DBSTORE(player, sp.player.password, dup_string(arg2));

		DBFETCH(player)->sp.player.password = make_password(
				DBFETCH(player)->sp.player.password);

		notify(player, player, "Password changed.");
	}
}

/* nukes all player hashtable stuff */
void clear_players() {
	kill_hash(player_table, PLAYER_HASH_SIZE, 0);
	for (pm = player_list; pm; pm = pm_next) {
		pm_next = pm->next;
		free(pm->name);
		free(pm);
	}
}

void delete_player(dbref who) {
	player_match *pm = NULL, *pm_next = NULL;
	char *current = NULL, *next = NULL;

	for (current = NAME(who); current; current = next) {
		next = (char *) strchr(current, EXIT_DELIMITER);
		if (next) {
			strncpy(buf, current, next - current);
			buf[next++ - current] = '\0';
		} else
			strcpy(buf, current);
		free_hash(buf, player_table, PLAYER_HASH_SIZE);
	}

	while (player_list && (player_list->d == who)) {
		pm_next = player_list->next;
		free(player_list->name);
		free(player_list);
		player_list = pm_next;
	}

	pm = player_list;
	while (pm && pm->next) {
		while (pm && pm->next && (pm->next->d == who)) {
			pm_next = pm->next;
			pm->next = pm_next->next;
			free(pm_next->name);
			free(pm_next);
		}
		if (pm)
			pm = pm->next;
	}
}

void add_player(dbref who) {
	hash_data hd;
	char *current = NULL, *next = NULL;

	for (current = NAME(who); current; current = next) {
		next = (char *) strchr(current, EXIT_DELIMITER);
		if (next) {
			strncpy(buf, current, next - current);
			buf[next++ - current] = '\0';
		} else
			strcpy(buf, current);
		/*  Will this work????? */
		if (buf[0] == '\0')
			return;
		pm = (player_match *) malloc(sizeof(player_match));
		pm->d = who;
		pm->name = dup_string(buf);
		pm->next = player_list;
		player_list = pm;
		hd.dbval = who;
		if (!add_hash(pm->name, hd, player_table, PLAYER_HASH_SIZE))
			panic("Out of memory");
	}
}

int awake_count(dbref player) {
	descriptor_data *d = NULL;
	int retval = 0;

	if (FLAGS(player) & DARK)
		return 0;
	for (d = descriptor_list; d; d = d->next) {
		if (d->connected && d->player == player)
			retval++;
	}
	return retval;
}

void do_player_setuid(__DO_PROTO) {
	descriptor_data *d = NULL;
	dbref new_player = 0l;

	/* nuke .last property */
	add_property(player, ".last", "@user", PERMS_COREAD | PERMS_COWRITE,
			ACCESS_CO);

	new_player = lookup_player(arg1);
	if (new_player == NOTHING) {
		notify(player, player, "Invalid player name. UID remains unchanged.");
		return;
	}

	if (awake_count(player) != 1) {
		notify(player, player,
				"Cannot set UID if you are connected more than once.");
		log_status("UID CHANGE: From %s to %s FAILED, multiple connects.\n",
				NAME(player), NAME(new_player));
		return;
	}

	if (awake_count(new_player) != 0) {
		notify(player, player, "Cannot set UID to that of a connected player.");
		log_status("UID CHANGE: From %s to %s FAILED, already connected.\n",
				NAME(player), NAME(new_player));
		return;
	}

	if ((!DBFETCH(new_player)->sp.player.password || check_password(arg2,
			new_player)) && !controls(player, new_player)) {
		notify(player, player, "Permission denied.");
		log_status("UID CHANGE: From %s to %s FAILED, BAD PASSWORD!!!\n",
				NAME(player), NAME(new_player));

	} else {
		for (d = descriptor_list; d; d = d->next)
			if (d->connected && d->player == player)
				d->player = new_player;
		log_status("UID CHANGE: From %s to %s, successful.\n", NAME(player),
				NAME(new_player));
		notify(new_player, new_player, "UID changed.");

		if (o_notify_wiz)
			notify_wizards("%s has setuid to %s.", unparse_name(player),
					unparse_name(new_player));
	}
	return;
}
