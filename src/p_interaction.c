#include "prims.h"
#include "config.h"

/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_nargs;
extern int p_result;
extern frame *frame_list;
extern dbref p_ref;
extern char p_buf[BUFFER_LEN];

line *get_new_line();
void do_insert_quit(dbref player, dbref p_ref, int flag);

void prims_notify(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");

	if (p_oper1->data.string)
		notify(player, p_oper2->data.objref, p_oper1->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_notify_nolisten(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (!valid_object(p_oper2))
		abort_interp("Non-object argument (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper1->data.string)
		abort_interp("Null string argument (2)");
	if (!fr->wizard)
		abort_interp("Permission denied.");

	notify_nolisten(p_oper2->data.objref, p_oper1->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_notify_except(__P_PROTO) {
	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	if (!valid_object(p_oper3))
		abort_interp("Invalid argument (1)");
	if (p_oper2->type != PROG_OBJECT)
		abort_interp("Invalid object argument (2)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (3)");

	if (p_oper1->data.string)
		notify_except(player, p_oper3->data.objref, p_oper2->data.objref,
				p_oper1->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

void prims_notify_except_nolisten(__P_PROTO) {
	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	if (!valid_object(p_oper3))
		abort_interp("Invalid argument (1)");
	if (p_oper2->type != PROG_OBJECT)
		abort_interp("Invalid object argument (2)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (3)");
	if (!p_oper1->data.string)
		abort_interp("Null string argument (3)");
	if (!fr->wizard)
		abort_interp("Permission denied.");

	notify_except_nolisten(player, p_oper3->data.objref, p_oper2->data.objref,
			p_oper1->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

/* FORCE ( d s -- ) <- forces a dbref to do an action */
void prims_force(__P_PROTO) {
	char str[BUFFER_LEN];
	dbref who;

	CHECKOP(2);
	p_oper2 = POP();
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument (1)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!p_oper2->data.string)
		abort_interp("Empty string argument (2)");

	if (!controls(fr->euid, p_oper1->data.objref) && (!(FLAGS(program) & GOD)
			&& !(FLAGS(OWNER(program)) & GOD)))
		abort_interp("Permission denied.");

	who = p_oper1->data.objref;
	if (FLAGS(who) & INTERACTIVE)
		abort_interp("Object is in interactive mode.");
	strcpy(str, p_oper2->data.string);

	process_command(who, str, who);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

/* From here on out are prims to control/get info on running MUF processes */

void prims_processes(__P_PROTO) {
	int frames;
	frame *fr1;

	for (fr1 = frame_list, frames = 0; fr1; fr1 = fr1->next) {
		if (*top >= STACK_SIZE)
			abort_interp("Stack Overflow!");
		p_result = fr1->pid;
		push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
		frames++;
	}
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow!");
	push(arg, top, PROG_INTEGER, MIPSCAST &frames);
}

void prims_pidkill(__P_PROTO) {
	frame *fr1;
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Operand not an integer.");

	switch (p_oper1->data.number) {
	case -2:
		if (fr->wizard) {
			for (fr1 = frame_list; fr1; fr1 = fr1->next)
				fr1->status = STATUS_DEAD;
		} else
		abort_interp("Permission denied.")
		;
		return;
	case -1:
		for (fr1 = frame_list; fr1; fr1 = fr1->next) {
			if (fr1->player == player)
				fr1->status = STATUS_DEAD;
		}
		return;
	default:
		if ((fr1 = find_frame(p_oper1->data.number))) {
			if (controls(player, fr1->euid)) {
				if ((FLAGS(fr1->player) & INTERACTIVE) && (fr1->pid
						== DBFETCH(fr1->player)->sp.player.pid))
					FLAGS(fr1->player) &= ~INTERACTIVE;
				free_frame(fr1);
				CLEAR(p_oper1);
			} else
			abort_interp("Permission denied.");
		} else
		abort_interp ("No such process.")
		;
	}
}

void prims_go(__P_PROTO) {
	frame *fr1;
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Operand not an integer.");

	if ((fr1 = find_frame(p_oper1->data.number))) {
		if ((fr->wizard) || (fr1->player == player)) {
			fr1->status = STATUS_RUN;
		} else
		abort_interp("Permission denied.");
	} else
	abort_interp ("No such process.");
}

void prims_sleeptime(__P_PROTO) {
	frame *fr1;
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Operand not an integer.");

	if ((fr1 = find_frame(p_oper1->data.number))) {
		if (fr1->status == STATUS_SLEEP)
			p_result = (fr1->sleeptime - time(NULL));
		else
			p_result = 0;

		push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	} else
	abort_interp ("No such process.");
}

void prims_piddbref(__P_PROTO) {
	frame *fr1;
	CHECKOP(1);
	p_oper1 = POP();
	if (!p_oper1->type == PROG_INTEGER)
		abort_interp("Operand not an integer.");

	if ((fr1 = find_frame(p_oper1->data.number)))
		p_ref = fr1->prog;
	else
		p_ref = (dbref) 0;

	push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
	CLEAR(p_oper1);
}

void prims_pid(__P_PROTO) {
	p_result = fr->pid;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_isapid(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument");
	p_result = find_a_frame(p_oper1->data.number);
	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_foreground(__P_PROTO) {
	if (fr->wizard)
		fr->endless = '1';
	else
	abort_interp("Permission denied.");
}

void prims_compile(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument (1)");

	p_ref = p_oper1->data.objref;
	if (Typeof(p_ref) != TYPE_PROGRAM)
		abort_interp("Non-program argument");
	if (!fr->wizard || !controls(player, p_ref))
		abort_interp("Permission denied.");

	/* try to compile it... */
	DBSTORE(p_ref, sp.program.first, read_program(p_ref));
	do_compile(player, p_ref);
	free_prog_text(DBFETCH(p_ref)->sp.program.first);

	if (DBFETCH(p_ref)->sp.program.start == 0)
		p_result = 0;
	else
		p_result = DBFETCH(p_oper1->data.objref)->sp.program.siz;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_uncompile(__P_PROTO) {
	char buf[200];

	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Invalid argument (1)");
	p_ref = p_oper1->data.objref;
	if (Typeof(p_ref) != TYPE_PROGRAM)
		abort_interp("Non-program argument");
	if (!fr->wizard || !controls(player, p_ref))
		abort_interp("Permission denied.");

	snprintf(buf, BUFFER_LEN, "Program %s uncompiled by %s", unparse_name(p_ref),
			unparse_name(player));
	bump_frames(buf, p_ref, player);
	free_prog(DBFETCH(p_ref)->sp.program.code, DBFETCH(p_ref)->sp.program.siz);
	/*  cleanpubs(DBFETCH(p_ref)->sp.program.pubs); */
	/*  DBFETCH(p_ref)->sp.program.pubs = NULL; */
	DBFETCH(p_ref)->sp.program.first = 0;
	DBFETCH(p_ref)->sp.program.curr_line = 0;
	DBFETCH(p_ref)->sp.program.siz = 0;
	DBFETCH(p_ref)->sp.program.code = 0;
	DBFETCH(p_ref)->sp.program.start = 0;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_newprogram(__P_PROTO) {
	char buf[300];
	CHECKOP(1);
	p_oper1 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string)
		abort_interp("NULL argument");
	if (!Wizard(fr->euid) || Typeof(fr->euid) != TYPE_PLAYER)
		abort_interp("Permission denied");

	p_ref = new_object();
	DBSTORE(p_ref, name, dup_string(p_oper1->data.string));
	snprintf(buf, BUFFER_LEN, "A scroll containing a spell called %s", p_oper1->data.string);
	DBSTORE(p_ref, desc, dup_string(buf));
	DBSTORE(p_ref, location, fr->euid);
	DBSTORE(p_ref, link, fr->euid);
	add_backlinks(p_ref);
	DBSTORE(p_ref, owner, fr->euid);
	add_ownerlist(p_ref);
	FLAGS(p_ref) = TYPE_PROGRAM;
	DBSTORE(p_ref, sp.program.first, 0);
	DBSTORE(p_ref, sp.program.curr_line, 0);
	DBSTORE(p_ref, sp.program.siz, 0);
	DBSTORE(p_ref, sp.program.code, 0);
	DBSTORE(p_ref, sp.program.start, 0);
	DBSTORE(p_ref, sp.program.editlocks, NULL);
	/*  DBSTORE(fr->euid, curr_prog, p_ref); */
	PUSH(p_ref, DBFETCH(fr->euid)->contents); DBDIRTY(p_ref); DBDIRTY(fr->euid);
	p_result = (int) p_ref;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_prog_size(__P_PROTO) {
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow!");
	p_result = DBFETCH(p_oper1->data.objref)->sp.program.siz;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_new_macro(__P_PROTO) {
	macrotable *newmacro;
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_STRING || p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string || !p_oper1->data.string)
		abort_interp("NULL argument.");

	newmacro = new_macro(p_oper2->data.string, p_oper1->data.string, fr->euid);
	if (!macrotop)
		macrotop = newmacro;
	else if (!grow_macro_tree(macrotop, newmacro))
		p_result = 0;
	else
		p_result = 1;

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_kill_macro(__P_PROTO) {
	char tmp[BUFFER_LEN];
	CHECKOP(1);
	p_oper1 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string)
		abort_interp("NULL argument.");
	if (!fr->wizard)
		abort_interp("Permission denied");

	strcpy(tmp, p_oper1->data.string);

	if (!string_compare(tmp, macrotop->name)) {
		macrotable *macrotemp = macrotop;
		int whichway = (macrotop->left) ? 1 : 0;
		macrotop = whichway ? macrotop->left : macrotop->right;
		if (macrotop && (whichway ? macrotemp->right : macrotemp->left))
			grow_macro_tree(macrotop, whichway ? macrotemp->right
					: macrotemp->left);
		free((void *) macrotemp);
		p_result = 1;
	} else if (erase_node(macrotop, macrotop, tmp))
		p_result = 1;
	else
		p_result = 0;

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_get_macro(__P_PROTO) {
	char *tmp;
	CHECKOP(1);
	p_oper1 = POP();

	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string)
		abort_interp("NULL argument.");

	if (!(tmp = (char *) macro_expansion(macrotop, p_oper1->data.string)))
		strcpy(p_buf, "");
	else
		strcpy(p_buf, tmp);

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_delete(__P_PROTO) {
	line *curr, *temp;
	int i, one = 0, two = 0, flag = 0;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-interger argument.");
	if (p_oper2->type == PROG_INTEGER) {
		CHECKOP(1);
		p_oper3 = POP();
		if (!valid_object(p_oper3))
			abort_interp("Non-object argument (3)");
		one = p_oper2->data.number;
		if (one <= 0)
			abort_interp("Negative/zero line number.(2)");
		flag++;
	}

	two = p_oper1->data.number;
	if (two <= 0)
		abort_interp("Negative/zero line number.(1)");
	if (flag)
		p_ref = p_oper3->data.objref;
	else
		p_ref = p_oper2->data.objref;

	if (Typeof(p_ref) != TYPE_PROGRAM)
		abort_interp("Invalid PROGRAM(2)");
	if (!controls(player, p_ref))
		abort_interp("Permission denied");
	if (DBFETCH(p_ref)->sp.program.editlocks != NULL)
		abort_interp("That program is being edited.");

	if (!flag)
		one = two;
	if (one > two)
		abort_interp("Nonsensical arguments.");

	DBSTORE(p_ref, sp.program.first, read_program(p_ref));
	DBSTORE(player, curr_prog, p_ref);
	DBSTORE(p_ref, sp.program.editlocks,
			dbreflist_add(DBFETCH(p_ref)->sp.program.editlocks, player)); DBDIRTY(player); DBDIRTY(p_ref);

	i = one - 1;
	for (curr = DBFETCH(p_ref)->sp.program.first; curr && i; i--)
		curr = curr->next;
	if (curr) {
		DBFETCH(p_ref)->sp.program.curr_line = one;
		i = two - one + 1;
		/* delete n lines */
		while (i && curr) {
			temp = curr;
			if (curr->prev)
				curr->prev->next = curr->next;
			else
				DBFETCH(p_ref)->sp.program.first = curr->next;
			if (curr->next)
				curr->next->prev = curr->prev;
			curr = curr->next;
			free_line(temp);
			i--;
		}
		p_result = two - one - i + 1;
	} else
		p_result = 0;

	do_insert_quit(player, p_ref, flag);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_insert(__P_PROTO) {
	dbref edprog;
	int i;
	line *curr;
	line *new_line;

	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();

	if (!valid_object(p_oper3))
		abort_interp("Non-object argument (1)");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Non-integer argument (2)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (3)");

	p_result = p_oper2->data.number;
	if (p_result <= 0)
		abort_interp("Negative/zero line number.(1)");

	p_ref = p_oper3->data.objref;
	if (Typeof(p_ref) != TYPE_PROGRAM)
		abort_interp("Invalid PROGRAM(2)");
	if (!controls(fr->euid, p_ref))
		abort_interp("Permission denied");
	if (DBFETCH(p_ref)->sp.program.editlocks != NULL)
		abort_interp("That program is being edited.");

	if (!p_oper1->data.string)
		abort_interp("NULL argument. (3)");
	strcpy(p_buf, p_oper1->data.string);

	DBFETCH(player)->sp.player.insert_mode++;
	DBSTORE(p_ref, sp.program.first, read_program(p_ref));
	DBSTORE(player, curr_prog, p_ref);
	DBSTORE(p_ref, sp.program.editlocks,
			dbreflist_add(DBFETCH(p_ref)->sp.program.editlocks, player)); DBDIRTY(player); DBDIRTY(p_ref);

	edprog = DBFETCH(player)->curr_prog;
	i = DBFETCH(edprog)->sp.program.curr_line - 1;
	for (curr = DBFETCH(edprog)->sp.program.first; curr && i && i + 1; i--, curr
			= curr->next)
		;
	new_line = get_new_line(); /* initialize line */
	new_line->this_line = dup_string(p_buf);
	if (!DBFETCH(edprog)->sp.program.first)
	/* nothing --- insert in front */
	{
		DBFETCH(edprog)->sp.program.first = new_line;
		DBFETCH(edprog)->sp.program.curr_line = 2; /* insert at the end */
		DBDIRTY(edprog);
		do_insert_quit(player, p_ref, 1);
		return;
	}
	if (!curr) /* insert at the end */
	{
		i = 1;
		for (curr = DBFETCH(edprog)->sp.program.first; curr->next; curr
				= curr->next, i++)
			;
		DBFETCH(edprog)->sp.program.curr_line = i + 2;
		new_line->prev = curr;
		curr->next = new_line;
		DBDIRTY(edprog);
		do_insert_quit(player, p_ref, 1);
		return;
	}
	if (!DBFETCH(edprog)->sp.program.curr_line) /* insert at the beginning */
	{
		DBFETCH(edprog)->sp.program.curr_line = 1; /* insert after this new line */
		new_line->next = DBFETCH(edprog)->sp.program.first;
		DBFETCH(edprog)->sp.program.first = new_line;
		DBDIRTY(edprog);
		do_insert_quit(player, p_ref, 1);
		return;
	}
	/* inserting in the middle */
	DBFETCH(edprog)->sp.program.curr_line++;
	new_line->prev = curr;
	new_line->next = curr->next;
	if (new_line->next)
		new_line->next->prev = new_line;
	curr->next = new_line;
	DBDIRTY(edprog);
	do_insert_quit(player, p_ref, 1);
}

void do_insert_quit(dbref player, dbref p_ref, int flag) {
	write_program(DBFETCH(p_ref)->sp.program.first, p_ref);
	free_prog_text(DBFETCH(p_ref)->sp.program.first);
	DBSTORE(p_ref, sp.program.editlocks,
			dbreflist_remove(DBFETCH(p_ref)->sp.program.editlocks, player));
	DBSTORE(player, curr_prog, NOTHING);
	DBFETCH(player)->sp.player.insert_mode = 0;
	DBDIRTY(player); DBDIRTY(p_ref);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	if (flag)
		CLEAR(p_oper3);
}

void prims_getlines(__P_PROTO) {
	line *curr;
	int i, count, one, two;

	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();

	if (!valid_object(p_oper3))
		abort_interp("Non-object argument (1)");
	if (p_oper2->type != PROG_INTEGER || p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer arguments");

	one = p_oper2->data.number;
	two = p_oper1->data.number;
	p_ref = p_oper3->data.objref;

	if (Typeof(p_ref) != TYPE_PROGRAM)
		abort_interp("Non-program object");
	if (!(controls(player, p_ref) || Linkable(p_ref)))
		abort_interp("Permission denied.");
	if (DBFETCH(p_ref)->sp.program.editlocks != NULL)
		abort_interp("That program is being edited.");

	if ((one > two) && (two != -1))
		abort_interp("Arguments don't make sense!");
	DBSTORE(p_ref, sp.program.first, read_program(p_ref)); DBDIRTY(p_ref);
	i = one - 1;
	for (curr = DBFETCH(p_ref)->sp.program.first; i && curr; i--, curr
			= curr->next)
		;
	if (curr) {
		i = two - one + 1;
		/* display n lines */
		for (count = one; curr && (i || (two == -1)); i--) {
			if (*top >= STACK_SIZE)
				abort_interp("Stack Overflow!");
			snprintf(p_buf, BUFFER_LEN, "%s", DoNull(curr->this_line));
			push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
			count++;
			curr = curr->next;
		}
		if (*top >= STACK_SIZE)
			abort_interp("Stack Overflow!");
		p_result = count - one;
		push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	} else
	abort_interp("Line not available.");

	if (!(FLAGS(p_ref) & INTERNAL))
		free_prog_text(DBFETCH(p_ref)->sp.program.first);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

void prims_prog_lines(__P_PROTO) {
	line *curr;
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Non-object argument (1)");
	p_ref = p_oper1->data.objref;
	if (Typeof(p_ref) != TYPE_PROGRAM)
		abort_interp("Non-program object");

	if (DBFETCH(p_ref)->sp.program.editlocks != NULL)
		abort_interp("That program is being edited.");

	DBSTORE(p_ref, sp.program.first, read_program(p_ref)); DBDIRTY(p_ref);
	curr = DBFETCH(p_ref)->sp.program.first;
	if (curr)
		for (p_result = 0; curr; curr = curr->next)
			p_result++;

	else
		p_result = 0;

	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow!");
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);

	if (!(FLAGS(p_ref) & INTERNAL))
		free_prog_text(DBFETCH(p_ref)->sp.program.first);
	CLEAR(p_oper1);
}
