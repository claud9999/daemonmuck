#include "copyright.h"
#include "config.h"
#include "money.h"

#include "db.h"
#include "interface.h"
#include "externs.h"
#include "params.h"
#include "match.h"
#include <ctype.h>

extern char *lowercase, *uppercase;
#define DOWNCASE(x) (lowercase[x])

static char buf[BUFFER_LEN];

void editor(dbref player, char *command);
void do_insert(dbref player, dbref program, int arg[], int argc);
void do_delete(dbref player, dbref program, int arg[], int argc);
void do_quit(dbref player, dbref program);
void insert(dbref player, char *line1);
line *get_new_line(void);
line *read_program(dbref i);
void do_compile(dbref player, dbref program);
void free_line(line *l);
void free_prog_text(line *l);
void val_and_head(dbref player, int arg[], int argc);
void do_list_header(dbref player, dbref program);
void toggle_numbers(dbref player);

/* Editor routines --- Also contains routines to handle input */

/* This routine determines if a player is editing or running an interactive
 command.  It does it by checking the frame pointer field of the player ---
 if the program counter is NULL, then the player is not running anything
 The reason we don't just check the pointer but check the pc too is because
 I plan to leave the frame always on to save the time required allocating
 space each time a program is run.
 */
void interactive(dbref player, char *command) {
	frame *fr = NULL;

	if ((fr = find_frame(DBFETCH(player)->sp.player.pid)) && (fr->status
			== STATUS_READ) && (fr->player == player)
			&& (DBFETCH(player)->curr_prog == NOTHING)) {
		if (!string_compare(command, BREAK_COMMAND))
			fr->status = STATUS_DEAD;
		else {
			if (fr->argument.top >= STACK_SIZE) {
				notify(player, player, "Program stack overflow.");
				fr->status = STATUS_DEAD;
				return;
			}
			fr->argument.st[fr->argument.top].type = PROG_STRING;
			fr->argument.st[fr->argument.top++].data.string = dup_string(
					command);
			fr->status = STATUS_RUN;
		}
	} else
		editor(player, command);
	DBDIRTY(player);
}

char *macro_expansion(macrotable *node, char *match) {
	int value = 0;

	if (!node)
		return NULL;
	else {
		value = string_compare(match, node->name);
		if (value < 0)
			return macro_expansion(node->left, match);
		else if (value > 0)
			return macro_expansion(node->right, match);
		else
			return dup_string(node->definition);
	}
}
macrotable *new_macro(char *name, char *definition, dbref player) {
	macrotable *newmacro = NULL;
	int i = 0;

	newmacro = (macrotable *) malloc(sizeof(macrotable));
	for (i = 0; name[i]; i++)
		buf[i] = DOWNCASE((int)name[i]);
	buf[i] = '\0';
	newmacro->name = dup_string(buf);
	newmacro->definition = dup_string(definition);
	newmacro->implementor = player;
	newmacro->left = NULL;
	newmacro->right = NULL;
	return (newmacro);
}

int grow_macro_tree(macrotable *node, macrotable *newmacro) {
	int value = 0;

	value = strcmp(newmacro->name, node->name);
	if (!value)
		return 0;
	else if (value < 0) {
		if (node->left)
			return grow_macro_tree(node->left, newmacro);
		else {
			node->left = newmacro;
			return 1;
		}
	} else if (node->right)
		return grow_macro_tree(node->right, newmacro);
	else {
		node->right = newmacro;
		return 1;
	}
}
void insert_macro(char *word[], dbref player) {
	macrotable *newmacro = NULL;

	newmacro = new_macro(word[1], word[2], player);
	if (!macrotop)
		macrotop = newmacro;
	else if (!grow_macro_tree(macrotop, newmacro))
		notify(player, player, "Oopsie! That macro's already been defined.");
	else
		notify(player, player, "Entry created.");
}

