#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "p_conversions.h"
#include "p_interaction.h"
#include "p_logic.h"
#include "p_operands.h"
#include "p_create.h"
#include "p_objects.h"
#include "p_property.h"
#include "p_stack.h"
#include "p_tests.h"
#include "p_time.h"
#include "p_descriptor.h"
#include "p_for.h"

void (*prims_function[])(__P_PROTO) = { PRIMS_CONVERSIONS_FL,
        PRIMS_INTERACTION_FL,
        PRIMS_LOGIC_FL,
        PRIMS_OPERANDS_FL,
        PRIMS_OBJECTS_FL,
        PRIMS_CREATE_FL,
        PRIMS_PROPERTY_FL,
        PRIMS_STACK_FL,
        PRIMS_STRINGS_FL,
        PRIMS_TESTS_FL,

        PRIMS_TIME_FL		,
        PRIMS_DESCRIPTOR_FL, PRIMS_FOR_FL };
/* This package performs the interpretation of mud forth programs.
 It is a basically push pop kinda thing, but I'm making some stuff
 inline for maximum efficiency.

 Oh yeah, because it is an interpreted language, please do type
 checking during this time.  While you're at it, any objects you
 are referencing should be checked against db_top.
 */

/* in cases of boolean expressions, we do return a value, the stuff
 that's on top of a stack when a mud-forth program finishes executing.
 In cases where they don't, leave a value, we just return a 0.  Note:
 this stuff does not return string or whatnot.  It at most can be
 relied on to return a boolean value.

 interp sets up a player's frames and so on and prepares it for
 execution.
 */

extern char *uppercase, *lowercase;
#define UPCASE(x) (uppercase[x])
#define DOWNCASE(x) (lowercase[x])
#ifdef COMPRESS
#define alloc_compressed(x) dup_string(compress(x))
#define get_compress(x) compress(x)
#define get_uncompress(x) uncompress(x)
#else /* COMPRESS */
#define alloc_compressed(x) dup_string(x)
#define get_compress(x) (x)
#define get_uncompress(x) (x)
#endif /* COMPRESS */

/* globally included variables [to save memory] */
char p_buf[BUFFER_LEN];
dbref p_ref;
inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
int p_result;
double p_float;
int p_nargs;
dbref_list *p_drl;

void push(inst *stack1, int *top, int type, voidptr res);

int valid_object(inst *oper);

extern line *read_program(dbref);
extern void free_prog_text(line *);

frame *frame_list = NULL, *frame_list_tail = NULL;
int nextpid = 0;

void kill_on_disconnect(descriptor_data *d) {
	frame *fr = NULL;

	for (fr = frame_list; fr; fr = fr->next) {
		if (fr && (fr->status == STATUS_READ) && (fr->player == d->player))
			fr->status = STATUS_DEAD;
	}
}

int find_a_frame(int frid) {
	frame *fr = NULL;

	for (fr = frame_list; fr; fr = fr->next)
		if (fr && (fr->pid == frid))
			return 1;
	return 0;
}

void bump_frames(char *message, dbref program, dbref player) {
	frame *fr = NULL, *fr_temp = NULL;
	dbref_list *drl = NULL;
	int flag = 0;

	for (fr = frame_list; fr; fr = fr_temp) {
		flag = 0;
		fr_temp = fr->next;
		if (fr->prog == program)
			flag = 1;
		else {
			for (drl = fr->caller; drl; drl = drl->next) {
				if (drl->object == program)
					flag = 1;
			}
		}

		if (flag) {
			if (fr->player != player)
				notify(fr->player, fr->player, message);
			fr->status = STATUS_DEAD;
		}
	}
}

void remove_frame(frame *fr) {
	frame *tmp = NULL;

	if (!frame_list)
		return;
	if (frame_list == fr) {
		frame_list = fr->next;
		if (!frame_list)
			frame_list_tail = NULL;
		return;
	}
	for (tmp = frame_list; tmp->next && (tmp->next != fr); tmp = tmp->next)
		;
	if (tmp->next) {
		if (tmp->next == frame_list_tail)
			frame_list_tail = tmp;
		tmp->next = tmp->next->next;
	}
}

