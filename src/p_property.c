#include "prims.h"
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result, p_nargs;
extern char p_buf[BUFFER_LEN];
extern dbref p_ref;

void prims_getpropval(__P_PROTO) {
	char *tmp;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper1->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");

	tmp = get_property_data(p_oper2->data.objref, p_oper1->data.string,
			access_rights(player, p_oper2->data.objref, program));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	if (tmp)
		p_result = atoi(tmp);
	else
		p_result = 0;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_getpropstr(__P_PROTO) {
	char *temp;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper1->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");

	temp = get_property_data(p_oper2->data.objref, p_oper1->data.string,
			access_rights(player, p_oper2->data.objref, program));
	temp = get_uncompress(temp);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(temp));
}

void prims_remove_prop(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper1->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");

	remove_property(p_oper2->data.objref, p_oper1->data.string, access_rights(
			player, p_oper2->data.objref, program));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_addprop(__P_PROTO) {
	char *temp;

	CHECKOP(4);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	p_oper4 = POP();

	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument (4)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (3)");
	if (p_oper3->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper3->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper4))
		abort_interp("Non-object argument (1)");

	snprintf(p_buf, BUFFER_LEN, "%ld", p_oper1->data.number);
	temp = get_compress(p_oper2->data.string ? p_oper2->data.string : p_buf);
	if (!add_property(p_oper4->data.objref, p_oper3->data.string, temp,
			default_perms(p_oper3->data.string), access_rights(player,
					p_oper4->data.objref, program)))
		abort_interp("Permission denied.");

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
	CLEAR(p_oper4);
}