void do_list_tree(macrotable *node, char *first, char *last, int length,
		dbref player) {
	static char buff[BUFFER_LEN];

	if (!node)
		return;
	else {
		if (strncmp(node->name, first, strlen(first)) >= 0)
			do_list_tree(node->left, first, last, length, player);
		if ((strncmp(node->name, first, strlen(first)) >= 0) && (strncmp(
				node->name, last, strlen(last)) <= 0)) {
			if (length) {
				snprintf(buff, BUFFER_LEN, "%-16s %-16s  %s", node->name,
						unparse_name(node->implementor), node->definition);
				notify(player, player, buff);
				buff[0] = '\0';
			} else {
				snprintf(buff + strlen(buff), BUFFER_LEN - strlen(buff),
						"%-16s", node->name);
				if (strlen(buff) > 70) {
					notify(player, player, buff);
					buff[0] = '\0';
				}
			}
		}
		if (strncmp(last, node->name, strlen(last)) >= 0)
			do_list_tree(node->right, first, last, length, player);
		if ((node == macrotop) && !length) {
			notify(player, player, buff);
			buff[0] = '\0';
		}
	}
}

void list_macros(char *word[], int k, dbref player, int length) {
	if (!k--)
		do_list_tree(macrotop, "a", "z", length, player);
	else
		do_list_tree(macrotop, word[0], word[k], length, player);
	notify(player, player, "End of list.");
	return;
}

int erase_node(macrotable *oldnode, macrotable *node, char *killname) {
	if (!node)
		return 0;
	else if (strcmp(killname, node->name) < 0)
		return erase_node(node, node->left, killname);
	else if (strcmp(killname, node->name))
		return erase_node(node, node->right, killname);
	else {
		if (node == oldnode->left) {
			oldnode->left = node->left;
			if (node->right)
				grow_macro_tree(macrotop, node->right);
			free((void *) node);
			return 1;
		} else {
			oldnode->right = node->right;
			if (node->left)
				grow_macro_tree(macrotop, node->left);
			free((void *) node);
			return 1;
		}
	}
}

void kill_macro(char *word[], dbref player) {
	if (!Wizard(player)) {
		notify(player, player, "I'm sorry, Dave, I can't let you do that.");
		return;
	} else if (!macrotop) {
		notify(player, player,
				"You've got nothing and you want to kill? Sheesh!");
		return;
	} else if (!string_compare(word[0], macrotop->name)) {
		macrotable *macrotemp = macrotop;
		int whichway = (macrotop->left) ? 1 : 0;
		macrotop = whichway ? macrotop->left : macrotop->right;
		if (macrotop && (whichway ? macrotemp->right : macrotemp->left))
			grow_macro_tree(macrotop, whichway ? macrotemp->right
					: macrotemp->left);
		free((void *) macrotemp);
		notify(player, player, "Entry removed.");
	} else if (erase_node(macrotop, macrotop, word[0]))
		notify(player, player, "Entry removed.");
	else
		notify(player, player, "Entry to remove not found.");
}

/* The editor itself --- this gets called each time every time to
 * parse a command.
 */