void free_frame(frame *fr) {
	dbref_list *drl = NULL, *temp = NULL;
	for_list *four = NULL, *tfour = NULL;

	if (!fr->writeonly) {
		FLAGS(fr->player) &= ~INTERACTIVE;
		DBSTORE(fr->player, sp.player.pid, 0);
	}
	remove_frame(fr);
	for (drl = fr->caller; drl; temp = drl, drl = drl->next, free(temp))
		;
	for (four = fr->for_loop; four; tfour = four, four = four->next, free(tfour))
		;
	free(fr);
}

frame *find_frame(int pid) {
	frame *fr = NULL;
	for (fr = frame_list; fr && (fr->pid != pid); fr = fr->next)
		;
	return fr;
}

void add_frame(frame *fr) {
	if (!fr)
		return;
	fr->next = NULL;
	if (frame_list == NULL) {
		frame_list = fr;
		frame_list_tail = fr;
	} else {
		frame_list_tail->next = fr;
		frame_list_tail = fr;
	}
}

void notify_frame(dbref player, frame *fr) {
	char status_buf[32], *status = status_buf; // either copy to status_buf or re-point status
	switch (fr->status) {
	case STATUS_RUN: status = "RUN  |-"; break;
	case STATUS_SLEEP:
		snprintf(status_buf, 32, "SLEEP|%- 10ld", fr->sleeptime - time(NULL));
		break;
	case STATUS_READ:
		status = "READ |-";
		break;
	case STATUS_DEAD:
		status =  "DEAD |-";
		break;
	case STATUS_WAIT:
		snprintf(status_buf, 32, "WAIT |%- 10d", fr->waitpid);
		break;
	}
	notify(player, player, "%- 10d|%-16s|%-16s|%s", fr->pid, unparse_name(
			fr->player), DBFETCH(fr->prog)->name);
	notify(player, player, status);
}

void do_ps(__DO_PROTO) {
	frame *fr = NULL;

	notify(player, player,
			"PID       |PLAYER          |PROGRAM         |STATE|TIME/WPID");
	notify(player, player,
			"----------+----------------+----------------+-----+----------");
	if (*arg1) {
		if ((fr = find_frame(strtol(arg1, NULL, 0))))
			notify_frame(player, fr);
	} else {
		for (fr = frame_list; fr; fr = fr->next)
			notify_frame(player, fr);
	}
}

void do_pidkill(__DO_PROTO) {
	frame *fr = NULL;
	int pidnum = 0;

	if ((!*arg1) || ((!isdigit(*arg1)) && (*arg1 != '-'))) {
		notify(player, player, "Usage:@kill <PID>");
		return;
	}

	pidnum = strtol(arg1, NULL, 0);

	switch (pidnum) {
	case -2:
		if (FLAGS(player) & WIZARD) {
			for (fr = frame_list; fr; fr = fr->next)
				fr->status = STATUS_DEAD;
		} else
			notify(player, player, "Permission denied.");
		return;
	case -1:
		for (fr = frame_list; fr; fr = fr->next) {
			if (fr->player == player)
				fr->status = STATUS_DEAD;
		}
		return;
	default:
		if ((fr = find_frame(strtol(arg1, NULL, 0)))) {
			if (controls(player, fr->euid)) {
				if ((FLAGS(fr->player) & INTERACTIVE) && (fr->pid
						== DBFETCH(fr->player)->sp.player.pid))
					FLAGS(fr->player) &= ~INTERACTIVE;
				free_frame(fr);
				notify(player, player, "Process killed.");
			} else
				notify(player, player, "Permission denied.");
		} else
			notify(player, player, "No such process.");
	}
}

void do_go(__DO_PROTO) {
	frame *fr = NULL;

	if (!*arg1) {
		notify(player, player, "Usage:@go <PID>");
		return;
	}

	if ((fr = find_frame(strtol(arg1, NULL, 0)))) {
		if ((FLAGS(player) & WIZARD) || (fr->player == player)) {
			fr->status = STATUS_RUN;
			notify(player, player, "Process forced.");
		} else
			notify(player, player, "Permission denied.");
	} else
		notify(player, player, "No such process.");
}

