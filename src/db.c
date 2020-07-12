#include "copyright.h"
#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "db.h"
#include "params.h"
#include "interface.h"
#include "externs.h"

object *db = 0;
dbref db_top = 0;
dbref compost_heap = NOTHING;

#ifndef DB_INITIAL_SIZE
#define DB_INITIAL_SIZE 1000
#endif /* DB_INITIAL_SIZE */

#define COUNTER_MAX 10000
#define VALIDATE_LISTS

#ifdef DB_DOUBLING
dbref db_size = DB_INITIAL_SIZE;
#endif /* DB_DOUBLING */

macrotable *macrotop;

extern char *dup_string(char *);
int number(char *s);

char buf[BUFFER_LEN];

/* returns 0 if password matches */
int check_password(char *plaintext, dbref player) {
	char salt[3];

	salt[0] = DBFETCH(player)->sp.player.password[0];
	salt[1] = DBFETCH(player)->sp.player.password[1];
	salt[2] = '\0';

	return (strcmp(DBFETCH(player)->sp.player.password, crypt(plaintext, salt)));
}

char *make_password(char *plaintext) {
	static char salt_chars[] =
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./";
	char salt[3];
	char *tmp = NULL;

	if (!plaintext)
		return NULL;
	salt[0] = salt_chars[random() % 64];
	salt[1] = salt_chars[random() % 64];
	salt[2] = '\0';

	tmp = (char *) malloc(sizeof(char) * 14);
	strcpy(tmp, crypt(plaintext, salt));
	free(plaintext);
	return (tmp);
}

void free_line(line *l) {
	if (l->this_line)
		free((void *) l->this_line);
	free((void *) l);
}

void free_prog_text(line *l) {
	line *next = NULL;

	while (l) {
		next = l->next;
		free_line(l);
		l = next;
	}
}

#ifdef DB_DOUBLING
static void db_grow(dbref newtop) {
	object *newdb;

	if (newtop > db_top) {
		db_top = newtop;
		if (!db) {
			/* make the initial one */
			db_size = DB_INITIAL_SIZE;
			if ((db = (object *) malloc(db_size * sizeof(object))) == 0)
				abort();
		}

		/* maybe grow it */
		if (db_top > db_size) {
			/* make sure it's big enough */
			while (db_top > db_size)
				db_size *= 2;
			if ((newdb = (object *) realloc((void *) db, db_size
					* sizeof(object))) == 0)
				abort();
			db = newdb;
		}
	}
}

#else /* DB_DOUBLING */

static void db_grow(dbref newtop)
{
	object *newdb;

	if(newtop > db_top)
	{
		db_top = newtop;
		if(db)
		{
			if((newdb = (object *)
							realloc((void *) db,
									db_top * sizeof(object))) == 0)
			abort();
			db = newdb;
		}
		else
		{
			/* make the initial one */
			if((db = (object *)
							malloc(DB_INITIAL_SIZE * sizeof(object))) == 0)
			abort();
		}
	}
}
#endif /* DB_DOUBLING */

void db_clear_object(dbref i) {
	DBSTORE(i, name, NULL);
	DBSTORE(i, desc, NULL);
	DBSTORE(i, succ, NULL);
	DBSTORE(i, fail, NULL);
	DBSTORE(i, drop, NULL);
	DBSTORE(i, osucc, NULL);
	DBSTORE(i, ofail, NULL);
	DBSTORE(i, odrop, NULL);
	DBSTOREPROP(i, NULL);
	DBSTORE(i, backlinks, NULL);
	DBSTORE(i, backlocks, NULL);
	DBSTORE(i, exits, NOTHING);
	DBSTORE(i, location, NOTHING);
	DBSTORE(i, contents, NOTHING);
	DBSTORE(i, next, NOTHING);
	DBSTORE(i, nextowned, NOTHING);
	DBSTORE(i, key, TRUE_BOOLEXP);
	DBSTORE(i, link, NOTHING);
	DBSTORE(i, pennies, 0);
#ifdef TIMESTAMPS
	DBSTORE(i, time_created, time(NULL));
	DBSTORE(i, time_modified, DBFETCH(i)->time_created);
	DBSTORE(i, time_used, DBFETCH(i)->time_created);
#endif
}

void add_compost(dbref d) {
	dbref scan = 0l;
	if ((compost_heap == NOTHING) || (d < compost_heap)) {
		/* insert at head of list */
		DBSTORE(d, next, compost_heap);
		compost_heap = d;
	} else {
		for (scan = compost_heap; (DBFETCH(scan)->next != NOTHING)
				&& (DBFETCH(scan)->next < d); scan = DBFETCH(scan)->next)
			;
		DBSTORE(d, next, DBFETCH(scan)->next);
		DBSTORE(scan, next, d);
	}
}

dbref new_object() {
	dbref newobj = 0l;

	if (compost_heap != NOTHING) {
		newobj = compost_heap;
		compost_heap = DBFETCH(newobj)->next;
	} else {
		newobj = db_top;
		db_grow(db_top + 1);
	}

	/* clear it out */
	db_clear_object(newobj);

	return newobj;
}

#define DB_MSGLEN 512

void putref(FILE *f, dbref ref) {
	fprintf(f, "%ld\n", ref);
}

static void putstring(FILE *f, char *s) {
	if (s)
		fputs(s, f);
	putc('\n', f);
}