void editor(dbref player, char *command) {
	dbref program = 0l;
	int arg[MAX_ARG + 1];
	char buff[BUFFER_LEN];
	char *word[MAX_ARG + 1];
	int i = 0, j = 0; /* loop variables */

	program = DBFETCH(player)->curr_prog;

	/* check to see if we are insert mode */
	if (DBFETCH(player)->sp.player.insert_mode) {
		insert(player, command); /* insert it! */
		return;
	}
	/* parse the commands */
	for (i = 0; i <= MAX_ARG && *command; i++) {
		while (*command && isspace(*command))
			command++;
		j = 0;
		while (*command && !isspace(*command)) {
			buff[j] = *command;
			command++, j++;
		}

		buff[j] = '\0';
		word[i] = dup_string(buff);
		if ((i == 1) && !string_compare(word[0], "def")) {
			while (*command && isspace(*command))
				command++;
			word[2] = dup_string(command);
			if (!word[2])
				notify(player, player, "Invalid definition syntax.");
			else
				insert_macro(word, player);
			for (; i >= 0; i--) {
				if (word[i])
					free((void *) word[i]);
			}
			return;
		}
		arg[i] = atoi(buff);
		if (arg[i] < 0) {
			notify(player, player, "Negative arguments not allowed!");
			for (; i >= 0; i--) {
				if (word[i])
					free((void *) word[i]);
			}
			return;
		}
	}
	i--;
	while ((i >= 0) && !word[i])
		i--;
	if (i < 0)
		return;
	else {
		switch (word[i][0]) {
		case KILL_COMMAND:
			kill_macro(word, player);
			break;
		case SHOW_COMMAND:
			list_macros(word, i, player, 1);
			break;
		case SHORTSHOW_COMMAND:
			list_macros(word, i, player, 0);
			break;
		case INSERT_COMMAND:
			notify(player, player, "Entering insert mode.");
			do_insert(player, program, arg, i);
			break;
		case DELETE_COMMAND:
			do_delete(player, program, arg, i);
			break;
		case QUIT_EDIT_COMMAND:
			do_quit(player, program);
			notify(player, player, "Editor exited.");
			break;
		case COMPILE_COMMAND:
			/* compile code belongs in compile.c, not in the editor */
			do_compile(player, program);
			notify(player, player, "Compiler done.");
			break;
		case LIST_COMMAND:
			do_list(player, program, arg, i);
			break;
		case EDITOR_HELP_COMMAND:
			spit_file(player, EDITOR_HELP_FILE);
			break;
		case VIEW_COMMAND:
			val_and_head(player, arg, i);
			break;
		case UNASSEMBLE_COMMAND:
			disassemble(player, program);
			break;
		case NUMBER_COMMAND:
			toggle_numbers(player);
			break;
		default:
			notify(player, player, "Illegal editor command.");
			break;
		}
	}
	for (; i >= 0; i--) {
		if (word[i])
			free((void *) word[i]);
	}
}

/* puts program into insert mode */
void do_insert(dbref player, dbref program, int arg[], int argc) {
	DBFETCH(player)->sp.player.insert_mode++;
	DBDIRTY(player);
	if (argc)
		DBSTORE(program, sp.program.curr_line, arg[0] - 1);
	/* set current line to something else */
	if (FLAGS(player) & INTERNAL) {
		notify(player, player, "%d>", DBFETCH(program)->sp.program.curr_line
				+ 1);
	}
}

/* deletes line n if one argument,
 lines arg1 -- arg2 if two arguments
 current line if no argument */
void do_delete(dbref player, dbref program, int arg[], int argc) {
	line *curr = NULL, *temp = NULL;
	int i = 0;

	switch (argc) {
	case 0:
		arg[0] = DBFETCH(program)->sp.program.curr_line;
	case 1:
		arg[1] = arg[0];
	case 2:
		/* delete from line 1 to line 2 */
		/* first, check for conflict */
		if (arg[0] > arg[1]) {
			notify(player, player, "Nonsensical arguments.");
			return;
		}
		i = arg[0] - 1;
		for (curr = DBFETCH(program)->sp.program.first; curr && i; i--)
			curr = curr->next;
		if (curr) {
			DBFETCH(program)->sp.program.curr_line = arg[0];
			i = arg[1] - arg[0] + 1;
			/* delete n lines */
			while (i && curr) {
				temp = curr;
				if (curr->prev)
					curr->prev->next = curr->next;
				else
					DBFETCH(program)->sp.program.first = curr->next;
				if (curr->next)
					curr->next->prev = curr->prev;
				curr = curr->next;
				free_line(temp);
				i--;
			}
			notify(player, player, "%d lines deleted", arg[1] - arg[0] - i + 1);
		} else
			notify(player, player, "No line to delete!");
		break;
	default:
		notify(player, player, "Too many arguments!");
		break;
	}
}

