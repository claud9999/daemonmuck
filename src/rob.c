#include "copyright.h"
#include "config.h"

/* rob and kill */

#include "db.h"
#include "params.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "money.h"

void do_rob(__DO_PROTO) {
	dbref thing;
	match_data md;

	init_match(player, arg1, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(player)) {
		match_absolute(&md);
		match_player(&md);
	}
	thing = match_result(&md);

	switch (thing) {
	case NOTHING:
		notify(player, player, "Rob whom?");
		break;
	case AMBIGUOUS:
		notify(player, player, "I don't know who you mean!");
		break;
	default:
		if (Typeof(thing) != TYPE_PLAYER)
			notify(player, player, "Sorry, you can only rob players.");
		else if (DBFETCH(thing)->pennies < 1) {
			notify(player, player, "%s is %sless.", unparse_name(thing),
					S_MONEY);
			notify(thing, thing,
					"%s tried to rob you, but you have no %s to take.",
					unparse_name(player), PL_MONEY);
		} else if (can_doit(player, thing, "Your conscience tells you not to.")) {
			/* steal a penny */
			DBFETCH(player)->pennies++;
			DBDIRTY(player);
			DBFETCH(thing)->pennies--;
			DBDIRTY(thing);
			notify(player, player, "You stole a %s.", S_MONEY);
			notify(thing, thing, "%s stole one of your %s!", unparse_name(
					player), PL_MONEY);
		}
		break;
	}
}

void do_kill(__DO_PROTO) {
	dbref victim = 0l;
	match_data md;
	int cost = 0;

	init_match(player, arg1, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(player)) {
		match_player(&md);
		match_absolute(&md);
	}
	victim = match_result(&md);

	switch (victim) {
	case NOTHING:
		notify(player, player, "I don't see that player here.");
		break;
	case AMBIGUOUS:
		notify(player, player, "I don't know who you mean!");
		break;
	default:
		if (Typeof(victim) != TYPE_PLAYER)
			notify(player, player, "Sorry, you can only kill players.");
		else {
			cost = atoi(arg2);
			if (cost < KILL_MIN_COST)
				cost = KILL_MIN_COST;

			if (FLAGS(DBFETCH(player)->location) & HAVEN) {
				notify(player, player, "You can't kill anyone here!");
				break;
			}

			/* see if it works */
			if (!payfor(player, cost))
				notify(player, player, "You don't have enough %s.", PL_MONEY);
			else if ((random() % KILL_BASE_COST) < cost && !Wizard(victim)) {
				/* you killed him */
				if (GET_DROP(victim))
					notify(player, player, GET_DROP(victim));
				else {
					notify(player, player, "You killed %s!", unparse_name(victim));
				}

				notify_except(player, DBFETCH(player)->location, player, "%s killed %s!%s%s", unparse_name(player), unparse_name(victim), GET_ODROP(victim) ? "  " : "", GET_ODROP(victim) ? pronoun_substitute(player, GET_ODROP(victim)) : "");

				if (DBFETCH(victim)->pennies < MAX_PENNIES) {
					notify(victim, victim, "Your insurance policy pays %d %s.",
							KILL_BONUS, PL_MONEY);
					DBFETCH(victim)->pennies += KILL_BONUS;
					DBDIRTY(victim);
				} else
					notify(victim, victim,
							"Your insurance policy has been revoked.");
				send_home(victim);
			} else {
				/* notify player and victim only */
				notify(player, player, "Your murder attempt failed.");
				notify(victim, victim, "%s tried to kill you!", unparse_name(player));
			}
			break;
		}
	}
}

void do_give(__DO_PROTO) {
	dbref who;
	match_data md;
	int amount;

	if (!arg2 || !*arg2)
		return;

	/* check recipient */
	init_match(player, arg1, NOTYPE, &md);
	match_all_exits(&md);
	match_neighbor(&md);
	match_possession(&md);
	match_absolute(&md);
	/* only Wizards can examine other players */
	if (Wizard(player))
		match_player(&md);
	match_here(&md);

	if ((who = noisy_match_result(&md)) == NOTHING)
		return;

	amount = atoi(arg2);

	if (!Wizard(player)) {
		if (DBFETCH(who)->pennies + amount > MAX_PENNIES) {
			notify(player, player, "That player doesn't need that many %s!",
					PL_MONEY);
			return;
		}
	}

	if (amount < 0 && !Wizard(player)) {
		notify(player, player, "Try using the \"rob\" command.");
		return;
	} else if (amount == 0) {
		notify(player, player, "You must specify a positive number of %s.", PL_MONEY);
		return;
	}

	{
		/* try to do the give to a non object type */
		if (payfor(player, amount)) {
			/* he can do it */
			DBSTORE(who, pennies, DBFETCH(who)->pennies + amount);
			notify(player, player, "You give %d %s to %s.", amount, amount == 1 ? S_MONEY : PL_MONEY, unparse_name(who));
			notify(who, who, "%s gives you %d %s.", unparse_name(player), amount, amount == 1 ? S_MONEY : PL_MONEY);
		} else
			notify(player, player, "You don't have that many %s to give!", PL_MONEY);
	}
}