static void putbool_subexp(FILE *f, boolexp *b) {
	switch (b->type) {
	case BOOLEXP_AND:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(AND_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;
	case BOOLEXP_OR:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(OR_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;
	case BOOLEXP_NOT:
		putc('(', f);
		putc(NOT_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_CONST:
		fprintf(f, "%ld", b->thing);
		break;
	case BOOLEXP_PROP:
		fprintf(f, "[%s:%s]", b->prop_name, b->prop_data);
		break;
	default:
		break;
	}
}

void putboolexp(FILE *f, boolexp *b) {
	if (b != TRUE_BOOLEXP)
		putbool_subexp(f, b);
	putc('\n', f);
}

void macrodump(macrotable *node, FILE *f) {
	if (!node)
		return;
	macrodump(node->left, f);
	putstring(f, node->name);
	putstring(f, node->definition);
	putref(f, node->implementor);
	macrodump(node->right, f);
}

char *file_line(FILE *f) {
	if (!fgets(buf, BUFFER_LEN, f))
		return NULL;
	buf[strlen(buf) - 1] = '\0';
	return dup_string(buf);
}

void foldtree(macrotable *center) {
	int count = 0;
	macrotable *nextcent = center;

	for (; nextcent; nextcent = nextcent->left)
		count++;
	if (count > 1) {
		for (nextcent = center, count /= 2; count--; nextcent = nextcent->left)
			;
		if (center->left)
			center->left->right = NULL;
		center->left = nextcent;
		foldtree(center->left);
	}
	for (count = 0, nextcent = center; nextcent; nextcent = nextcent->right)
		count++;
	if (count > 1) {
		for (nextcent = center, count /= 2; count--; nextcent = nextcent->right)
			;
		if (center->right)
			center->right->left = NULL;
		foldtree(center->right);
	}
}

int macrochain(macrotable *lastnode, FILE *f) {
	char *line1 = NULL, *line2 = NULL;
	macrotable *newmacro = NULL;

	if (!(line1 = file_line(f)))
		return 0;
	line2 = file_line(f);
	newmacro = (macrotable *) new_macro(line1, line2, getref(f));
	/* file_line returns malloc'd strings */
	/* new_macro dup_strings it's arguments, so we need to free */
	if (line1)
		free(line1);
	if (line2)
		free(line2);
	if (!macrotop)
		macrotop = (macrotable *) newmacro;
	else {
		newmacro->left = lastnode;
		lastnode->right = newmacro;
	}
	return (1 + macrochain(newmacro, f));
}

void macroload(FILE *f) {
	int count = 0;

	macrotop = NULL;
	count = macrochain(macrotop, f);
	for (count /= 2; count--; macrotop = macrotop->right);
	foldtree(macrotop);
	return;
}

void write_program(line *first, dbref i) {
	FILE *f = NULL;
	char buf[BUFFER_LEN];

	snprintf(buf, BUFFER_LEN, "muf/%ld.m", i);
	f = fopen(buf, "w");
	if (!f) {
		log_status("Couldn't open file %s!\n", buf);
		return;
	}

	while (first) {
		if (!first->this_line)
			continue;
		fputs(first->this_line, f);
		fputc('\n', f);
		first = first->next;
	}
	fclose(f);
}

int db_write_object(FILE *f, dbref i) {
	int j = 0;
	object *o = DBFETCH(i);

#ifndef STRINGS_ONLY
	putstring(f, NAME(i));
#endif
	putstring(f, o->desc);
#ifndef STRINGS_ONLY
	putref(f, o->location);
	putref(f, o->contents);
	putref(f, o->link);
	putref(f, o->exits);
	putref(f, o->pennies);
	putref(f, o->next);
	putref(f, o->nextowned);
	putref(f, o->owner);
	putboolexp(f, o->key);
#endif
	putstring(f, o->fail);
	putstring(f, o->succ);
	putstring(f, o->drop);
	putstring(f, o->ofail);
	putstring(f, o->osucc);
	putstring(f, o->odrop);
#ifndef STRINGS_ONLY
	putref(f, (FLAGS(i) & ~INTERACTIVE));
#ifdef TIMESTAMPS
	putref(f, o->time_created);
	putref(f, o->time_modified);
	putref(f, o->time_used);
#else
	putref(f, 0);
	putref(f, 0);
	putref(f, 0);
#endif
	putstring(f, "*backlinks*");
	dbreflist_dump(f, o->backlinks);
	putstring(f, "*end backlinks*");
	putstring(f, "*backlocks*");
	dbreflist_dump(f, o->backlocks);
	putstring(f, "*end backlocks*");
	putstring(f, "***Property list start ***");
#endif /* STRINGS_ONLY */

	putproperties(f, i);

#ifndef STRINGS_ONLY
	putstring(f, "***Property list end ***");

	switch (Typeof(i)) {
	case TYPE_EXIT:
		putref(f, o->sp.exit.ndest);
		for (j = 0; j < o->sp.exit.ndest; j++)
			putref(f, (o->sp.exit.dest)[j]);
		break;
	case TYPE_PLAYER:
		putstring(f, o->sp.player.password);
	}
#endif  /* STRINGS_ONLY */
	return 0;
}

/* what a hack!  Let people write lachesis style dumps... */

int db_write_object_lachesis(FILE *f, dbref i) {
	object *o = DBFETCH(i);
	int j = 0;

	putstring(f, NAME(i));
	putstring(f, o->desc);
	putref(f, o->location);
	putref(f, o->contents);
	putref(f, o->next);
	putboolexp(f, o->key);
	putstring(f, o->fail);
	putstring(f, o->succ);
	putstring(f, o->drop);
	putstring(f, o->ofail);
	putstring(f, o->osucc);
	putstring(f, o->odrop);
	putref(f, (FLAGS(i) & ~INTERACTIVE));
	putstring(f, "***Property list start ***");
	putproperties(f, i);
	putstring(f, "***Property list end ***");

	switch (Typeof(i)) {
	case TYPE_THING:
		putref(f, o->link);
		putref(f, o->exits);
		putref(f, OWNER(i));
		putref(f, o->pennies);
		break;
	case TYPE_ROOM:
		putref(f, o->link);
		putref(f, o->exits);
		putref(f, OWNER(i));
		break;
	case TYPE_EXIT:
		putref(f, o->sp.exit.ndest);
		for (j = 0; j < o->sp.exit.ndest; j++)
			putref(f, (o->sp.exit.dest)[j]);
		putref(f, OWNER(i));
		break;
	case TYPE_PLAYER:
		putref(f, o->link);
		putref(f, o->exits);
		putref(f, o->pennies);
		putstring(f, o->sp.player.password);
		break;
	case TYPE_PROGRAM:
		putref(f, OWNER(i));
		break;
	}

	return 0;
}

dbref db_write_lachesis(FILE *f) {
	dbref i;

	fputs("***Lachesis TinyMUCK DUMP Format***\n", f);
	for (i = 0; i < db_top; i++) {
		fprintf(f, "#%ld\n", i);
		db_write_object_lachesis(f, i);
	}
	fputs("***END OF DUMP***\n", f);
	fflush(f);
	return (db_top);
}

dbref db_write(FILE *f) {
	dbref i = 0l;

	fputs("***MULCH***\n", f);
	for (i = 0; i < db_top; i++) {
		fprintf(f, "#%ld\n", i);
		db_write_object(f, i);
	}
	fputs("***END OF DUMP***\n", f);
	fflush(f);
	return (db_top);
}

dbref parse_dbref(char *s) {
	char *p = NULL;
	long x = 0l;

	x = atol(s);
	if (x > 0)
		return x;
	else if (x == 0) {
		/* check for 0 */
		for (p = s; *p; p++) {
			if (*p == '0')
				return 0;
			if (!isspace(*p))
				break;
		}
	}

	/* else x < 0 or s != 0 */
	return NOTHING;
}

dbref getref(FILE *f) {
	fgets(buf, BUFFER_LEN, f);
	return (atol(buf));
}

static char *getstring_noalloc(FILE *f) {
	char c;

	fgets(buf, BUFFER_LEN, f);
	if (strlen(buf) == DB_MSGLEN - 1) {
		/* ignore whatever comes after */
		if (buf[DB_MSGLEN - 2] != '\n')
			while ((c = fgetc(f)) != '\n')
				;
	}

	buf[strlen(buf) - 1] = '\0';
	return buf;
}

#define getstring(x) dup_string(getstring_noalloc(x))

#ifdef COMPRESS
#define alloc_compressed(x) dup_string(compress(x))
#define getstring_compress(x) dup_string(compress(getstring_noalloc(x)))
#else
#define alloc_compressed(x) dup_string(x)
#define getstring_compress(x) getstring(x)
#endif /* COMPRESS */

static boolexp *negate_boolexp(boolexp *b) {
	boolexp *n = NULL;

	/* Obscure fact: !NOTHING == NOTHING in old-format databases! */
	if (b == TRUE_BOOLEXP)
		return TRUE_BOOLEXP;

	n = (boolexp *) malloc(sizeof(boolexp));
	n->type = BOOLEXP_NOT;
	n->sub1 = b;

	return n;
}

/* returns true for numbers of form [ + | - ] <series of digits> */
int number(char *s) {
	if (!s)
		return 0;
	while (isspace(*s))
		s++;
	if (*s == '+' || *s == '-')
		s++;
	if (*s < '0' || *s > '9')
		return 0;
	return 1;
}

static boolexp *getboolexp1(FILE *f) {
	boolexp *b = NULL;
	int c = 0, i = 0;

	c = getc(f);
	switch (c) {
	case '\n':
		ungetc(c, f);
		return TRUE_BOOLEXP;
	case EOF:
		abort();
		break;
	case '(':
		b = (boolexp *) malloc(sizeof(boolexp));
		if ((c = getc(f)) == '!') {
			b->type = BOOLEXP_NOT;
			b->sub1 = getboolexp1(f);
			if (getc(f) != ')')
				goto error;
			return b;
		} else {
			ungetc(c, f);
			b->sub1 = getboolexp1(f);
			switch (c = getc(f)) {
			case AND_TOKEN:
				b->type = BOOLEXP_AND;
				break;
			case OR_TOKEN:
				b->type = BOOLEXP_OR;
				break;
			default:
				goto error;
			}
			b->sub2 = getboolexp1(f);
			if (getc(f) != ')')
				goto error;
			return b;
		}
	case '[':
		/* property type */
		b = (boolexp *) malloc(sizeof(boolexp));
		b->type = BOOLEXP_PROP;
		b->sub1 = b->sub2 = 0;
		for (i = 0; ((c = getc(f)) != PROP_DELIMITER) && (i < BUFFER_LEN); i++)
			buf[i] = c;
		if (i >= BUFFER_LEN && (c != PROP_DELIMITER))
			goto error;
		buf[i] = '\0';
		b->prop_name = dup_string(buf);
		for (i = 0; (c = getc(f)) != ']'; i++)
			buf[i] = c;
		buf[i] = '\0';
		if (i >= BUFFER_LEN && c != ']')
			goto error;
		b->prop_data = dup_string(buf);
		return b;
	case '-':
		/* uncomment if old format db
		 while((c = getc(f)) != '\n') if(c == EOF) abort();
		 ungetc(c, f);
		 return TRUE_BOOLEXP; */
	default:
		/* better be a dbref */
		ungetc(c, f);
		b = (boolexp *) malloc(sizeof(boolexp));
		b->type = BOOLEXP_CONST;
		b->thing = 0;

		i = (c == '-') ? -1 : 1;
		if (c == '-')
			c = getc(f);
		/* NOTE possibly non-portable code */
		/* Will need to be changed if putref/getref change */
		while (isdigit(c = getc(f))) {
			b->thing = b->thing * 10 + c - '0';
		}
		b->thing *= i;
		ungetc(c, f);
		return b;
	}
	error: abort(); /* bomb out */
	return TRUE_BOOLEXP;
}

boolexp *getboolexp(FILE *f) {
	boolexp *b = NULL;

	b = getboolexp1(f);
	if (getc(f) != '\n')
		abort(); /* parse error, we lose */
	return b;
}

void free_boolexp(boolexp *b) {
	if (b != TRUE_BOOLEXP) {
		switch (b->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			free_boolexp(b->sub1);
			free_boolexp(b->sub2);
			free((void *) b);
			break;
		case BOOLEXP_NOT:
			free_boolexp(b->sub1);
			free((void *) b);
			break;
		case BOOLEXP_CONST:
			free((void *) b);
			break;
		case BOOLEXP_PROP:
			free(b->prop_name);
			free(b->prop_data);
			free(b);
			break;
		}
	}
}

void db_free_object(dbref i) {
	object *o = NULL;
	int j = 0;
	inst *code = NULL;

	o = DBFETCH(i);
	if (NAME(i))
		free(NAME(i));
	if (o->desc)
		free(o->desc);
	if (o->succ)
		free(o->succ);
	if (o->fail)
		free(o->fail);
	if (o->drop)
		free(o->drop);
	if (o->ofail)
		free(o->ofail);
	if (o->osucc)
		free(o->osucc);
	if (o->odrop)
		free(o->odrop);
	if (o->key)
		free_boolexp(o->key);
	dbreflist_burn(o->backlinks);
	dbreflist_burn(o->backlocks);
	burn_proptree(o->properties);
	switch (Typeof(i)) {
	case TYPE_EXIT:
		if (o->sp.exit.dest)
			free(o->sp.exit.dest);
		break;
	case TYPE_PLAYER:
		if (o->sp.player.password)
			free(o->sp.player.password);
		break;
	case TYPE_DAEMON:
		break;
	case TYPE_PROGRAM:
		dbreflist_burn(DBFETCH(i)->sp.program.editlocks);
		code = o->sp.program.code;
		if (code) {
			for (j = 0; j < o->sp.program.siz; j++)
				if (code[j].type == PROG_STRING && code[j].data.string)
					free((void *) code[j].data.string);
			free((void *) code);
		}
	}DBDIRTY(i);
}

void db_free() {
	dbref i = 0l;

	if (db) {
		for (i = 0; i < db_top; i++)
			db_free_object(i);
		free((void *) db);
		db = 0;
		db_top = 0;
	}
	clear_players();
	clear_primitives();
	compost_heap = NOTHING;
}

line *get_new_line() {
	line *new = NULL;

	new = (line *) malloc(sizeof(line));
	new->this_line = NULL;
	new->next = NULL;
	new->prev = NULL;
	return new;
}

line *read_program(dbref i) {
	line *first = NULL, *prev = NULL, *new = NULL;
	FILE *f = NULL;
	char buf[BUFFER_LEN];

	first = NULL;
	snprintf(buf, BUFFER_LEN, "muf/%ld.m", i);
	f = fopen(buf, "r");
	if (!f)
		return 0;

	while (fgets(buf, BUFFER_LEN, f)) {
		new = get_new_line();
		buf[strlen(buf) - 1] = '\0';
		new->this_line = dup_string(buf);
		if (!first)
			first = prev = new;
		else {
			prev->next = new;
			new->prev = prev;
			prev = new;
		}
	}

	fclose(f);
	return first;
}

void db_read_object_old(FILE *f, object *o, dbref objno) {
	dbref exits = 0l;
	int pennies = 0;
	char *password = NULL, *gender = NULL;

	NAME(objno) = getstring(f);
	o->desc = getstring_compress(f);
	o->location = getref(f);
	o->contents = getref(f);
	exits = getref(f);
	o->backlinks = NULL;
	o->backlocks = NULL;
	o->next = getref(f);
	o->nextowned = NOTHING;
	o->key = getboolexp(f);
	o->fail = getstring_compress(f);
	o->succ = getstring_compress(f);
	o->ofail = getstring_compress(f);
	o->osucc = getstring_compress(f);
	o->properties = NULL;
	OWNER(objno) = getref(f);
	pennies = getref(f);
	FLAGS(objno) = getref(f);
	/* flags have to be checked for conflict --- if they happen to coincide
	 with chown_ok flags and jump_ok flags, we bump them up to
	 the corresponding HAVEN and ABODE flags                           */
	if (FLAGS(objno) & CHOWN_OK) {
		FLAGS(objno) &= ~CHOWN_OK;
		FLAGS(objno) |= HAVEN;
	}
	if (FLAGS(objno) & JUMP_OK) {
		FLAGS(objno) &= ~JUMP_OK;
		FLAGS(objno) |= ABODE;
	}
	password = getstring(f);
	/* convert GENDER flag to property */
	if (FLAGS(objno) & TYPE_PLAYER) {
		switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
		case GENDER_NEUTER:
			gender = "neuter";
			break;
		case GENDER_FEMALE:
			gender = "female";
			break;
		case GENDER_MALE:
			gender = "male";
			break;
		default:
			gender = "unassigned";
			break;
		}
		add_property(objno, "sex", gender, PERMS_COREAD | PERMS_COWRITE
				| PERMS_OTREAD, ACCESS_CO);
	}
	/* Blast the gender flags (Doran) */
	FLAGS(objno) &= (~GENDER_MASK);

	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		o->key = negate_boolexp(o->key);
		FLAGS(objno) &= ~ANTILOCK;
	}

	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		o->link = exits;
		o->pennies = pennies;
		break;
	case TYPE_ROOM:
		o->link = o->location;
		o->location = NOTHING;
		o->exits = exits;
		break;
	case TYPE_EXIT:
		if (o->location == NOTHING) {
			o->sp.exit.ndest = 0;
			o->sp.exit.dest = NULL;
		} else {
			o->sp.exit.ndest = 1;
			o->sp.exit.dest = (dbref *) malloc(sizeof(dbref));
			(o->sp.exit.dest)[0] = o->location;
		}
		o->location = NOTHING;
		break;
	case TYPE_PLAYER:
		o->link = exits;
		o->exits = NOTHING;
		o->pennies = pennies;
		o->sp.player.password = make_password(password);
		OWNER(objno) = objno;
		add_player(objno);
		break;
	case TYPE_PROGRAM:
		o->sp.program.editlocks = NULL;
		o->link = o->owner;
		break;
	case TYPE_GARBAGE:
		OWNER(objno) = NOTHING;
		add_compost(objno);
		free(NAME(objno));
		free(o->desc);
		NAME(objno) = COMPOST_NAME;
		o->desc = COMPOST_DESC;
		FLAGS(objno) = TYPE_GARBAGE;
		break;
	}
}

