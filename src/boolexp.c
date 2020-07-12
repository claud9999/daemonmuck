#include "copyright.h"
#include "config.h"

#include <ctype.h>

#include "db.h"
#include "match.h"
#include "externs.h"
#include "params.h"
#include "interface.h"

/* Lachesis note on the routines in this package:
 *   eval_boolexp does just evaluation.
 *
 *   parse_boolexp makes potentially recursive calls to several different
 *   subroutines ---
 *          parse_boolexp_F
 *            This routine does the leaf level parsing and the NOT.
 *        parse_boolexp_E
 *            This routine does the ORs.
 *        parse_boolexp_T
 *            This routine does the ANDs.
 *
 *   Because property expressions are leaf level expressions, I have only
 *   touched eval_boolexp_F, asking it to call my additional parse_boolprop()
 *   routine.
 */

int eval_boolexp(dbref player, boolexp *b, dbref thing) {
	frame *fr = NULL;
	int result = 0;

	if (b == TRUE_BOOLEXP)
		return 1;
	else {
		switch (b->type) {
		case BOOLEXP_AND:
			return (eval_boolexp(player, b->sub1, thing) && eval_boolexp(
					player, b->sub2, thing));
		case BOOLEXP_OR:
			return (eval_boolexp(player, b->sub1, thing) || eval_boolexp(
					player, b->sub2, thing));
		case BOOLEXP_NOT:
			return !eval_boolexp(player, b->sub1, thing);
		case BOOLEXP_CONST:
			if (Typeof(b ->thing) == TYPE_PROGRAM) {
				fr = new_frame(player, b->thing, thing,
						DBFETCH(player)->location, 1);
				result = run_frame(fr, 1);
				if (fr && (fr->status != STATUS_SLEEP))
					free_frame(fr);
				return result;
			}
			return (b->thing == player || member(b->thing,
					DBFETCH(player)->contents) || b->thing
					== DBFETCH(player)->location);
		case BOOLEXP_PROP:
			return (has_property(player, b->prop_name, b->prop_data,
					access_rights(player, player, NOTHING)));
		default:
			abort(); /* bad type */
		}
	}
}

boolexp *copy_bool(boolexp *old) {
	boolexp *o = (boolexp *) malloc(sizeof(boolexp));

	if (!old || !o)
		return 0;

	o->type = old->type;

	switch (old ->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
		o->sub1 = copy_bool(old->sub1);
		o->sub2 = copy_bool(old->sub2);
		break;
	case BOOLEXP_NOT:
		o->sub1 = copy_bool(old->sub1);
		break;
	case BOOLEXP_CONST:
		o->thing = old->thing;
		break;
	case BOOLEXP_PROP:
		o->prop_name = dup_string(old->prop_name);
		o->prop_data = dup_string(old->prop_data);
		break;
	default:
		log_status("PANIC: copy_boolexp: Error in boolexp!\n");
		abort();
	}
	return o;
}

/* If the parser returns TRUE_BOOLEXP, you lose */
/* TRUE_BOOLEXP cannot be typed in by the user; use @unlock instead */
static char *parsebuf;
static dbref parse_player;

static void skip_whitespace() {
	while (*parsebuf && isspace(*parsebuf))
		parsebuf++;
}

static boolexp *parse_boolexp_E(void); /* defined below */
static boolexp *parse_boolprop(char *buf); /* defined below */

/* F -> (E); F -> !F; F -> object identifier */
static boolexp *parse_boolexp_F() {
	boolexp *b = NULL;
	char *p = NULL;
	match_data md;
	char buf[BUFFER_LEN];

	skip_whitespace();
	switch (*parsebuf) {
	case '(':
		parsebuf++;
		b = parse_boolexp_E();
		skip_whitespace();
		if (b == TRUE_BOOLEXP || *parsebuf++ != ')') {
			free_boolexp(b);
			return TRUE_BOOLEXP;
		} else
			return b;
	case NOT_TOKEN:
		parsebuf++;
		b = (boolexp *) malloc(sizeof(boolexp));
		b->type = BOOLEXP_NOT;
		b->sub1 = parse_boolexp_F();
		if (b->sub1 == TRUE_BOOLEXP) {
			free((void *) b);
			return TRUE_BOOLEXP;
		} else
			return b;
	default:
		/* must have hit an object ref */
		/* load the name into our buffer */
		p = buf;
		while (*parsebuf && *parsebuf != AND_TOKEN && *parsebuf != OR_TOKEN
				&& *parsebuf != ')')
			*p++ = *parsebuf++;
		/* strip trailing whitespace */
		*p-- = '\0';
		while (isspace(*p))
			*p-- = '\0';

		/* check to see if this is a property expression */
		if (index(buf, PROP_DELIMITER))
			return parse_boolprop(buf);

		b = (boolexp *) malloc(sizeof(boolexp));
		b->type = BOOLEXP_CONST;

		if (strcmp(buf, "#-1") && string_compare(buf, "nothing")) {
			/* do the match */
			init_match(parse_player, buf, TYPE_THING, &md);
			match_neighbor(&md);
			match_possession(&md);
			match_me(&md);
			match_here(&md);
			match_absolute(&md);
			match_player(&md);

			if ((b->thing = match_result(&md)) == NOTHING) {
				notify(parse_player, parse_player, "I don't see %s here.", buf);
				free(b);
				return TRUE_BOOLEXP;
			} else if (b->thing == AMBIGUOUS) {
				notify(parse_player, parse_player, "I don't know which %s you mean!", buf);
				free(b);
				return TRUE_BOOLEXP;
			} else
				return b;
		} else {
			b = (boolexp *) malloc(sizeof(boolexp));
			b->type = BOOLEXP_CONST;
			b->thing = NOTHING;
			return b;
		}
	}
}