frame *new_frame(dbref player, dbref program, dbref trigger, dbref location,
		char writeonly) {
	frame *fr = NULL;
	int framecount = 0;

	for (fr = frame_list; fr; fr = fr->next) {
		if (fr->player == player)
			framecount++;
	}

	if (FLAGS(player) & WIZARD) {
		if (framecount > MAX_FRAMES_WIZARD)
			return NULL;
	} else {
		if (framecount > MAX_FRAMES_USER)
			return NULL;
	}

	if (!can_link_to(OWNER(trigger), TYPE_EXIT, program)) {
		notify(player, player, "Program call: Permission denied.");
		return NULL;
	}

	if (DBFETCH(program)->sp.program.start == 0) {
		/* try to compile it... */
		DBSTORE(program, sp.program.first, read_program(program));
		do_compile(player, program);
		free_prog_text(DBFETCH(program)->sp.program.first);
	}

	if (DBFETCH(program)->sp.program.start == 0) {
		/* compile failed... */
		notify(player, player, "Program not compiled. Cannot run.");
		return NULL;
	}

#ifdef TIMESTAMPS
	DBFETCH(program)->time_used = time((long *) NULL);
#endif

	if (!(fr = (frame *) calloc(1, sizeof(frame))))
		return NULL;
	fr->system.top = 1;
	fr->argument.top = 0;
	fr->pc = DBFETCH(program)->sp.program.start;
	fr->writeonly = writeonly;

	/* set basic variables */
	fr->variables[0].type = PROG_OBJECT;
	fr->variables[0].data.objref = player;
	fr->variables[1].type = PROG_OBJECT;
	fr->variables[1].data.objref = location;
	fr->variables[2].type = PROG_OBJECT;
	fr->variables[2].data.objref = trigger;
	fr->for_loop = NULL;
	fr->iterations = 0;
	fr->endless = '0';
	fr->caller = NULL;
	fr->trigger = trigger;
	fr->player = player;
	fr->prog = program;
	fr->euid = FLAGS(program) & STICKY ? OWNER(program) : player;
	fr->wizard = (FLAGS(DBFETCH(fr->prog)->owner) & WIZARD) && (FLAGS(program)
			& WIZARD);
	fr->next = NULL;
	fr->status = STATUS_RUN;

	/* make sure we don't use an already occupied pid */
	while (find_frame(nextpid))
		nextpid++;
	fr->pid = nextpid++;

	if (!fr->writeonly) {
		FLAGS(player) |= INTERACTIVE;
		DBSTORE(player, sp.player.pid, fr->pid);
		DBSTORE(player, curr_prog, NOTHING);
	}
	fr->sleeptime = 0;
	fr->waitpid = 0;
	push(fr->argument.st, &(fr->argument.top), PROG_STRING,
			match_args ? MIPSCAST dup_string(match_args) : NULL);
	return fr;
}

/* clean up the stack. */
void prog_clean(frame *fr) {
	int i = 0;

	fr->system.top = 0;
	for (i = 0; i < fr->argument.top; i++)
		CLEAR(&fr->argument.st[i]);

	for (i = 0; i < MAX_VAR; i++)
		CLEAR(&fr->variables[i]);
	fr->argument.top = 0;
	fr->pc = 0;
}

int false(inst *p) {
	return ((p->type == PROG_STRING && (p->data.string == NULL
			|| !(*p->data.string))) || (p->type == PROG_INTEGER
			&& p->data.number == 0) || (p->type == PROG_FLOAT && p->data.fnum
			== 0) || (p->type == PROG_OBJECT && p->data.objref == NOTHING));
}

static int err;

void copyinst(inst *from, inst *to) {
	*to = *from;
	if (from->type == PROG_STRING && from -> data.string)
		to->data.string = dup_string(from->data.string);
}

#define abort_loop(S) \
{ \
  notify(fr->player, fr->player, S); \
  fr->status = STATUS_DEAD; \
  FLAGS(fr->player) &= ~INTERACTIVE; \
  return 0; \
}