void db_read_object_new(FILE *f, object *o, dbref objno) {
	int j = 0;
	char *gender = NULL;

	NAME(objno) = getstring(f);
	o->desc = getstring_compress(f);
	o->location = getref(f);
	o->contents = getref(f);
	o->backlinks = NULL;
	o->backlocks = NULL;
	o->next = getref(f);
	o->nextowned = NOTHING;
	o->key = getboolexp(f);
	o->fail = getstring_compress(f);
	o->succ = getstring_compress(f);
	o->ofail = getstring_compress(f);
	o->osucc = getstring_compress(f);
	o->properties = NULL;
	FLAGS(objno) = getref(f);
	/* flags have to be checked for conflict --- if they happen to coincide
	 with chown_ok flags and jump_ok flags, we bump them up to
	 the corresponding HAVEN and ABODE flags                           */
	if (FLAGS(objno) & CHOWN_OK) {
		FLAGS(objno) &= ~CHOWN_OK;
		FLAGS(objno) |= HAVEN;
	}
	if (FLAGS(objno) & JUMP_OK) {
		FLAGS(objno) &= ~JUMP_OK;
		FLAGS(objno) |= ABODE;
	}
	/* convert GENDER flag to property */
	if (FLAGS(objno) & TYPE_PLAYER) {
		switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
		case GENDER_NEUTER:
			gender = "neuter";
			break;
		case GENDER_FEMALE:
			gender = "female";
			break;
		case GENDER_MALE:
			gender = "male";
			break;
		default:
			gender = "unassigned";
			break;
		}
		add_property(objno, "sex", gender, PERMS_COREAD | PERMS_COWRITE
				| PERMS_OTREAD, ACCESS_CO);
	}
	/* Blast the gender flags (Doran) */
	FLAGS(objno) &= (~GENDER_MASK);

	/* o->password = getstring(f); */
	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		o->key = negate_boolexp(o->key);
		FLAGS(objno) &= ~ANTILOCK;
	}
	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		o->link = getref(f);
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		o->pennies = getref(f);
		break;
	case TYPE_ROOM:
		o->link = getref(f);
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_EXIT:
		o->sp.exit.ndest = getref(f);
		o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * o->sp.exit.ndest);
		for (j = 0; j < o->sp.exit.ndest; j++) {
			(o->sp.exit.dest)[j] = getref(f);
		}
		OWNER(objno) = getref(f);
		break;
	case TYPE_PLAYER:
		o->link = getref(f);
		o->exits = getref(f);
		o->pennies = getref(f);
		o->sp.player.password = getstring(f);
		o->sp.player.password = make_password(o->sp.player.password);
		OWNER(objno) = objno;
		add_player(objno);
		break;
	}
}

