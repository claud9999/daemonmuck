#include "prims.h"
#include "params.h"

/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
static int tmp;
extern int p_nargs;
extern dbref p_ref;
extern char p_buf[BUFFER_LEN];

void prims_addressp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = (p_oper1->type == PROG_ADD);
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_numberp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (p_oper1->type != PROG_STRING || !p_oper1->data.string)
		p_result = 0;
	else
		p_result = number(p_oper1->data.string);
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_playerp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1) && !is_home(p_oper1))
		p_result = 0;
	else {
		p_ref = p_oper1->data.objref;
		p_result = (Typeof(p_ref) == TYPE_PLAYER);
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_thingp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1) && !is_home(p_oper1))
		p_result = 0;
	else {
		p_ref = p_oper1->data.objref;
		p_result = (Typeof(p_ref) == TYPE_THING);
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_roomp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1) && !is_home(p_oper1))
		p_result = 0;
	else {
		p_ref = p_oper1->data.objref;
		p_result = (Typeof(p_ref) == TYPE_ROOM);
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_programp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1) && !is_home(p_oper1))
		p_result = 0;
	else {
		p_ref = p_oper1->data.objref;
		p_result = (Typeof(p_ref) == TYPE_PROGRAM);
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_exitp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1) && !is_home(p_oper1))
		p_result = 0;
	else {
		p_ref = p_oper1->data.objref;
		p_result = (Typeof(p_ref) == TYPE_EXIT);
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_okp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = (valid_object(p_oper1));

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_flagp(__P_PROTO) {
	char *flag;
	FLAG *fst;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument type (2)");
	if (!(p_oper1->data.string))
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Invalid object.");
	p_ref = p_oper2->data.objref;
	tmp = 0;
	p_result = 0;
	flag = p_oper1->data.string;
	if (p_result)
		flag++;

	if ((fst = flag_lookup(flag, p_ref)) != NULL)
		tmp = fst->flag;
	p_result = (tmp && ((FLAGS(p_ref) & tmp) != 0));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_set(__P_PROTO) {
	char *flag;
	FLAG *fst;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument type (2)");
	if (!(p_oper1->data.string))
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Invalid object.");
	p_ref = p_oper2->data.objref;
	tmp = 0;
	p_result = (*p_oper1->data.string == '!');
	flag = p_oper1->data.string;

	if (p_result)
		flag++;
	if ((fst = flag_lookup(flag, p_ref)) != NULL)
		tmp = fst->flag;

	if (!tmp)
		abort_interp("Unrecognized flag.");
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if ((!fr->wizard & ((tmp & DARK) && Typeof(p_ref) != TYPE_ROOM)
			&& (Typeof(p_ref) != TYPE_PROGRAM)) || (tmp & WIZARD)
			|| (tmp & GOD) || (tmp & MUCKER))
		abort_interp("Permission denied.");

	if (!p_result) {
		FLAGS(p_ref) |= tmp;
		DBDIRTY(p_ref);
	} else {
		FLAGS(p_ref) &= ~tmp;
		DBDIRTY(p_ref);
	}

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_stringp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = p_oper1->type;
	p_result = (p_result == PROG_STRING);

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_dbrefp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = p_oper1->type;
	p_result = (p_result == PROG_OBJECT);

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_intp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = p_oper1->type;
	p_result = (p_result == PROG_INTEGER);

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_floatp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = p_oper1->type;
	p_result = (p_result == PROG_FLOAT);

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_varp(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	p_result = p_oper1->type;
	p_result = (p_result == PROG_VAR);

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/****************************************
 * getflags ( d -- i ) - get flags on an object
 ****************************************/
void prims_getflags(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid object.");
	if (!fr->wizard && (fr->euid != OWNER(p_oper1->data.objref)))
		abort_interp("Permission denied.");

	p_result = FLAGS(p_oper1->data.objref);
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_locked(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1) && !is_home(p_oper1))
		abort_interp("Non-object argument.");

	if (DBFETCH(p_oper1->data.objref)->key == NULL)
		p_result = 0;
	else
		p_result = 1;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

static int passlock_depth = 0;

void prims_passlockp(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_OBJECT || p_oper2->type != PROG_OBJECT)
		abort_interp("Non-object argument.");

	passlock_depth++;

	if (passlock_depth < MAX_FRAMES_USER)
		p_result = eval_boolexp(p_oper2->data.objref,
				DBFETCH(p_oper1->data.objref)->key, p_oper1->data.objref);
	else
		p_result = 0; /* fail if too deep */

	passlock_depth--;

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_okplayer(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (!valid_object(p_oper2))
		abort_interp("Invalid object");
	p_ref = p_oper2->data.objref;
	if (Typeof(p_ref) != TYPE_PLAYER)
		abort_interp("Non player object (1)");
	if (!(p_oper1->data.string))
		abort_interp("Empty string argument (2)");
	b = DoNullInd(p_oper1->data.string);

	p_result = 1;
	if (!ok_player_name(b, p_ref))
		p_result = 0;

	if (o_taboonames) {
		if (!ok_taboo_name(p_ref, b, 1))
			p_result = 0;
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_controls(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_OBJECT || p_oper2->type != PROG_OBJECT)
		abort_interp("Non-object argument.");

	p_result = controls(p_oper2->data.objref, p_oper1->data.objref);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_abort(__P_PROTO) {
	char p_buf[BUFFER_LEN];

	CHECKOP(1);
	p_oper1 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument");
	if (!p_oper1->data.string)
		abort_interp("NULL argument");
	strcpy(p_buf, DoNullInd(p_oper1->data.string));
	abort_interp(p_buf);
}

#define ABORT_CHECKARGS(msg) { char zbuf[BUFFER_LEN]; if (*top == stackpos+1) snprintf(zbuf, BUFFER_LEN, "%s (top)", msg); else snprintf(zbuf, BUFFER_LEN, "%s (top-%d)", msg, ((*top)-stackpos-1));  abort_interp(zbuf); }

#define MaxComplexity 18     /* A truly ridiculously high number! */

void prims_checkargs(__P_PROTO) {
	int currpos, stackpos;
	int rngstktop = 0;
	enum {
		itsarange, itsarepeat
	} rngstktyp[MaxComplexity];
	int rngstkpos[MaxComplexity];
	int rngstkcnt[MaxComplexity];

	CHECKOP(1);
	p_oper1 = POP(); /* string argument */
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non string argument.");
	if (!p_oper1->data.string) {
		/* if null string, then no args expected. */
		CLEAR(p_oper1);
		return;
	}
	strcpy(p_buf, p_oper1->data.string); /* copy into local buffer */
	currpos = strlen(p_buf) - 1;
	stackpos = *top - 1;

	while (currpos >= 0) {
		if (isdigit(p_buf[currpos])) {
			if (rngstktop >= MaxComplexity)
				abort_interp("Argument expression ridiculously complex.");
			tmp = 1;
			p_result = 0;
			while ((currpos >= 0) && isdigit(p_buf[currpos])) {
				p_result = p_result + (tmp * (p_buf[currpos] - '0'));
				tmp = tmp * 10;
				currpos--;
			}
			if (p_result == 0)
				abort_interp("Bad multiplier '0' in argument expression.");
			if (p_result >= STACK_SIZE)
				abort_interp("Multiplier too large in argument expression.");
			rngstktyp[rngstktop] = itsarepeat;
			rngstkcnt[rngstktop] = p_result;
			rngstkpos[rngstktop] = currpos;
			rngstktop++;
		} else if (p_buf[currpos] == '}') {
			if (rngstktop >= MaxComplexity)
				abort_interp("Argument expression ridiculously complex.");
			if (stackpos < 0)
				ABORT_CHECKARGS("Stack underflow.");
			if (arg[stackpos].type != PROG_INTEGER)
				ABORT_CHECKARGS("Expected an integer range counter.");
			p_result = arg[stackpos].data.number;
			if (p_result < 0)
				ABORT_CHECKARGS("Range counter should be non-negative.");
			rngstkpos[rngstktop] = currpos - 1;
			rngstkcnt[rngstktop] = p_result;
			rngstktyp[rngstktop] = itsarange;
			rngstktop++;
			currpos--;
			if (p_result == 0) {
				while ((currpos > 0) && (p_buf[currpos] != '{'))
					currpos--;
			}
			stackpos--;
		} else if (p_buf[currpos] == '{') {
			if (rngstktop <= 0)
				abort_interp("Mismatched { in argument expression");
			if (rngstktyp[rngstktop - 1] != itsarange)
				abort_interp("Misformed argument expression.");
			if (--rngstkcnt[rngstktop - 1] > 0) {
				currpos = rngstkpos[rngstktop - 1];
			} else {
				rngstktop--;
				currpos--;
				if (rngstktop && (rngstktyp[rngstktop - 1] == itsarepeat)) {
					if (--rngstkcnt[rngstktop - 1] > 0) {
						currpos = rngstkpos[rngstktop - 1];
					} else {
						rngstktop--;
					}
				}
			}
		} else {
			switch (p_buf[currpos]) {
			case 'i':
				if (stackpos < 0)
					ABORT_CHECKARGS("Stack underflow.")
				;
				if (arg[stackpos].type != PROG_INTEGER)
					ABORT_CHECKARGS("Expected an integer.")
				;
				break;
			case 's':
			case 'S':
				if (stackpos < 0)
					ABORT_CHECKARGS("Stack underflow.")
				;
				if (arg[stackpos].type != PROG_STRING)
					ABORT_CHECKARGS("Expected a string.")
				;
				if (p_buf[currpos] == 'S' && !arg[stackpos].data.string)
					ABORT_CHECKARGS("Expected a non-null string.")
				;
				break;
			case 'd':
			case 'p':
			case 'r':
			case 't':
			case 'e':
			case 'f':
			case 'D':
			case 'P':
			case 'R':
			case 'T':
			case 'E':
			case 'F':
				if (stackpos < 0)
					ABORT_CHECKARGS("Stack underflow.")
				;
				if (arg[stackpos].type != PROG_OBJECT)
					ABORT_CHECKARGS("Expected a dbref.")
				;
				p_ref = arg[stackpos].data.objref;
				if ((p_ref >= db_top) || (p_ref < HOME))
					ABORT_CHECKARGS("Invalid dbref.")
				;
				switch (p_buf[currpos]) {
				case 'D':
					if ((p_ref < 0) && (p_ref != HOME))
						ABORT_CHECKARGS("Invalid dbref.")
					;
					if (Typeof(p_ref) == TYPE_GARBAGE)
						ABORT_CHECKARGS("Invalid dbref.")
					;
				case 'd':
					if (p_ref < HOME)
						ABORT_CHECKARGS("Invalid dbref.")
					;
					break;

				case 'P':
					if (p_ref < 0)
						ABORT_CHECKARGS("Expected player dbref.")
					;
				case 'p':
					if ((p_ref >= 0) && (Typeof(p_ref) != TYPE_PLAYER))
						ABORT_CHECKARGS("Expected player dbref.")
					;
					if (p_ref == HOME)
						ABORT_CHECKARGS("Expected player dbref.")
					;
					break;

				case 'R':
					if ((p_ref < 0) && (p_ref != HOME))
						ABORT_CHECKARGS("Expected room dbref.")
					;
				case 'r':
					if ((p_ref >= 0) && (Typeof(p_ref) != TYPE_ROOM))
						ABORT_CHECKARGS("Expected room dbref.")
					;
					break;

				case 'T':
					if (p_ref < 0)
						ABORT_CHECKARGS("Expected thing dbref.")
					;
				case 't':
					if ((p_ref >= 0) && (Typeof(p_ref) != TYPE_THING))
						ABORT_CHECKARGS("Expected thing dbref.")
					;
					if (p_ref == HOME)
						ABORT_CHECKARGS("Expected player dbref.")
					;
					break;

				case 'E':
					if (p_ref < 0)
						ABORT_CHECKARGS("Expected exit dbref.")
					;
				case 'e':
					if ((p_ref >= 0) && (Typeof(p_ref) != TYPE_EXIT))
						ABORT_CHECKARGS("Expected exit dbref.")
					;
					if (p_ref == HOME)
						ABORT_CHECKARGS("Expected player dbref.")
					;
					break;

				case 'F':
					if (p_ref < 0)
						ABORT_CHECKARGS("Expected program dbref.")
					;
				case 'f':
					if ((p_ref >= 0) && (Typeof(p_ref) != TYPE_PROGRAM))
						ABORT_CHECKARGS("Expected program dbref.")
					;
					if (p_ref == HOME)
						ABORT_CHECKARGS("Expected player dbref.")
					;
					break;
				}
				break;
			case '?':
				if (stackpos < 0)
					ABORT_CHECKARGS("Stack underflow.")
				;
				break;
			case 'v':
				if (stackpos < 0)
					ABORT_CHECKARGS("Stack underflow.")
				;
				if (arg[stackpos].type != PROG_VAR)
					ABORT_CHECKARGS("Expected a variable.")
				;
				break;
			case 'a':
				if (stackpos < 0)
					ABORT_CHECKARGS("Stack underflow.")
				;
				if (arg[stackpos].type != PROG_ADD)
					ABORT_CHECKARGS("Expected a function address.")
				;
				break;
			case ' ':
				/* this is meaningless space.  Ignore it. */
				stackpos++;
				break;
			default:
				abort_interp("Unkown argument type in expression.")
				;
				break;
			}

			currpos--; /* decrement string index */
			stackpos--; /* move on to next stack item down */

			/* are we expecting a repeat of the last argument or range? */
			if ((rngstktop > 0) && (rngstktyp[rngstktop - 1] == itsarepeat)) {
				/* is the repeat is done yet? */
				if (--rngstkcnt[rngstktop - 1] > 0) {
					/* no, repeat last argument or range */
					currpos = rngstkpos[rngstktop - 1];
				} else {
					/* yes, we're done with this repeat */
					rngstktop--;
				}
			}
		}
	} /* while loop */

	if (rngstktop > 0)
		abort_interp("Badly formed argument expression.");
	/* Oops. still haven't finished a range or repeat expression. */

	CLEAR(p_oper1); /* clear link to shared string */
}

void prims_password(__P_PROTO) {
	char *pw;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!(p_oper1->data.string))
		abort_interp("Empty string argument (2)");

	p_ref = p_oper2->data.objref;
	if (Typeof(p_ref) != TYPE_PLAYER)
		abort_interp("Non-player object type.");
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");

	pw = DoNullInd(p_oper1->data.string);
	if (check_password(pw, p_ref))
		p_result = 0;
	else
		p_result = 1;

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	if (!p_result) {
		fr->status = STATUS_SLEEP;
		fr->sleeptime = time(NULL) + PWSLEEP;
	}
}