long ilimit = ILIMIT_DEFAULT;

void autostart_frames() {
	dbref i = 0l, pgm = 0l;
	int loop = 0;
	char *val = NULL;

	for (i = 0; i < db_top; i++) {
		if ((Typeof(i) == TYPE_EXIT) && (FLAGS(i) & ABODE) && (FLAGS(i)
				& WIZARD)) {
			val = get_property_data(i, "_autostart", access_rights(OWNER(i), i,
					NOTHING));
			if (val)
				strcpy(match_args, val);
			else
				strcpy(match_args, "");

			for (loop = 0; loop < DBFETCH(i)->sp.exit.ndest; loop++) {
				pgm = DBFETCH(i)->sp.exit.dest[loop];
				if (FLAGS(pgm) & TYPE_PROGRAM)
					add_frame(new_frame(OWNER(pgm), pgm, i,
							DBFETCH(pgm)->location, 1));
			}
		}
	}
}

void run_frames() {
	frame *fr = NULL, *tmp = NULL, *nxt = NULL, *nxt2 = NULL;

	for (fr = frame_list; fr; fr = nxt) {
		nxt = fr->next;
		if ((fr->status == STATUS_SLEEP) && (fr->sleeptime < time(NULL)))
			fr->status = STATUS_RUN;
		if (fr->status == STATUS_RUN)
			run_frame(fr, 0);
		if (fr->status == STATUS_DEAD) {
			for (tmp = frame_list; tmp; tmp = nxt2) {
				nxt2 = tmp->next;
				if ((tmp->status == STATUS_WAIT) && (tmp->waitpid == fr->pid))
					tmp->status = STATUS_RUN; /* Doran says this is a Good Thing (tm) */
			}
			free_frame(fr);
		}
	}
}