void db_read_object_lachesis(FILE *f, object *o, dbref objno) {
	int c = 0, j = 0, prop_flag = 0;
	char *gender = NULL;

	NAME(objno) = getstring(f);
	o->desc = getstring_compress(f);
	o->location = getref(f);
	o->contents = getref(f);
	o->backlinks = NULL;
	o->backlocks = NULL;
	o->next = getref(f);
	o->nextowned = NOTHING;
	o->key = getboolexp(f);
	o->fail = getstring_compress(f);
	o->succ = getstring_compress(f);
	o->drop = getstring_compress(f);
	o->ofail = getstring_compress(f);
	o->osucc = getstring_compress(f);
	o->odrop = getstring_compress(f);
	FLAGS(objno) = getref(f);
	c = getc(f);
	if (c == '*') {
		getproperties(f, objno, 0);
		prop_flag++;
	} else {
		/* do our own getref */
		int sign = 0;
		int i = 0;

		if (c == '-')
			sign = 1;
		else if (c != '+') {
			buf[i] = c;
			i++;
		}

		while ((c = getc(f)) != '\n') {
			buf[i] = c;
			i++;
		}
		buf[i] = '\0';
		j = atol(buf);
		if (sign)
			j = -j;

		/* set gender stuff */
		/* convert GENDER flag to property */
		switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
		case GENDER_NEUTER:
			gender = "neuter";
			break;
		case GENDER_FEMALE:
			gender = "female";
			break;
		case GENDER_MALE:
			gender = "male";
			break;
		default:
			gender = "unassigned";
			break;
		}
		add_property(objno, "sex", gender, PERMS_COREAD | PERMS_COWRITE
				| PERMS_OTREAD, ACCESS_CO);
	}

	/* o->password = getstring(f); */
	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		o->key = negate_boolexp(o->key);
		FLAGS(objno) &= ~ANTILOCK;
	}
	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		o->pennies = getref(f);
		break;
	case TYPE_ROOM:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_EXIT:
		o->sp.exit.ndest = prop_flag ? getref(f) : j;
		o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * (o->sp.exit.ndest));
		for (j = 0; j < o->sp.exit.ndest; j++)
			(o->sp.exit.dest)[j] = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_PLAYER:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		o->pennies = getref(f);
		o->sp.player.password = getstring(f);
		o->curr_prog = NOTHING;
		o->sp.player.insert_mode = 0;
		o->sp.player.password = make_password(o->sp.player.password);
		OWNER(objno) = objno;
		add_player(objno);
		break;
	case TYPE_PROGRAM:
		OWNER(objno) = getref(f);
		FLAGS(objno) &= ~INTERNAL;
		o->link = o->owner;
		o->sp.program.curr_line = 0;
		o->sp.program.code = 0;
		o->sp.program.siz = 0;
		o->sp.program.start = 0;
		o->sp.program.editlocks = NULL;
