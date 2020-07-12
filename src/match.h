#ifndef MATCH_H
#define MATCH_H

#include "copyright.h"
#include "db.h"

typedef struct match_data {
	dbref exact_match; /* holds result of exact match */
	int check_keys; /* if non-zero, check for keys */
	dbref last_match; /* holds result of last match */
	int match_count; /* holds total number of inexact matches */
	dbref match_who; /* player who is being matched around */
	char *match_name; /* name to match */
	int preferred_type; /* preferred type */
	int longest_match; /* longest matched string */
} match_data;

/* match functions */
/* Usage: init_match(player, name, type); match_this(); match_that(); ... */
/* Then get value from match_result() */

/* initialize matcher */
void init_match(dbref player, char *name, int type, match_data *md);
void init_match_check_keys(dbref player, char *name, int type, match_data *md);

/* match (LOOKUP_TOKEN)player */
void match_player(match_data *md);

/* match (NUMBER_TOKEN)number */
void match_absolute(match_data *md);

void match_me(match_data *md);

void match_here(match_data *md);

void match_home(match_data *md);

/* match something player is carrying */
void match_possession(match_data *md);

/* match something in the same room as player */
void match_neighbor(match_data *md);

/* match an exit from player's room */
void match_room_exits(dbref loc, match_data *md);

/* match an action attached to a room object */
void match_roomobj_actions(match_data *md);

/* match an action attached to an object in inventory */
void match_invobj_actions(match_data *md);

/* match an action attached to a player */
void match_player_actions(match_data *md);

/* match 4 above */
void match_all_exits(match_data *md);

/* only used for rmatch */
void match_rmatch(dbref, match_data *md);

/* all of the above, except only Wizards do match_absolute and match_player */
void match_everything(match_data *md);

/* return match results */
dbref match_result(match_data *md); /* returns AMBIGUOUS for
 multiple inexacts */
dbref last_match_result(match_data *md); /* returns last
 result */

#define NOMATCH_MESSAGE "I don't see that here."
#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"

dbref noisy_match_result(match_data *md);
/* wrapper for match_result */
/* noisily notifies player */
/* returns matched object or NOTHING */

void match_registered(struct match_data *md);

#endif /* MATCH_H */