/* T -> F; T -> F & T */
static boolexp *parse_boolexp_T() {
	boolexp *b = NULL;
	boolexp *b2 = NULL;

	if ((b = parse_boolexp_F()) == TRUE_BOOLEXP)
		return b;
	else {
		skip_whitespace();
		if (*parsebuf == AND_TOKEN) {
			parsebuf++;

			b2 = (boolexp *) malloc(sizeof(boolexp));
			b2->type = BOOLEXP_AND;
			b2->sub1 = b;
			if ((b2->sub2 = parse_boolexp_T()) == TRUE_BOOLEXP) {
				free_boolexp(b2);
				return TRUE_BOOLEXP;
			} else
				return b2;
		} else
			return b;
	}
}

/* E -> T; E -> T | E */
static boolexp *parse_boolexp_E() {
	boolexp *b = NULL;
	boolexp *b2 = NULL;

	if ((b = parse_boolexp_T()) == TRUE_BOOLEXP)
		return b;
	else {
		skip_whitespace();
		if (*parsebuf == OR_TOKEN) {
			parsebuf++;

			b2 = (boolexp *) malloc(sizeof(boolexp));
			b2->type = BOOLEXP_OR;
			b2->sub1 = b;
			if ((b2->sub2 = parse_boolexp_E()) == TRUE_BOOLEXP) {
				free_boolexp(b2);
				return TRUE_BOOLEXP;
			} else
				return b2;
		} else
			return b;
	}
}

boolexp *parse_boolexp(dbref player, char *buf) {
	parsebuf = buf;
	parse_player = player;
	return parse_boolexp_E();
}

/* parse a property expression
 If this gets changed, please also remember to modify set.c       */
static boolexp *parse_boolprop(char *buf) {
	char *type = NULL, *class = NULL, *x = NULL, *temp = NULL;
	boolexp *b = NULL;

	x = type = dup_string(buf);
	class = (char *) strchr(type, PROP_DELIMITER);
	b = (boolexp *) malloc(sizeof(boolexp));
	b->type = BOOLEXP_PROP;
	b->sub1 = b->sub2 = 0;
	b->thing = NOTHING;
	while (isspace(*type) && (*type != PROP_DELIMITER))
		type++;
	if (*type == PROP_DELIMITER) {
		/* Oops!  No property name!  Clean up and return true. */
		free(x);
		free(b);
		return TRUE_BOOLEXP;
	}
	/* get rid of trailing spaces */
	for (temp = class - 1; isspace(*temp); temp--)
		;
	temp++;
	*temp = '\0';
	class++;
	/* skip over leading spaces */
	while (isspace(*class) && *class)
		class++;
	for (temp = class + strlen(class); isspace(*temp); temp--)
		;
	temp++;
	if (isspace(*temp))
		*temp = '\0';
	if (!*class) {
		/* Oops!  No property data!  Clean up and return true. */
		free(x);
		free(b);
		return TRUE_BOOLEXP;
	}
	b->prop_name = dup_string(type);
	b->prop_data = dup_string(class);
	free(x);
	return b;
}

/* boolean expression dbref global search and replace   */
/* To be used in copies, shifts, etc for sanity et. al. */

void bool_dgsr(boolexp *lock, dbref old, dbref new) {
	switch (lock->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
		bool_dgsr(lock->sub1, old, new);
		bool_dgsr(lock->sub2, old, new);
		break;
	case BOOLEXP_NOT:
		bool_dgsr(lock->sub1, old, new);
		break;
	case BOOLEXP_CONST:
		if (lock->thing == old)
			lock->thing = new;
		break;
	case BOOLEXP_PROP:
		break;
	default:
		log_status("PANIC: bool_dgsr: Error in boolexp!\n");
		abort();
	}
}

void sanitize_lock(boolexp *lock) {
	if (lock != NULL)
		switch (lock->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			sanitize_lock(lock->sub1);
			sanitize_lock(lock->sub2);
			break;
		case BOOLEXP_NOT:
			sanitize_lock(lock->sub1);
			break;
		case BOOLEXP_CONST:
			if (lock->thing < (dbref) 0 || lock->thing >= db_top)
				lock->thing = HOME;
			/* well, it was better than NOTHING */
			break;
		case BOOLEXP_PROP:
			break;
		default:
			log_status("PANIC: sanitize_lock: Error in boolexp!\n");
			abort();
		}
}