#ifdef COMPILE_ON_LOAD
		o->sp.program.first = read_program(objno);
		do_compile(NOTHING, objno);
		free_prog_text(o->sp.program.first);
#endif /* COMPILE_ON_LOAD */
		o->sp.program.first = 0;
		break;
	case TYPE_GARBAGE:
		add_compost(objno);
		free(NAME(objno));
		free(o->desc);
		NAME(objno) = COMPOST_NAME;
		o->desc = COMPOST_DESC;
		break;
	}
}

void db_read_object_doran(FILE *f, object *o, dbref objno) {
	int j = 0, c = 0, prop_flag = 0;
	char *gender = NULL;

	NAME(objno) = getstring(f);
	o->desc = getstring_compress(f);
	o->location = getref(f);
	o->contents = getref(f);
	o->backlinks = NULL;
	o->backlocks = NULL;
	o->next = getref(f);
	o->nextowned = NOTHING;
	o->key = getboolexp(f);
	o->fail = getstring_compress(f);
	o->succ = getstring_compress(f);
	o->drop = getstring_compress(f);
	o->ofail = getstring_compress(f);
	o->osucc = getstring_compress(f);
	o->odrop = getstring_compress(f);
	FLAGS(objno) = getref(f);
	o->time_created = getref(f);
	o->time_modified = getref(f);
	o->time_used = getref(f);
	c = getc(f);
	if (c == '*') {
		getproperties(f, objno, 0);
		prop_flag++;
	} else {
		/* do our own getref */
		int sign = 0;
		int i = 0;

		if (c == '-')
			sign = 1;
		else if (c != '+') {
			buf[i] = c;
			i++;
		}

		while ((c = getc(f)) != '\n') {
			buf[i] = c;
			i++;
		}
		buf[i] = '\0';
		j = atol(buf);
		if (sign)
			j = -j;

		/* set gender stuff */
		/* convert GENDER flag to property */
		switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
		case GENDER_NEUTER:
			gender = "neuter";
			break;
		case GENDER_FEMALE:
			gender = "female";
			break;
		case GENDER_MALE:
			gender = "male";
			break;
		default:
			gender = "unassigned";
			break;
		}
		add_property(objno, "sex", gender, PERMS_COREAD | PERMS_COWRITE
				| PERMS_OTREAD, ACCESS_CO);
	}

	/* Blast the gender flags (Doran) */
	FLAGS(objno) &= (~GENDER_MASK);

	/* o->password = getstring(f); */
	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		o->key = negate_boolexp(o->key);
		FLAGS(objno) &= ~ANTILOCK;
	}
	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		o->pennies = getref(f);
		break;
	case TYPE_ROOM:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_EXIT:
		o->sp.exit.ndest = prop_flag ? getref(f) : j;
		o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * (o->sp.exit.ndest));
		for (j = 0; j < o->sp.exit.ndest; j++)
			(o->sp.exit.dest)[j] = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_PLAYER:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		o->pennies = getref(f);
		o->sp.player.password = getstring(f);
		o->curr_prog = NOTHING;
		o->sp.player.insert_mode = 0;
		OWNER(objno) = objno;
		add_player(objno);
		break;
	case TYPE_DAEMON:
		if (prop_flag)
			getref(f); /* just throw away the data */
		getref(f);
		getref(f);
		free(getstring(f));
		recycle((dbref) 1, objno);
		break;
	case TYPE_PROGRAM:
		OWNER(objno) = getref(f);
		FLAGS(objno) &= ~INTERNAL;
		o->sp.program.curr_line = 0;
		o->sp.program.code = 0;
		o->sp.program.siz = 0;
		o->sp.program.start = 0;
		o->link = o->owner;
		o->sp.program.editlocks = NULL;
#ifdef COMPILE_ON_LOAD
		o->sp.program.first = read_program(objno);
		do_compile(NOTHING, objno);
		free_prog_text(o->sp.program.first);
#endif /* COMPILE_ON_LOAD */
		o->sp.program.first = 0;
		break;
	case TYPE_GARBAGE:
		add_compost(objno);
		free(NAME(objno));
		NAME(objno) = COMPOST_NAME;
		free(o->desc);
		o->desc = COMPOST_DESC;
		break;
	}
}

void burn_dbref_list(dbref_list *drl) {
	dbref_list *drl_tmp = NULL;
	for (; drl; drl = drl_tmp) {
		drl_tmp = drl->next;
		free(drl);
	}
}