void prims_name(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (NAME(p_ref))
		strcpy(p_buf, unparse_name(p_ref));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

/* fullname ( d -- s )...returns all in compound name... */
void prims_fullname(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (NAME(p_ref))
		strcpy(p_buf, NAME(p_ref));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}
void prims_desc(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_DESC(p_ref))
		strcpy(p_buf, get_uncompress(GET_DESC(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_succ(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_SUCC(p_ref))
		strcpy(p_buf, get_uncompress(GET_SUCC(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_fail(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_FAIL(p_ref))
		strcpy(p_buf, get_uncompress(GET_FAIL(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_drop(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_DROP(p_ref))
		strcpy(p_buf, get_uncompress(GET_DROP(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_osucc(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_OSUCC(p_ref))
		strcpy(p_buf, get_uncompress(GET_OSUCC(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_ofail(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_OFAIL(p_ref))
		strcpy(p_buf, get_uncompress(GET_OFAIL(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_odrop(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument type.");

	p_ref = p_oper1->data.objref;
	if (GET_ODROP(p_ref))
		strcpy(p_buf, get_uncompress(GET_ODROP(p_ref)));
	else
		p_buf[0] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_setname(__P_PROTO) {
	char *b;
	CHECKOP(2); /*  Modified to allow player name changes */
	p_oper1 = POP();
	p_oper2 = POP();

	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");

	b = DoNullInd(p_oper1->data.string);
	p_ref = p_oper2->data.objref;

	if (Typeof(p_ref) == TYPE_PLAYER) {
		if (o_player_names) {
			if (!controls(fr->euid, p_ref))
				abort_interp("Permission denied.");
			if (!ok_player_name(b, p_ref))
				abort_interp("Invalid name.");
			if (o_taboonames) {
				if (!ok_taboo_name(p_ref, b, 1))
					abort_interp("Disallowed name.");
			}
			log_status("NAME CHANGE: %s(#%d) to %s\n", NAME(p_ref), p_ref, b);
			snprintf(p_buf, BUFFER_LEN, "%s did a name change:", unparse_name(
					player));
			strcat(p_buf, unparse_name(p_ref));
			strcat(p_buf, "->");
			delete_player(p_ref);
			if (NAME(p_ref))
				free(NAME(p_ref));
			NAME(p_ref) = dup_string(b);
			add_player(p_ref);
			strcat(p_buf, unparse_name(p_ref));
			if (o_notify_wiz) {
				notify_wizards(p_buf);
			}
			DBSTORE(p_ref, time_modified, time(NULL));
		} else {
			abort_interp("Permission denied.");
		}
	} else {
		if (!ok_name(b))
			abort_interp("Invalid name.");
		if (NAME(p_ref))
			free(NAME(p_ref));
		DBSTORE(p_ref, name, dup_string(b));
	}

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setdesc(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");

	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if (GET_DESC(p_ref))
		free((void *) DBFETCH(p_ref)->desc);
	DBSTORE(p_ref, desc, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setsucc(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");

	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if (GET_SUCC(p_ref))
		free((void *) DBFETCH(p_ref)->succ);
	DBSTORE(p_ref, succ, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setfail(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");

	if (GET_FAIL(p_ref))
		free((void *) DBFETCH(p_ref)->fail);
	DBSTORE(p_ref, fail, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setdrop(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");

	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if (GET_DROP(p_ref))
		free((void *) DBFETCH(p_ref)->drop);
	DBSTORE(p_ref, drop, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setosucc(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if (GET_OSUCC(p_ref))
		free((void *) DBFETCH(p_ref)->osucc);
	DBSTORE(p_ref, osucc, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setofail(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if (GET_OFAIL(p_ref))
		free((void *) DBFETCH(p_ref)->ofail);
	DBSTORE(p_ref, ofail, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_setodrop(__P_PROTO) {
	char *b;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	b = DoNullInd(p_oper1->data.string);
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument type (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");

	p_ref = p_oper2->data.objref;
	if (!fr->wizard && !permissions(fr->euid, p_ref))
		abort_interp("Permission denied.");
	if (GET_ODROP(p_ref))
		free((void *) DBFETCH(p_ref)->odrop);
	DBSTORE(p_ref, odrop, alloc_compressed(b));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_pennies(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();

	if (!valid_object(p_oper1))
		abort_interp("Invalid argument.");

	switch (Typeof(p_oper1->data.objref)) {
	case TYPE_PLAYER:
		p_result = DBFETCH(p_oper1->data.objref)->pennies;
		break;
	case TYPE_THING:
		p_result = DBFETCH(p_oper1->data.objref)->pennies;
		break;
	default:
		abort_interp("Invalid argument.")
		;
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_addpennies(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (!valid_object(p_oper2))
		abort_interp("Invalid object.");
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument (2)");

	p_ref = p_oper2->data.objref;
	if (Typeof(p_ref) == TYPE_PLAYER) {
		p_result = p_oper1->data.number;
		if (!fr->wizard) {
			if (p_result + DBFETCH(p_ref)->pennies > MAX_PENNIES)
				abort_interp("Would exceed MAX_PENNIES.");
			if (p_result < 0)
				abort_interp("Amount would be negative.");
			if (!payfor(DBFETCH(fr->prog)->owner, p_oper1->data.number))
				abort_interp("Out of money.");
		}
		DBSTORE(p_ref, pennies, DBFETCH(p_ref)->pennies + p_result);
	} else if (Typeof(p_ref) == TYPE_THING) {
		if (!fr->wizard)
			abort_interp("Permission denied.");
		p_result = DBFETCH(p_ref)->pennies + p_oper1->data.number;
		if (p_result < 1)
			abort_interp("Result must be positive.");
		DBFETCH(p_ref)->pennies += p_oper1->data.number;
		DBDIRTY(p_ref);
	} else
	abort_interp("Invalid object type.");

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

/*
 SETPROP ( d s s -- )
 d - object
 s - property
 s - value to be set to
 */
void prims_setprop(__P_PROTO) {
	char *temp;

	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (3)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper2->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper3))
		abort_interp("Non-object argument (1)");

	temp = get_compress(p_oper1->data.string ? p_oper1->data.string : "");
	add_property(p_oper3->data.objref, p_oper2->data.string, temp,
			default_perms(p_oper2->data.string), access_rights(player,
					p_oper3->data.objref, program));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

/*
 SETPERMS ( d s i -- )
 d - object
 s - property
 i - permissions
 */
void prims_setperms(__P_PROTO) {
	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();

	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-number argument (3)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper2->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper3))
		abort_interp("Non-object argument (1)");

	change_perms(p_oper3->data.objref, p_oper2->data.string,
			p_oper1->data.number, access_rights(player, p_oper3->data.objref,
					program));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

/*
 PERMS ( d s -- i )
 d - object
 s - property
 i - permissions
 */
void prims_perms(__P_PROTO) {
	propdir *p;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper1->data.string)
		abort_interp("Empty string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");

	p = find_property(p_oper2->data.objref, p_oper1->data.string,
			access_rights(player, p_oper3->data.objref, program));
	p_result = (p) ? p->perms : 0;

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/*
 nextprop? ( d s -- i )
 d - object
 s - property
 i - if property ends in /, does property have children?  if not, is there
 one next in line?
 */
void prims_nextpropp(__P_PROTO) {
	int i;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");

	i = strlen(p_oper1->data.string);

	p_result = has_next_property(p_oper2->data.objref, p_oper1->data.string,
			access_rights(player, p_oper3->data.objref, program),
			i ? p_oper1->data.string[i - 1] == '/' : 0);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_nextprop(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");

	next_property(p_buf, p_oper2->data.objref, p_oper1->data.string,
			access_rights(player, p_oper2->data.objref, program));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_is_propdir(__P_PROTO) {
	CHECKOP(2);
	p_oper2 = POP(); /* prop name */
	p_oper1 = POP(); /* dbref */

	if (p_oper1->type != PROG_OBJECT)
		abort_interp("Non dbref argument (1)");
	if (!valid_object(p_oper1))
		abort_interp("Invalid dbref (1)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non string argument (2)");
	if (!p_oper2->data.string)
		abort_interp("Null string not allowed. (2)");

	p_ref = p_oper1->data.objref;
	strcpy(p_buf, p_oper2->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);

	p_result = is_propdir(player, p_ref, p_buf, program);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}