/* quit from edit mode.  Put player back into the regular game mode */
void do_quit(dbref player, dbref program) {
	write_program(DBFETCH(program)->sp.program.first, program);
	free_prog_text(DBFETCH(program)->sp.program.first);
	DBSTORE(program, sp.program.editlocks,
			dbreflist_remove(DBFETCH(program)->sp.program.editlocks, player));
	FLAGS(player) &= ~INTERACTIVE;
	DBSTORE(player, curr_prog, NOTHING);DBDIRTY(player);DBDIRTY(program);
}

void match_and_list(dbref player, char *name, char *linespec) {
	dbref thing = 0l;
	char *p = NULL;
	char *q = NULL;
	int range[2];
	int argc = 0;
	match_data md;

	if (Typeof(player) != TYPE_PLAYER) {
		notify(OWNER(player), OWNER(player), "Only Players can list programs.");
		return;
	}

	init_match(player, name, TYPE_PROGRAM, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_absolute(&md);
	if ((thing = noisy_match_result(&md)) == NOTHING)
		return;
	if (Typeof(thing) != TYPE_PROGRAM) {
		notify(player, player, "You can't list anything but a program.");
		return;
	}
	if (!(controls(player, thing) || Linkable(thing))) {
		notify(player, player, "Permission denied.");
		return;
	}

	if (DBFETCH(thing)->sp.program.editlocks != NULL) {
		notify(player, player, "Sorry, that program is being edited.");
		return;
	}

	if (!*linespec) {
		range[0] = 1;
		range[1] = -1;
		argc = 2;
	} else {
		q = p = linespec;
		while (*p) {
			while (*p && !isspace(*p))
				*q++ = *p++;
			while (*p && isspace(*++p))
				;
		}
		*q = '\0';

		argc = 1;
		if (isdigit(*linespec)) {
			range[0] = atoi(linespec);
			while (*linespec && isdigit(*linespec))
				linespec++;
		} else
			range[0] = 1;
		if (*linespec) {
			argc = 2;
			while (*linespec && !isdigit(*linespec))
				linespec++;
			if (*linespec)
				range[1] = atoi(linespec);
			else
				range[1] = -1;
		}
	}
	DBSTORE(thing, sp.program.first, read_program(thing));
	do_list(player, thing, range, argc);
	if (!(FLAGS(thing) & INTERNAL))
		free_prog_text(DBFETCH(thing)->sp.program.first);
	return;
}

/* list --- if no argument, redisplay the current line
 if 1 argument, display that line
 if 2 arguments, display all in between   */
void do_list(dbref player, dbref program, int oarg[], int argc) {
	line *curr = NULL;
	int i = 0, count = 0;
	int arg[2];

	if (oarg) {
		arg[0] = oarg[0];
		arg[1] = oarg[1];
	} else
		arg[0] = arg[1] = 0;
	switch (argc) {
	case 0:
		arg[0] = DBFETCH(program)->sp.program.curr_line;
	case 1:
		arg[1] = arg[0];
	case 2:
		if ((arg[0] > arg[1]) && (arg[1] != -1)) {
			notify(player, player, "Arguments don't make sense!");
			return;
		}
		i = arg[0] - 1;
		for (curr = DBFETCH(program)->sp.program.first; i && curr; i--, curr
				= curr->next)
			;
		if (curr) {
			i = arg[1] - arg[0] + 1;
			/* display n lines */
			for (count = arg[0]; curr && (i || (arg[1] == -1)); i--) {
				if (FLAGS(player) & INTERNAL)
					notify(player, player, "%3d: %s", count,
							DoNull(curr->this_line));
				else
					notify(player, player, "%s", DoNull(curr->this_line));
				count++;
				curr = curr->next;
			}
			if (count - arg[0] > 1)
				notify(player, player, "%d lines displayed.", count - arg[0]);
		} else
			notify(player, player, "Line not available for display.");
		break;
	default:
		notify(player, player, "Too many arguments!");
		break;
	}
}

void val_and_head(dbref player, int arg[], int argc) {
	dbref program = 0l;

	if (argc != 1) {
		notify(player, player,
				"I don't understand which header you're trying to look at.");
		return;
	}
	program = arg[0];
	if (program < 0 || program >= db_top || Typeof(program) != TYPE_PROGRAM) {
		notify(player, player, "That isn't a program.");
		return;
	}
	if (!(controls(player, program) || Linkable(program))) {
		notify(player, player, "That's not a public program.");
		return;
	}
	do_list_header(player, program);
}

void do_list_header(dbref player, dbref program) {
	line *curr = read_program(program);

	while (curr && (curr->this_line)[0] == '(') {
		notify(player, player, curr->this_line);
		curr = curr->next;
	}
	if (!(FLAGS(program) & INTERNAL))
		free_prog_text(curr);
	notify(player, player, "Done.");
}

void toggle_numbers(dbref player) {
	if (FLAGS(player) & INTERNAL) {
		FLAGS(player) &= ~INTERNAL;
		notify(player, player, "Line numbers off.");
	} else {
		FLAGS(player) |= INTERNAL;
		notify(player, player, "Line numbers on.");
	}
}

/* insert this line into program */
void insert(dbref player, char *line_text) {
	dbref program = 0l;
	int i = 0;
	line *curr = NULL;
	line *new_line = NULL;

	program = DBFETCH(player)->curr_prog;
	if (!string_compare(line_text, EXIT_INSERT)) {
		DBSTORE(player, sp.player.insert_mode, 0); /* turn off insert mode */
		return;
	}
	i = DBFETCH(program)->sp.program.curr_line - 1;
	for (curr = DBFETCH(program)->sp.program.first; curr && i && i + 1; i--, curr
			= curr->next)
		;
	new_line = get_new_line(); /* initialize line */
	new_line->this_line = dup_string(line_text);
	if (!DBFETCH(program)->sp.program.first)
	/* nothing --- insert in front */
	{
		DBFETCH(program)->sp.program.first = new_line;
		DBFETCH(program)->sp.program.curr_line = 2; /* insert at the end */
		DBDIRTY(program);
		if (FLAGS(player) & INTERNAL)
			notify(player, player, "%d>",
					DBFETCH(program)->sp.program.curr_line);
		return;
	}
	if (!curr) /* insert at the end */
	{
		i = 1;
		for (curr = DBFETCH(program)->sp.program.first; curr->next; curr
				= curr->next, i++)
			;
		DBFETCH(program)->sp.program.curr_line = i + 2;
		new_line->prev = curr;
		curr->next = new_line;
		DBDIRTY(program);
		if (FLAGS(player) & INTERNAL)
			notify(player, player, "%d>",
					DBFETCH(program)->sp.program.curr_line);
		return;
	}
	if (!DBFETCH(program)->sp.program.curr_line) /* insert at the beginning */
	{
		DBFETCH(program)->sp.program.curr_line = 1; /* insert after this new line */
		new_line->next = DBFETCH(program)->sp.program.first;
		DBFETCH(program)->sp.program.first = new_line;
		DBDIRTY(program);
		if (FLAGS(player) & INTERNAL)
			notify(player, player, "%d>",
					DBFETCH(program)->sp.program.curr_line + 1);
		return;
	}
	/* inserting in the middle */
	DBFETCH(program)->sp.program.curr_line++;
	if (FLAGS(player) & INTERNAL)
		notify(player, player, "%d>", DBFETCH(program)->sp.program.curr_line
				+ 1);
	new_line->prev = curr;
	new_line->next = curr->next;
	if (new_line->next)
		new_line->next->prev = new_line;
	curr->next = new_line;
	DBDIRTY(program);
}