void db_read_object_daemon(FILE *f, object *o, dbref objno, int permsflag) {
	int j = 0, c = 0, prop_flag = 0;
	char garbagebuf[BUFFER_LEN];

	NAME(objno) = getstring(f);
	o->desc = getstring_compress(f);
	o->location = getref(f);
	o->contents = getref(f);
	o->backlinks = NULL;
	o->backlocks = NULL;
	o->next = getref(f);
	o->nextowned = getref(f);
#ifdef RESET_LISTS
	o->nextowned = NOTHING;
#endif
	o->owner = getref(f);
	o->key = getboolexp(f);
	o->fail = getstring_compress(f);
	o->succ = getstring_compress(f);
	o->drop = getstring_compress(f);
	o->ofail = getstring_compress(f);
	o->osucc = getstring_compress(f);
	o->odrop = getstring_compress(f);
	FLAGS(objno) = getref(f);
	o->time_created = getref(f);
	o->time_modified = getref(f);
	o->time_used = getref(f);
	fgets(garbagebuf, BUFFER_LEN, f);/* getstring_compress(f); Someone kill me */
	o->backlinks = dbreflist_read(f);
	fgets(garbagebuf, BUFFER_LEN, f);/* getstring_compress(f); Someone kill me */
	o->backlocks = dbreflist_read(f);
	c = getc(f);
	if (c == '*') {
		getproperties(f, objno, permsflag);
		prop_flag++;
	} else {
		/* do our own getref */
		int sign = 0;
		int i = 0;

		if (c == '-')
			sign = 1;
		else if (c != '+') {
			buf[i] = c;
			i++;
		}

		while ((c = getc(f)) != '\n') {
			buf[i] = c;
			i++;
		}
		buf[i] = '\0';
		j = atol(buf);
		if (sign)
			j = -j;
	}

	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		o->pennies = getref(f);
		break;
	case TYPE_ROOM:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		break;
	case TYPE_EXIT:
		o->sp.exit.ndest = prop_flag ? getref(f) : j;
		o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * (o->sp.exit.ndest));
		for (j = 0; j < o->sp.exit.ndest; j++)
			(o->sp.exit.dest)[j] = getref(f);
		break;
	case TYPE_PLAYER:
		o->link = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		o->pennies = getref(f);
		o->sp.player.password = getstring(f);
		o->curr_prog = NOTHING;
		o->sp.player.insert_mode = 0;
		add_player(objno);
		break;
	case TYPE_DAEMON:
		recycle((dbref) 1, objno);
		break;
	case TYPE_PROGRAM:
		FLAGS(objno) &= ~INTERNAL;
		o->link = o->owner;
		o->sp.program.curr_line = 0;
		o->sp.program.code = 0;
		o->sp.program.siz = 0;
		o->sp.program.start = 0;
		o->sp.program.editlocks = NULL;
#ifdef COMPILE_ON_LOAD
		o->sp.program.first = read_program(objno);
		do_compile(NOTHING, objno);
		free_prog_text(o->sp.program.first);
#endif /* COMPILE_ON_LOAD */
		o->sp.program.first = 0;
		break;
	case TYPE_GARBAGE:
		add_compost(objno);
		free(NAME(objno));
		NAME(objno) = COMPOST_NAME;
		free(o->desc);
		o->desc = COMPOST_DESC;
		break;
	}
}

void db_read_object_mulch(FILE *f, object *o, dbref objno) {
	char garbagebuf[BUFFER_LEN];
	int c = 0;
	int j = 0;

	o->backlinks = NULL;
	o->backlocks = NULL;
	o->curr_prog = NOTHING;

	NAME(objno) = getstring(f);
	o->desc = getstring_compress(f);
	o->location = getref(f);
	o->contents = getref(f);
	o->link = getref(f);
	o->exits = getref(f);
	o->pennies = getref(f);
	o->next = getref(f);
	o->nextowned = getref(f);
#ifdef RESET_LISTS
	o->nextowned = NOTHING;
#endif
	o->owner = getref(f);
	o->key = getboolexp(f);
	o->fail = getstring_compress(f);
	o->succ = getstring_compress(f);
	o->drop = getstring_compress(f);
	o->ofail = getstring_compress(f);
	o->osucc = getstring_compress(f);
	o->odrop = getstring_compress(f);
	FLAGS(objno) = getref(f);
	if (Typeof(objno) == TYPE_EXIT)
		o->link = NOTHING;
	o->time_created = getref(f);
	o->time_modified = getref(f);
	o->time_used = getref(f);
	fgets(garbagebuf, BUFFER_LEN, f);/* getstring_compress(f); Someone kill me */
	o->backlinks = dbreflist_read(f);
	fgets(garbagebuf, BUFFER_LEN, f);/* getstring_compress(f); Someone kill me */
	o->backlocks = dbreflist_read(f);
	c = getc(f);
	if (c == '*') {
		getproperties(f, objno, 1);
	} else {
		/* do our own getref */
		int sign = 0;
		int i = 0;

		if (c == '-')
			sign = 1;
		else if (c != '+') {
			buf[i] = c;
			i++;
		}

		while ((c = getc(f)) != '\n') {
			buf[i] = c;
			i++;
		}
		buf[i] = '\0';
		j = atol(buf);
		if (sign)
			j = -j;
	}

	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_EXIT:
		o->sp.exit.ndest = getref(f);
		o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * (o->sp.exit.ndest));
		for (j = 0; j < o->sp.exit.ndest; j++)
			(o->sp.exit.dest)[j] = getref(f);
		break;
	case TYPE_PLAYER:
		o->sp.player.password = getstring(f);
		o->sp.player.insert_mode = 0;
		add_player(objno);
		break;
	case TYPE_PROGRAM:
		FLAGS(objno) &= ~INTERNAL;
		o->sp.program.curr_line = 0;
		o->sp.program.code = 0;
		o->sp.program.siz = 0;
		o->sp.program.start = 0;
		o->sp.program.editlocks = NULL;
#ifdef COMPILE_ON_LOAD
		o->sp.program.first = read_program(objno);
		do_compile(NOTHING, objno);
		free_prog_text(o->sp.program.first);
#endif /* COMPILE_ON_LOAD */
		o->sp.program.first = 0;
		break;
	case TYPE_GARBAGE:
		add_compost(objno);
		free(NAME(objno));
		NAME(objno) = COMPOST_NAME;
		free(o->desc);
		o->desc = COMPOST_DESC;
	}
}

void db_chown(dbref thing, dbref newowner) {
	DBSTORE(thing, time_modified, time(NULL));
	remove_ownerlist(thing);
	DBSTORE(thing, owner, newowner);
	add_ownerlist(thing);
}

void add_ownerlist(dbref d) {
	dbref owner = 0l;

	owner = DBFETCH(d)->owner;
	if (owner == d)
		return;
	if (Typeof(owner) == TYPE_PLAYER) {
		DBSTORE(d, nextowned, DBFETCH(owner)->nextowned);
		DBSTORE(owner, nextowned, d);
	} else
		fputs("Error, illegal owner value.\n", stderr);
}

void remove_ownerlist(dbref d) {
	dbref step = 0l;

	for (step = DBFETCH(d)->owner; (DBFETCH(step)->nextowned != NOTHING)
			&& (DBFETCH(step)->nextowned != d); step = DBFETCH(step)->nextowned)
		;

	if (DBFETCH(step)->nextowned == d) {
		DBSTORE(step, nextowned, DBFETCH(DBFETCH(step)->nextowned)->nextowned);
	}

	DBSTORE(d, nextowned, NOTHING);
}

void add_backlocks_parse(dbref src, boolexp *b) {
	if (b) {
		switch (b->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			add_backlocks_parse(src, b->sub1);
			add_backlocks_parse(src, b->sub2);
			break;
		case BOOLEXP_NOT:
			add_backlocks_parse(src, b->sub1);
			break;
		case BOOLEXP_CONST:
			if (b->thing != NOTHING) {
				if (b->thing == HOME) {
					fprintf(stderr, "Object locked to HOME. %ld\n", src);
					exit(999);
				}
				DBSTORE(b->thing, backlocks,
						dbreflist_add(DBFETCH(b->thing)->backlocks, src));
			}
		}
	}
}

