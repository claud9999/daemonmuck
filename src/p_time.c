#include "prims.h"
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
extern char p_buf[BUFFER_LEN];
static time_t lt;
static struct tm *tm;
extern int p_nargs;
extern dbref p_ref;

#define PUSHINT(i) do{result=(i);push(arg,top,PROG_INTEGER,MIPSCAST &result);}while(0);

void prims_gmtoffset(__P_PROTO) {
	if ((*top) >= STACK_SIZE)
		abort_interp("Stack overflow.");
	lt = time((long *) 0);
	tm = localtime(&lt);
#ifndef HAVE_GMTOFFSET
	abort_interp("GMT offset not supported");
#else
	p_result = tm->tm_gmtoff;
#endif
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_systime(__P_PROTO) {
	if ((*top) >= STACK_SIZE)
		abort_interp("Stack overflow.");
	p_result = time((long *) NULL);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_ctime(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type == PROG_STRING)
		abort_interp("Invalid argument.");

	snprintf(p_buf, BUFFER_LEN, "%s", asctime(localtime(&p_oper1->data.number)));
	p_buf[strlen(p_buf) - 1] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_time(__P_PROTO) {
	if ((*top) + 2 >= STACK_SIZE)
		abort_interp("Stack overflow.");

	lt = time((long *) NULL);
	tm = localtime(&lt);

	p_result = tm->tm_sec;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = tm->tm_min;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = tm->tm_hour;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_date(__P_PROTO) {
	if ((*top) + 2 >= STACK_SIZE)
		abort_interp("Stack overflow.");

	lt = time((long *) NULL);
	tm = localtime(&lt);

	p_result = tm->tm_mday;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_mon) + 1;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_year) + 1900;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_gmtsplit(__P_PROTO) {
	time_t t;
	struct tm *tm;

	CHECKOP(1);
	p_oper1 = POP();

	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Invalid argument.");
	if ((*top) + 8 > STACK_SIZE)
		abort_interp("Stack overflow.");

	t = p_oper1->data.number;
	CLEAR(p_oper1);
	tm = gmtime(&t);

	p_result = (tm->tm_sec);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_min);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_hour);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_mday);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_mon + 1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_year + 1900);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_wday + 1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_yday + 1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_timesplit(__P_PROTO) {
	time_t t;
	struct tm *tm;

	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Invalid argument.");
	if ((*top) + 8 > STACK_SIZE)
		abort_interp("Stack overflow.");

	t = p_oper1->data.number;
	CLEAR(p_oper1);
	tm = localtime(&t);

	p_result = (tm->tm_sec);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_min);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_hour);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_mday);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_mon + 1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_year + 1900);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_wday + 1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	p_result = (tm->tm_yday + 1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_strftime(__P_PROTO) {
	CHECKOP(2);
	p_oper2 = POP(); /* integer: time */
	p_oper1 = POP(); /* string: format */

	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument (1)");
	if (!p_oper1->data.string)
		abort_interp("Illegal NULL string (1)");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Invalid argument (2)");

	tm = localtime((time_t *) (&(p_oper2->data.number)));
	if (!format_time(p_buf, BUFFER_LEN, p_oper1->data.string, tm))
		abort_interp("Operation would result in overflow.");

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

#ifdef TIMESTAMPS
void prims_touch(__P_PROTO) {
	CHECKOP(1);
	p_oper2 = POP();
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid object.");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Argument 2 is not an integer.");

	p_ref = p_oper1->data.objref;

	DBSTORE(p_ref, time_used,
			fr->wizard ? p_oper2->data.number : time((time_t *)NULL));

	CLEAR(p_oper1);
}

void prims_touch_created(__P_PROTO) {
	CHECKOP(1);
	p_oper2 = POP();
	p_oper1 = POP();
	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (!valid_object(p_oper1))
		abort_interp("Invalid object.");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Argument 2 is not an integer.");

	p_ref = p_oper1->data.objref;

	DBSTORE(p_ref, time_created, p_oper2->data.number);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_touch_modified(__P_PROTO) {
	CHECKOP(1);
	p_oper2 = POP();
	p_oper1 = POP();
	if (!fr->wizard)
		abort_interp("Permission denied.");
	if (!valid_object(p_oper1))
		abort_interp("Invalid object.");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Argument 2 is not an integer.");

	p_ref = p_oper1->data.objref;

	DBSTORE(p_ref, time_modified, p_oper2->data.number);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_time_created(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument.");

	p_result = DBFETCH(p_oper1->data.objref)->time_created;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_time_modified(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument.");

	p_result = DBFETCH(p_oper1->data.objref)->time_modified;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_time_used(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument.");

	p_result = DBFETCH(p_oper1->data.objref)->time_used;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}
#endif /*TIMESTAMPS*/