int run_frame(frame *fr, int endlessflag) {
	int retval = 0, timeslice = 0;
	dbref_list *drl = NULL;
	dbref obj = 0l;
	inst *temp1 = NULL, *temp2 = NULL;

	if (!fr)
		return 0;
	if (!DBFETCH(fr->prog)->sp.program.code) {
		fr->status = STATUS_DEAD;
		notify(fr->player, fr->player, "Program not compiled.");
		if (FLAGS(fr->player) & INTERACTIVE)
			FLAGS(fr->player) &= ~INTERACTIVE;
		return 0;
	}

	err = 0;

	/* This is the 'natural' way to exit a function */
	while (fr->system.top) {
		if (((timeslice++ == QUANTUM) && !endlessflag && fr->endless == '0')
				|| (fr->status != STATUS_RUN))
			return 0;

		if ((FLAGS(fr->prog) & DARK) && ((fr->pc->type != PROG_PRIMITIVE)
				|| (fr->pc->data.number != IN_NOP))) {
			debug_inst(fr->pc, fr->argument.st, fr->argument.top);
		}

		/* count them iterations */
		if (fr->iterations++ > ilimit)
			abort_loop("Iteration overflow.");

		switch (fr->pc->type) {
		case PROG_INTEGER:
		case PROG_FLOAT:
		case PROG_ADD:
		case PROG_OBJECT:
		case PROG_VAR:
		case PROG_STRING:
			if (fr->argument.top >= STACK_SIZE)
				abort_loop("Program Constant: Stack overflow.")
			;
			copyinst(fr->pc, fr->argument.st + fr->argument.top);
			fr->pc++;
			fr->argument.top++;
			break;
		case PROG_PRIMITIVE:
			/* All pc modifiers and stuff like that should stay here,
			 everything else call with an independent dispatcher.  */
			switch (fr->pc->data.number) {
			case IN_NOP:
				/* don't EVEN ask.  You really don't want to know
				 * and even if you did I'd probably be WAY to embarrased
				 * to tell you about it. Just trust me;  This is a NOP
				 * or NULL OPERATION and needs to be here. --Doran
				 */
				fr->pc++;
				break;
			case IN_IF:
				if (fr->argument.top < 2)
					abort_loop("IF: Stack Underflow.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				temp2 = fr->argument.st + --fr->argument.top;
				if (temp1->type != PROG_ADD) {
					CLEAR(temp2);
					abort_loop("Program internal error: non-address IF. Aborted.");
				}
				if (false(temp2))
					fr->pc = temp1->data.call;
				else
					fr->pc++;
				CLEAR(temp2)
				;
				break;
			case IN_LOOP:
				if (fr->argument.top < 2)
					abort_loop("IF: Stack Underflow.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				temp2 = fr->argument.st + --fr->argument.top;
				if (temp1->type != PROG_ADD) {
					CLEAR(temp2);
					abort_loop("Program internal error: non-address LOOP. Aborted.");
				}
				if (!false(temp2))
					fr->pc = temp1->data.call;
				else
					fr->pc++;
				CLEAR(temp2)
				;
				break;
			case IN_JMP:
				if (fr->argument.top < 1)
					abort_loop("Program internal error: JMP underflow. Aborted.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				if (temp1->type != PROG_ADD)
					abort_loop("Program internal error: non-address JMP. Aborted.")
				;
				fr->pc = temp1->data.call;
				break;
			case IN_EXECUTE:
				if (fr->argument.top < 1)
					abort_loop("Program word: Stack Underflow.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				if (temp1->type != PROG_ADD)
					abort_loop("JMP: Non-address argument.")
				;
				if (fr->system.top >= STACK_SIZE)
					abort_loop("Program word: Stack Overflow")
				;
				fr->system.st[fr->system.top++].data.call = fr->pc + 1;
				fr->pc = temp1->data.call;
				break;
			case IN_CALL:
				if (fr->argument.top < 1)
					abort_loop("CALL: Stack Underflow.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				if (!valid_object(temp1) || Typeof(temp1->data.objref)
						!= TYPE_PROGRAM)
					abort_loop("CALL: Invalid object.")
				;
				obj = temp1->data.objref;
				if ((OWNER(obj) != fr->euid) && !Linkable(obj) && !fr->wizard)
					abort_loop("CALL: permission denied")
				;
				if (fr->system.top >= STACK_SIZE)
					abort_loop("CALL: Stack Overflow")
				;

				if (DBFETCH(obj)->sp.program.start == 0) {
					/* try to compile it... */
					DBSTORE(obj, sp.program.first, read_program(obj));
					do_compile(OWNER(fr->player), obj);
					free_prog_text(DBFETCH(obj)->sp.program.first);
				}
				if (DBFETCH(obj)->sp.program.start == 0)
					abort_loop("Program not compiled.")
				;

				fr->system.st[fr->system.top++].data.call = fr->pc + 1;
				fr->pc = DBFETCH(obj)->sp.program.start;

				drl = (dbref_list *) malloc(sizeof(dbref_list));
				drl->next = fr->caller;
				drl->object = fr->prog;
				fr->caller = drl; /* add this to the caller list, then change */
				fr->prog = obj;
				fr->wizard = (FLAGS(fr->player) & WIZARD) || (FLAGS(fr->prog)
						& WIZARD);
				fr->euid = (FLAGS(fr->prog) & STICKY) ? OWNER(fr->prog)
						: fr->player;
				break;
			case IN_PROGRAM:
				if (fr->argument.top < 1)
					abort_loop("Program internal error: PROGRAM underflow. aborted.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				if (!valid_object(temp1) || Typeof(temp1->data.objref)
						!= TYPE_PROGRAM
						|| (!(DBFETCH(temp1->data.objref)->sp.program.code)))
					abort_loop(
							"Program internal error: PROGRAM invalid arg. aborted.")
				;
				fr->prog = temp1->data.objref;
				drl = fr->caller;
				fr->caller = drl->next;
				free(drl);
				fr->wizard = (FLAGS(fr->player) & WIZARD) || (FLAGS(fr->prog)
						& WIZARD);
				fr->euid = (FLAGS(fr->prog) & STICKY) ? OWNER(fr->prog)
						: fr->player;
				fr->pc++;
				break;
			case IN_RET:
				fr->pc = fr->system.st[--fr->system.top].data.call;
				break;
			case IN_READ:
				if (fr->writeonly)
					abort_loop("READ: Program is write-only.")
				;
				fr->status = STATUS_READ;
				fr->iterations = 0;
				fr->endless = '0';
				fr->pc++;
				break;
			case IN_SLEEP:
				if (endlessflag)
					add_frame(fr);
				if (!fr->writeonly) {
					fr->writeonly = 1;
					FLAGS(fr->player) &= ~INTERACTIVE;
				}
				if (fr->argument.top < 1)
					abort_loop("SLEEP: stack underflow.")
				;
				temp1 = fr->argument.st + --fr->argument.top;
				if (temp1->type != PROG_INTEGER)
					abort_loop("SLEEP: non integer.")
				;
				if (temp1->data.number == 0) {
					fr->status = STATUS_RUN;
					fr->sleeptime = time(NULL);
				} else {
					fr->status = STATUS_SLEEP;
					fr->sleeptime = time(NULL) + temp1->data.number;
				}
				fr->iterations = 0;
				fr->endless = '0';
				fr->pc++;
				break;
			default:
				prims_function[fr->pc->data.number - 1](fr->player, fr->prog,
						fr->pc, fr->argument.st, &(fr->argument.top), fr);
				fr->pc++;
				break;
			} /* switch */
			break;
		default:
			abort_loop(
					"Program internal error: unrecognized instruction. Aborted.")
			;
		} /* switch */
		if (err) {
			prog_clean(fr);
			fr->status = STATUS_DEAD;
			return 0;
		}
	} /* while */

	fr->status = STATUS_DEAD;
	if (fr->argument.top)
		retval = !false(fr->argument.st + fr->argument.top - 1);
	prog_clean(fr);
	return retval;
}

void interp_err(dbref player, char *msg1, char *msg2, dbref program) {
	err++;
	notify(player, player, "Programmer Error.  Tell %s the next message: %s: %s", unparse_name(
			OWNER(program)), msg1, msg2);
}

void push(inst *stack1, int *top, int type, voidptr res) {
	stack1[*top].type = type;

	switch (type) {
	case PROG_VAR:
	case PROG_INTEGER:
		stack1[*top].data.number = *(int *) res;
		break;
	case PROG_FLOAT:
		stack1[*top].data.fnum = *(double *) res;
		break;
	case PROG_STRING:
		stack1[*top].data.string = (char *) res;
		break;
	case PROG_OBJECT:
		stack1[*top].data.objref = *(dbref *) res;
		break;
	}
	(*top)++;
}

int valid_player(inst *oper) {
	return (!(oper->type != PROG_OBJECT || oper->data.objref >= db_top
			|| oper->data.objref < 0 || (Typeof(oper->data.objref)
			!= TYPE_PLAYER)));
}

int valid_object(inst *oper) {
	return (!(oper->type != PROG_OBJECT || oper->data.objref >= db_top
			|| (oper->data.objref >= 0 && Typeof(oper->data.objref)
					== TYPE_GARBAGE) || (oper->data.objref < 0)));
}

int is_home(inst *oper) {
	return (oper->type == PROG_OBJECT && oper->data.objref == HOME);
}

int permissions(dbref player, dbref thing) {
	if (thing == player || thing == HOME)
		return 1;

	switch (Typeof(thing)) {
	case TYPE_PLAYER:
		return 0;
	case TYPE_EXIT:
		return (OWNER(thing) == player || OWNER(thing) == NOTHING);
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_PROGRAM:
		return (OWNER(thing) == player);
	}

	return 0;
}

int arith_type(inst *op1, inst *op2) {
	return ((op1->type == PROG_INTEGER && op2->type == PROG_INTEGER)
			|| (op1->type == PROG_VAR && op2->type == PROG_INTEGER));
}

int float_type(inst *op1, inst *op2) {
	return ((op1->type == PROG_FLOAT && op2->type == PROG_FLOAT) || (op1->type
			== PROG_VAR && op2->type == PROG_FLOAT));
}