void remove_backlocks_parse(dbref src, boolexp *b) {
	if (b) {
		switch (b->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			remove_backlocks_parse(src, b->sub1);
			remove_backlocks_parse(src, b->sub2);
			break;
		case BOOLEXP_NOT:
			remove_backlocks_parse(src, b->sub1);
			break;
		case BOOLEXP_CONST:
			if (b->thing != NOTHING)
				DBSTORE(b->thing, backlocks,
						dbreflist_remove(DBFETCH(b->thing)->backlocks, src))
			;
		}
	}
}

void establish_lists() {
	dbref d = 0l;

	for (d = 0; d < db_top; d++) {
		if (Typeof(d) != TYPE_GARBAGE) {
			add_backlocks_parse(d, DBFETCH(d)->key);
			add_backlinks(d);
			add_ownerlist(d);
		}
	}
}

void reset_lists() {
	dbref d = 0l;

	fputs("Resetting lists...\n", stderr);

	for (d = 0; d < db_top; d++) {
		dbreflist_burn(DBFETCH(d)->backlinks);
		dbreflist_burn(DBFETCH(d)->backlocks);
		DBSTORE(d, backlinks, NULL);
		DBSTORE(d, backlocks, NULL);
		DBSTORE(d, nextowned, NOTHING);
	}
	establish_lists();
}

/* returns object a is linked to b... */
int validate_lists_backlinks(dbref a, dbref b) {
	int c = 0, flag = 0;

	switch (Typeof(a)) {
	case TYPE_PLAYER:
	case TYPE_PROGRAM:
	case TYPE_THING:
		return ((DBFETCH(a)->link == HOME) ? (DBFETCH(OWNER(a))->link == b)
				: (DBFETCH(a)->link == b));
	case TYPE_ROOM:
		return ((DBFETCH(a)->link == HOME) ? 0 : (DBFETCH(a)->link == b));
	case TYPE_EXIT:
		for (c = 0, flag = 0; c < DBFETCH(a)->sp.exit.ndest; c++) {
			flag |= ((DBFETCH(a)->sp.exit.dest[c] == HOME) ? 0
					: (DBFETCH(a)->sp.exit.dest[c] == b));
		}
		return (flag);
	}
	return 0;
}

int validate_lists_backlocks(boolexp *a, dbref b) {
	if (a) {
		switch (a->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			return (validate_lists_backlocks(a->sub1, b)
					|| validate_lists_backlocks(a->sub2, b));
		case BOOLEXP_NOT:
			return (validate_lists_backlocks(a->sub1, b));
		case BOOLEXP_CONST:
			return (a->thing == b);
		default:
			return 0;
		}
	}
	return 0;
}

int validate_lists_blback(dbref a, boolexp *bxp) {
	int flag = 0;
	dbref_list *drl = NULL;

	if (bxp) {
		switch (bxp->type) {
		case BOOLEXP_AND:
		case BOOLEXP_OR:
			return (validate_lists_blback(a, bxp->sub1)
					&& validate_lists_blback(a, bxp->sub2));
		case BOOLEXP_NOT:
			return (validate_lists_blback(a, bxp->sub1));
		case BOOLEXP_CONST:
			flag = 0;

			if (bxp->thing == NOTHING || bxp->thing == HOME)
				return 1;

			for (drl = DBFETCH(bxp->thing)->backlocks; drl; drl = drl->next) {
				if (drl->object == a)
					flag++;
			}
			if (flag == 0)
				return 0;
			return 1;
		}
	}
	return 1;
}

int validate_lists() {
	dbref d = 0l, d2 = 0l;
	dbref_list *drl = NULL;
	int counter = 0, counter2 = 0, flag = 1, i = 0;

	fputs("Validating lists.\n", stderr);
	for (d = 0; d < db_top; d++) {
		if (!(d % 1000))
			fprintf(stderr, "%ld...\n", d);

		if ((Typeof(d) != TYPE_GARBAGE) && (Typeof(OWNER(d)) != TYPE_PLAYER)) {
			flag = 0;
			if (Typeof(d) == TYPE_PLAYER) {
				DBSTORE(d, owner, d);
				fprintf(stderr, "Player %ld doesn't own themself.\n", d);
			} else {
				DBSTORE(d, owner, GOD_DBREF);
				fprintf(stderr, "Object %ld not owned by a player.\n", d);
			}
		}

		/* check backlinks list */
		for (drl = DBFETCH(d)->backlinks, counter = 0; (drl) && (counter
				< COUNTER_MAX); drl = drl->next, counter++) {
			if (!validate_lists_backlinks(drl->object, d)) {
				flag = 0;
				fprintf(stderr, "Illegal backlink connection %ld->%ld\n",
						drl->object, d);
			}
		}

		if (counter == COUNTER_MAX) {
			flag = 0;
			fprintf(stderr, "Possible backlinks loop for object %ld.\n", d);
		}

		/* check backlocks */
		for (drl = DBFETCH(d)->backlocks, counter = 0; (drl) && (counter
				< COUNTER_MAX); drl = drl->next, counter++) {
			if (!validate_lists_backlocks(DBFETCH(drl->object)->key, d)) {
				fprintf(stderr, "Illegal backlock %ld is not locked to %ld.\n",
						drl->object, d);
			}
		}

		if (counter == COUNTER_MAX) {
			flag = 0;
			fprintf(stderr, "Possible backlocks loop on %ld.\n", d);
		}
	}

	for (d = 0; d < db_top; d++) {
		if (Typeof(d) != TYPE_GARBAGE) {
			for (d2 = OWNER(d), counter = counter2 = 0; (d2 != NOTHING)
					&& (counter < COUNTER_MAX); d2 = DBFETCH(d2)->nextowned, counter++) {
				if (d2 == d)
					counter2++;
			}

			if (counter2 != 1) {
				flag = 0;
				fprintf(stderr, "Object %ld in %ld ownerlist %d times.\n", d,
						OWNER(d), counter2);
			}
		}

		counter = 0;
		switch (Typeof(d)) {
		case TYPE_PLAYER:
			/* Check owner lists... */
			for (d2 = d, counter = 0; (d2 != NOTHING)
					&& (counter < COUNTER_MAX); d2 = DBFETCH(d2)->nextowned, counter++) {
				if (OWNER(d2) != d) {
					flag = 0;
					fprintf(stderr, "%ld is not owned by %ld.\n", d2, OWNER(d));
				}
			}

			if (counter == COUNTER_MAX) {
				flag = 0;
				fprintf(stderr, "Possible owner list loop for %ld.\n", OWNER(d));
			}
		case TYPE_THING:
		case TYPE_PROGRAM:
			if (DBFETCH(d)->link != NOTHING) {
				d2 = (DBFETCH(d)->link == HOME) ? DBFETCH(OWNER(d))->link
						: DBFETCH(d)->link;
				for (drl = DBFETCH(d2)->backlinks, counter = 0; drl; drl
						= drl->next) {
					if (drl->object == d)
						counter++;
				}
				if (counter != 1) {
					flag = 0;
					fprintf(stderr,
							"Object %ld in %ld backlink list %d times.\n", d,
							d2, counter);
				}
			} else {
				fprintf(stderr, "Object %ld linked to nothing.\n", d);
				if (Typeof(d) == TYPE_PLAYER) {
					DBSTORE(d, link, DBFETCH(d)->location);
				} else {
					DBSTORE(d, link, OWNER(d));
				}
				flag = 0;
			}
			break;
		case TYPE_ROOM:
			d2 = DBFETCH(d)->link;
			if ((d2 != NOTHING) && (d2 != HOME)) {
				for (drl = DBFETCH(d2)->backlinks; drl; drl = drl->next) {
					if (drl->object == d)
						counter++;
				}
				if (counter != 1) {
					flag = 0;
					fprintf(stderr,
							"Object %ld in %ld backlink list %d times.\n", d,
							d2, counter);
				}
			}
			break;
		case TYPE_EXIT:
			for (i = 0; i < DBFETCH(d)->sp.exit.ndest; i++) {
				d2 = DBFETCH(d)->sp.exit.dest[i];
				if (d2 != HOME) {
					counter = 0;
					for (drl = DBFETCH(d2)->backlinks; drl; drl = drl->next) {
						if (drl->object == d)
							counter++;
					}
					if (counter == 0) {
						flag = 0;
						fprintf(stderr,
								"Object %ld not in %ld backlink list.\n", d, d2);
					}
				}
			}
		}

		if (!validate_lists_blback(d, DBFETCH(d)->key)) {
			fprintf(stderr, "Object %ld not in correct backlock list.\n", d);
			flag = 0;
		}
	}

	return flag;
}

dbref db_read(FILE *f) {
	dbref i = 0l;
	object *o = NULL;
	char *special = NULL;
	int newformat = 0;
	char c = '\0';

	if ((c = getc(f)) == '*') {
		special = getstring(f);
		if (!strcmp(special, "**TinyMUCK DUMP Format***")) {
			newformat = 1;
			fputs("Database in TinyMUCK format.\n", stderr);
		} else if (!strcmp(special, "**Lachesis TinyMUCK DUMP Format***")) {
			newformat = 2;
			fputs("Database in Lachesis format.\n", stderr);
		} else if (!strcmp(special, "**Doran TinyMUCK DUMP Format***")) {
			newformat = 3;
			fputs("Database in Doran format.\n", stderr);
		} else if (!strcmp(special, "**DaemonMUCK DUMP Format***")) {
			newformat = 4;
			fputs("Database in DaemonMUCK format.\n", stderr);
		} else if (!strcmp(special, "**DaemonMUCK P DUMP Format***")) {
			newformat = 5;
			fputs("Database in DaemonMUCK P format.\n", stderr);
		} else if (!strcmp(special, "**MULCH***")) {
			newformat = 6;
			fputs("Database in MULCH format.\n", stderr);
		}
		free((void *) special);
		c = getc(f); /* get next char */
	}

	db_free();
	fputs("Initializing primitives.\n", stderr);
	init_primitives();
	fputs("Reading database.\n", stderr);
	for (i = 0;; i++) {
		if (!(i % 1000))
			fprintf(stderr, "%ld...\n", i);
		switch (c) {
		case '#':
			/* another entry, yawn */
			if (i != getref(f))
				return -1; /* we blew it */

			db_grow(i + 1); /* make space */
			db_clear_object(i);

			o = DBFETCH(i); /* read it in */

			switch (newformat) {
			case 0:
				db_read_object_old(f, o, i);
				break;
			case 1:
				db_read_object_new(f, o, i);
				break;
			case 2:
				db_read_object_lachesis(f, o, i);
				break;
			case 3:
				db_read_object_doran(f, o, i);
				break;
			case 4:
				db_read_object_daemon(f, o, i, 0); /* db sans perms */
				break;
			case 5:
				db_read_object_daemon(f, o, i, 1); /* db with perms */
				break;
			case 6:
				db_read_object_mulch(f, o, i);
			}
			break;
		case '*':
			special = getstring(f);
			if (strcmp(special, "**END OF DUMP***")) {
				free((void *) special);
				fputs("Done.\n", stderr);
				return NOTHING;
			} else {
				free((void *) special);
				if (newformat < 4)
					establish_lists();
#ifdef RESET_LISTS
				reset_lists();
#endif
#ifdef VALIDATE_LISTS
				if (!validate_lists())
					reset_lists();
#endif
				return db_top;
			}
		default:
			return -1;
		}
		c = getc(f);
	} /* for */
} /* db_read */

void add_backlinks(dbref obj) {
	int i = 0;
	dbref tmp = 0l;

	if (obj == NOTHING)
		return;

	switch (Typeof(obj)) {
	case TYPE_EXIT:
		for (i = 0; i < DBFETCH(obj)->sp.exit.ndest; i++) {
			tmp = DBFETCH(obj)->sp.exit.dest[i];
			if (tmp != HOME) {
				DBSTORE(tmp, backlinks, dbreflist_add(DBFETCH(tmp)->backlinks, obj));
			}
		}
		return;
	case TYPE_ROOM:
		tmp = DBFETCH(obj)->link;
		if ((tmp != HOME) && (tmp != NOTHING)) {
			DBSTORE(tmp, backlinks, dbreflist_add(DBFETCH(tmp)->backlinks, obj));
		}
		return;
	default:
		tmp = DBFETCH(obj)->link;
		if (tmp == HOME)
			tmp = DBFETCH(OWNER(obj))->link;
		if (tmp == NOTHING)
			return;
		DBSTORE(tmp, backlinks, dbreflist_add(DBFETCH(tmp)->backlinks, obj))
		;
	}
}

void remove_backlinks(dbref obj) {
	int i = 0;
	dbref tmp = 0l;

	if (obj == NOTHING)
		return;
	switch (Typeof(obj)) {
	case TYPE_EXIT:
		for (i = 0; i < DBFETCH(obj)->sp.exit.ndest; i++) {
			tmp = DBFETCH(obj)->sp.exit.dest[i];
			if (tmp != HOME) {
				DBSTORE(tmp, backlinks,
						dbreflist_remove(DBFETCH(tmp)->backlinks, obj));
			}
		}
		return;
	case TYPE_ROOM:
		tmp = DBFETCH(obj)->link;
		if ((tmp != HOME) && (tmp != NOTHING)) {
			DBSTORE(tmp, backlinks, dbreflist_remove(DBFETCH(tmp)->backlinks, obj));
		}
		return;
	default:
		tmp = DBFETCH(obj)->link;
		if (tmp == HOME)
			tmp = DBFETCH(OWNER(obj))->link;
		if (tmp == NOTHING)
			return;
		DBSTORE(tmp, backlinks, dbreflist_remove(DBFETCH(tmp)->backlinks, obj))
		;
	}
}
