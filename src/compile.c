#include <ctype.h>
#include <stdio.h>
#include "copyright.h"
#include "config.h"
#include "version.h"

#include "db.h"
#include "interface.h"
#include "inst.h"
#include "externs.h"
#include "match.h"

/* This file contains code for doing "byte-compilation" of
 mud-forth programs.  As such, it contains many internal
 data structures and other such which are not found in other
 parts of TinyMUCK.                                       */

/* The IF_STACK is a stack for holding previous IF statements.
 Everytime a THEN is encountered, the next address is inserted
 into the code before the most recent IF.  */

/* Of course I modified the crap out of it to handle for, do, and while
 loops.  now the type field holds a SORCE_whatever to tell where the
 loop came from, and LOOP parses accordingly to tidy up --Doran*/

/* Also, if you change any primitive names, you need to look through
 the 'name dependency' and change those accordingly. */

/* name dependancy *//* not all are used */
#define IF_NAME "IF"
#define CALL_NAME "CALL"
#define READ_NAME "READ"
#define EXIT_NAME "EXIT"
#define JMP_NAME "JMP"
#define PROGRAM_NAME "PROGRAM "
#define EXECUTE_NAME "EXECUTE"
#define SLEEP_NAME "SLEEP"
#define VAR_NAME "VAR"
#define LOOP_NAME "LOOP"
#define NOP_NAME "NOP"
#define FOR_CHECK_NAME "FOR_CHECK "
#define FOR_ADD_NAME "FOR_ADD "
#define FOR_POP_NAME "FOR_POP "

static hash_tab primitive_list[COMP_HASH_SIZE];

/* #defines for types of addresses shoved on the if stack */
#define SOURCE_ERROR -1
#define SOURCE_IF 0
#define SOURCE_FOR 1
#define SOURCE_DO 2
#define SOURCE_WHILE_ALPHA 3
#define SOURCE_WHILE_BETA 4

/* The intermediate code is code generated as a linked list
 when there is no hint or notion of how much code there
 will be, and to help resolve all references.
 There is always a pointer to the current word that is
 being compiled kept.                                   */

typedef struct intermediate {
	int no; /* which number instruction this is */
	inst in; /* instruction itself */
	struct intermediate *next; /* next instruction */
} intermediate;

typedef struct if_stack {
	int source;
	intermediate *place;
	struct if_stack *next;
} if_stack;

if_stack *ifs;

/* This structure is an association list that contains both a procedure
 name and the place in the code that it belongs.  A lookup to the procedure
 will see both it's name and it's number and so we can generate a
 reference to it.  Since I want to disallow co-recursion,  I will not allow
 forward referencing.
 */

typedef struct proc_list {
	char *name;
	intermediate *code;
	struct proc_list *next;
} proc_list;

proc_list *procs;

static int nowords; /* number of words compiled */
static intermediate *curr_word; /* word currently being compiled */
static intermediate *first_word; /* first word of the list */
static intermediate *curr_proc; /* first word of current procedure */
/* variable names.  The index into variables give you what position
 * the variable holds.
 */
static char *variables[MAX_VAR] = { "ME", "LOC", "TRIGGER" };

static line *curr_line; /* current line */
static int lineno; /* current line number */
static char *next_char; /* next char * */
static dbref player, program; /* globalized player and program */

/* 1 if error occured */
static int compile_err;

int primitive(char *s); /* returns primitive_number if primitive */
void advance_line();
void free_prog(inst *, int);
char *next_token();
char *next_token_raw();
intermediate *next_word(char *);
intermediate *process_special(char *);
intermediate *primitive_word(char *);
intermediate *string_word(char *);
intermediate *number_word(char *);
intermediate *floating_word(char *);
intermediate *object_word(char *);
intermediate *quoted_word(char *);
intermediate *call_word(char *);
intermediate *var_word(char *);
char *do_string();
void do_comment();
intermediate *new_inst();
intermediate *find_if();
void cleanup();
void add_proc(char *, intermediate *);
void addif(intermediate *, int from);
int query_if();
int for_nest();
int add_variable(char *);
int special(char *);
int call(char *);
int quoted(char *);
int object_check(char *);
int string(char *);
int variable(char *);
int get_primitive(char *);
void copy_program();
void set_start();
char *line_copy = NULL;
int macrosubs; /* Safeguard for macro-subst. infinite loops */

/* Character defines */
#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'
#define BEGINDIRECTIVE '$'
#define BEGINESCAPE '\\'

#define SUBSTITUTIONS 20  /* How many nested macros will we allow? */

/* abort compile macro */
#define abort_compile(C) \
{ \
  if (line_copy) \
  { \
    free (line_copy); \
    line_copy = NULL; \
  } \
  if (player != NOTHING) notify(player, player, "Error in line %d: %s", lineno, C); \
  else \
    log_muf("MUF compiler warning in program %ld:\nError in line %d: %s\n", program, lineno, C); \
  cleanup(); \
  compile_err++; \
  free_prog(DBFETCH(program)->sp.program.code, \
    DBFETCH(program)->sp.program.siz); \
  return 0; \
}

/* for void functions */
#define v_abort_compile(C) \
{ \
  if (line_copy) \
  { \
    free (line_copy); \
    line_copy = NULL; \
  } \
  if (player != NOTHING) notify(player, player, "Error in line %d: %s", lineno, C); \
  else\
   log_muf("MUF compiler warning in program %ld:\nError in line %d: %s\n", program, lineno, C); \
  cleanup(); \
  compile_err++; \
  free_prog(DBFETCH(program)->sp.program.code, \
    DBFETCH(program)->sp.program.siz); \
  return; \
}

extern frame *frame_list;

/* returns true for numbers of form [x.x] <series of digits> */
int floating(char *s) {
	if (!s)
		return 0;
	while (isspace(*s))
		s++;
	if (*s == '+' || *s == '-')
		s++;
	if ((*s < '0' || *s > '9') || !index(s, '.'))
		return 0;
	return 1;
}

/* overall control code.  Does piece-meal tokenization parsing and
 backward checking.                                            */
void do_compile(dbref player_in, dbref program_in) {
	char *token = NULL;
	char buf[BUFFER_LEN];
	intermediate *new_word = NULL;

	snprintf(buf, BUFFER_LEN, "Program %s has been recompiled by %s.", unparse_name(
			program_in), unparse_name(player_in));
	bump_frames(buf, program_in, player_in);

	/* set all global variables */
	nowords = 0;
	curr_word = first_word = curr_proc = 0;
	player = player_in;
	program = program_in;
	lineno = 1;
	curr_line = DBFETCH(program)->sp.program.first;
	if (curr_line)
		next_char = curr_line -> this_line;
	first_word = curr_word = NULL;
	procs = 0;
	compile_err = 0;
	ifs = 0;
	/* free old stuff */
	free_prog(DBFETCH(program)->sp.program.code,
			DBFETCH(program)->sp.program.siz);

	if (!curr_line)
		v_abort_compile("Missing program text.");

	/* do compilation */
	while ((token = next_token())) {
		new_word = next_word(token);

		/* test for errors */
		if (compile_err)
			return;

		if (new_word) {
			if (!first_word)
				first_word = curr_word = new_word;
			else {
				curr_word -> next = new_word;
				curr_word = curr_word -> next;
			}
		}

		while (curr_word && curr_word -> next)
			curr_word = curr_word -> next;

		free(token);
	}

	if (curr_proc)
		v_abort_compile("Unexpected end of file.");

	if (!procs)
		v_abort_compile("Missing procedure definition.");

	/* do copying over */
	copy_program();

	if (compile_err)
		return;

	set_start();
	cleanup();
}

intermediate *next_word(char *token) {
	intermediate *new_word = NULL;
	char buf[BUFFER_LEN];

	if (!token)
		return 0;

	if (special(token))
		new_word = process_special(token);
	else if (variable(token))
		new_word = var_word(token);
	else if (primitive(token))
		new_word = primitive_word(token);
	else if (string(token))
		new_word = string_word(token + 1);
	else if (floating(token))
		new_word = floating_word(token);
	else if (number(token))
		new_word = number_word(token);
	else if (object_check(token))
		new_word = object_word(token);
	else if (quoted(token))
		new_word = quoted_word(token + 1);
	else if (call(token))
		new_word = call_word(token);
	else {
		snprintf(buf, BUFFER_LEN, "Unrecognized word %s.", token);
		abort_compile(buf);
	}

	if (new_word)
		new_word -> in.linenum = lineno;
	return new_word;
}

/* Little routine to do the line_copy handling right */
void advance_line() {
	curr_line = curr_line -> next;
	lineno++;
	macrosubs = 0;
	if (line_copy) {
		free(line_copy);
		line_copy = NULL;
	}
	if (curr_line)
		next_char = (line_copy = dup_string(curr_line -> this_line));
	else
		next_char = (line_copy = NULL);
}

/* Skips comments, grabs strings, returns NULL when no more tokens to grab. */
char * next_token() {
	char buf[BUFFER_LEN];
	char *expansion = NULL, *temp = NULL;
	int i = 0;

	if (!curr_line)
		return (char *) 0;

	if (!next_char)
		return (char *) 0;

	/* skip white space */
	while (*next_char && isspace(*next_char))
		next_char++;

	if (!(*next_char)) {
		advance_line();
		if (!curr_line)
			return (char *) 0;
		else
			return next_token();
	}

	/* take care of comments */
	if (*next_char == BEGINCOMMENT) {
		do_comment();
		return next_token();
	}

	if (*next_char == BEGINSTRING)
		return do_string();

	/* macro */
	if (*next_char == BEGINMACRO) {
		next_char++;
		for (i = 0; *next_char && !isspace(*next_char); i++) {
			buf[i] = *next_char;
			next_char++;
		}
		buf[i] = '\0';
		if (!(expansion = (char *) macro_expansion(macrotop, buf))) {
			abort_compile ("Macro not defined.");
		} else {
			if (++macrosubs > SUBSTITUTIONS) {
				abort_compile ("Too many macro substitutions.");
			} else {
				temp = (char *) malloc(strlen(next_char) + strlen(expansion)
						+ 21);
				strcpy(temp, expansion);
				strcat(temp, next_char);
				free(expansion);
				free(line_copy);
				next_char = line_copy = temp;
				return next_token();
			}
		}
	}
	/* ordinary token */
	for (i = 0; *next_char && !isspace(*next_char); i++) {
		buf[i] = *next_char;
		next_char++;
	}
	buf[i] = '\0';
	return dup_string(buf);
}

/* skip comments */
void do_comment() {
	while (*next_char && *next_char != ENDCOMMENT)
		next_char++;
	if (!(*next_char)) {
		advance_line();
		if (!curr_line) {
			v_abort_compile("Unterminated comment.");
		}
		do_comment();
	} else {
		next_char++;
		if (!(*next_char))
			advance_line();
	}
}

/* return string */
char * do_string() {
	char buf[BUFFER_LEN];
	int i = 0, quoted1 = 0;

	buf[i] = *next_char;
	next_char++;
	i++;
	while ((quoted1 || *next_char != ENDSTRING) && *next_char)
		if (*next_char == '\\' && !quoted1) {
			quoted1++;
			next_char++;
		} else {
			buf[i] = *next_char;
			i++;
			next_char++;
			quoted1 = 0;
		}
	if (!*next_char) {
		abort_compile("Unterminated string found at end of line.");
	}
	next_char++;
	buf[i] = '\0';
	return dup_string(buf);
}

/* process special.  Performs special processing.
 It sets up FOR and IF structures.  Remember --- for those,
 we've got to set aside an extra argument space.         */
intermediate * process_special(char *token) {
	char buf[BUFFER_LEN];
	char *tok = NULL;
	intermediate *new = NULL;

	if (!string_compare(token, ":")) {
		char *proc_name = NULL;

		if (curr_proc)
			abort_compile("Definition within definition.");
		proc_name = next_token();
		if (!proc_name)
			abort_compile("Unexpected end of file within procedure.");
		tok = next_token();
		new = next_word(tok);
		if (tok)
			free(tok);
		if (!new) {
			snprintf(buf, BUFFER_LEN, "Error in definition of %s.", proc_name);
			free(proc_name);
			abort_compile(buf);
		}
		curr_proc = new;
		add_proc(proc_name, new);
		free(proc_name);
		return new;
	} else if (!string_compare(token, ";")) {
		if (ifs)
			abort_compile("Unexpected end of procedure definition.");
		if (!curr_proc)
			abort_compile("Procedure end without body.");
		curr_proc = 0;
		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_PRIMITIVE;
		new -> in.data.number = get_primitive(EXIT_NAME); /* name dependency */
		return new;
	} else if (!string_compare(token, "IF")) {
		intermediate *curr = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_ADD;
		new -> in.data.call = 0;
		new -> next = new_inst();
		curr = new -> next;
		curr -> no = nowords++;
		curr -> in.type = PROG_PRIMITIVE;
		curr -> in.data.number = get_primitive(IF_NAME); /* name dependency */
		addif(new, SOURCE_IF);
		return new;
	} else if (!string_compare(token, "ELSE")) {
		intermediate *eef = NULL;
		intermediate *curr = NULL;
		intermediate *after = NULL;

		eef = find_if();
		if (!eef)
			abort_compile("ELSE without IF.");
		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_ADD;
		new -> in.data.call = 0;
		new -> next = new_inst();
		curr = new -> next;
		curr -> no = nowords++;
		curr -> in.type = PROG_PRIMITIVE;
		curr -> in.data.number = get_primitive(JMP_NAME); /* name dependency */
		addif(new, SOURCE_IF); /* treated as if when next then comes up */
		tok = next_token();
		curr -> next = after = next_word(tok);
		if (tok)
			free(tok);
		if (!after)
			abort_compile("Unexpected end of program.");
		eef -> in.data.number = after -> no;
		return new;
	} else if (!string_compare(token, "THEN")) {
		/* can't use 'if' because it's a reserved word */
		intermediate *eef = NULL;

		if (query_if() != SOURCE_IF)
			abort_compile("THEN improperly nested.");
		eef = find_if();
		if (!eef)
			abort_compile("THEN without IF.");
		tok = next_token();
		new = next_word(tok);
		if (tok)
			free(tok);
		if (!new)
			abort_compile("Unexpected end of program.");
		eef -> in.data.number = new -> no;
		return new;
	} else if (!string_compare(token, "DO")) {
		intermediate *after = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_PRIMITIVE;
		new -> in.data.number = get_primitive(NOP_NAME); /* name dependency */
		addif(new, SOURCE_DO);
		tok = next_token();
		new -> next = after = next_word(tok);
		if (tok)
			free(tok);
		if (!after)
			abort_compile("Unexpected end of program.");
		return new;
	} else if (!string_compare(token, "BEGIN")) {
		intermediate *after = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_PRIMITIVE;
		new -> in.data.number = get_primitive(NOP_NAME); /* name dependency */
		addif(new, SOURCE_WHILE_ALPHA);
		tok = next_token();
		new -> next = after = next_word(tok);
		if (tok)
			free(tok);
		if (!after)
			abort_compile("Unexpected end of program.");
		return new;
	} else if (!string_compare(token, "WHILE")) {
		intermediate *curr = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_ADD;
		new -> in.data.call = 0;
		new -> next = new_inst();
		curr = new -> next;
		curr -> no = nowords++;
		curr -> in.type = PROG_PRIMITIVE;
		curr -> in.data.number = get_primitive(IF_NAME); /* name dependency */
		addif(new, SOURCE_WHILE_BETA);
		return new;
	} else if (!string_compare(token, "FOR")) {
		intermediate *after = NULL;
		intermediate *curr = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_PRIMITIVE;
		new -> in.data.number = get_primitive(FOR_ADD_NAME); /* name dependency */
		new -> next = new_inst();
		curr = new -> next;
		curr -> no = nowords++;
		curr -> in.type = PROG_PRIMITIVE;
		curr -> in.data.number = get_primitive(NOP_NAME); /* name dependency */
		addif(curr, SOURCE_FOR);
		tok = next_token();
		curr -> next = after = next_word(tok);
		if (tok)
			free(tok);
		if (!after)
			abort_compile("Unexpected end of program.");
		return new;
	} else if (!string_compare(token, "LOOP")) {
		/* can't use 'if' because it's a reserved word */
		intermediate *eef = NULL;
		intermediate *wheel = NULL;
		intermediate *curr = NULL;
		intermediate *after = NULL;

		if (query_if() == SOURCE_IF || query_if() == SOURCE_ERROR)
			abort_compile("LOOP improperly nested.");
		switch (query_if()) {
		case SOURCE_WHILE_BETA:
			eef = find_if();
			if (query_if() != SOURCE_WHILE_ALPHA)
				abort_compile("Improperly nested loop in conditional of WHILE.")
			;
			wheel = find_if();
			if (!wheel)
				abort_compile("WHILE ... LOOP without BEGIN.")
			;
			new = new_inst();
			new -> no = nowords++;
			new -> in.type = PROG_ADD;
			new -> in.data.number = wheel -> no;
			new -> next = new_inst();
			curr = new -> next;
			curr -> no = nowords++;
			curr -> in.type = PROG_PRIMITIVE;
			curr -> in.data.number = get_primitive(JMP_NAME); /* name dependency */
			tok = next_token();
			curr -> next = after = next_word(tok);
			if (tok)
				free(tok);
			if (!after)
				abort_compile("Unexpected end of program.")
			;
			eef -> in.data.number = new -> no + 2;
			break;
		case SOURCE_DO:
			eef = find_if();
			new = new_inst();
			new -> no = nowords++;
			new -> in.type = PROG_ADD;
			new -> in.data.number = eef -> no;
			new -> next = new_inst();
			curr = new -> next;
			curr -> no = nowords++;
			curr -> in.type = PROG_PRIMITIVE;
			curr -> in.data.number = get_primitive(LOOP_NAME); /* name dependency */
			tok = next_token();
			curr -> next = after = next_word(tok);
			if (tok)
				free(tok);
			if (!after)
				abort_compile("Unexpected end of program.")
			;
			break;
		case SOURCE_FOR:
			eef = find_if();
			new = new_inst();
			new -> no = nowords++;
			new -> in.type = PROG_PRIMITIVE;
			new -> in.data.number = get_primitive(FOR_CHECK_NAME);
			new -> next = new_inst();
			curr = new -> next;
			curr -> no = nowords++;
			curr -> in.type = PROG_ADD;
			curr -> in.data.number = eef -> no;
			curr -> next = new_inst();
			curr = curr -> next;
			curr -> no = nowords++;
			curr -> in.type = PROG_PRIMITIVE;
			curr -> in.data.number = get_primitive(LOOP_NAME); /* name dependency */
			tok = next_token();
			curr -> next = after = next_word(tok);
			if (tok)
				free(tok);
			if (!after)
				abort_compile("Unexpected end of program.")
			;
			break;
		case SOURCE_WHILE_ALPHA:
			abort_compile("WHILE statement missing.")
			;
			break;
		default: {
			char BUF[80];
			snprintf(BUF, BUFFER_LEN, "Unexpected IF_STACK type: %d.", query_if());
			abort_compile(BUF);
		}
			break;
		}
		return new;

	}
#ifdef BREAK_CONTINUE
	else if (!string_compare(token, "BREAK")) {
		/* can't use 'if' because it's a reserved word */
		intermediate *eef = NULL;
		intermediate *curr = NULL;

		eef = find_if();
		if (!eef)
		abort_compile("Can't have a BREAK outside of a loop.");
		new = new_inst();
		new->no = nowords++;
		new->in.type = PROG_ADD;
		/*	new->in.line = lineno; */
		new->in.data.number = 0;
		new->next = new_inst();
		curr = new->next;
		curr->no = nowords++;
		curr->in.type = PROG_PRIMITIVE;
		/*	curr->in.line = lineno; */
		curr->in.data.number = IN_JMP;

		/*	addwhile(new); */
		addif(new, SOURCE_WHILE_BETA);
		return new;
	} else if (!string_compare(token, "CONTINUE")) {
		/* can't use 'if' because it's a reserved word */
		intermediate *beef;
		intermediate *curr;

		beef = find_if();
		if (!beef)
		abort_compile("Can't CONTINUE outside of a loop.");
		new = new_inst();
		new->no = nowords++;
		new->in.type = PROG_ADD;
		/*	new->in.line = lineno; */
		new->in.data.number = beef->no;
		new->next = new_inst();
		curr = new->next;
		curr->no = nowords++;
		curr->in.type = PROG_PRIMITIVE;
		/*	curr->in.line = lineno; */
		curr->in.data.number = IN_JMP;

		return new;
	}
#endif /* BREAK_CONTINUE */
	else if (!string_compare(token, "CALL")) {
		intermediate *curr = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_PRIMITIVE;
		new -> in.data.number = get_primitive(CALL_NAME); /* name dependency */
		new -> next = new_inst();
		curr = new -> next;
		curr -> no = nowords++;
		curr -> in.type = PROG_OBJECT;
		curr -> in.data.objref = program;
		curr -> next = new_inst();
		curr = curr -> next;
		curr -> no = nowords++;
		curr -> in.type = PROG_PRIMITIVE;
		curr -> in.data.number = get_primitive(PROGRAM_NAME); /* name dependency */
		return new;
	} else if (!string_compare(token, "EXIT")) {
		intermediate *curr = NULL;

		new = new_inst();
		new -> no = nowords++;
		new -> in.type = PROG_PRIMITIVE;
		new -> in.data.number = get_primitive(EXIT_NAME); /* name dependency */
		if (for_nest() != 0) {
			new -> in.type = PROG_INTEGER;
			new -> in.data.number = for_nest();
			new -> next = new_inst();
			curr = new -> next;
			curr -> no = nowords++;
			curr -> in.type = PROG_PRIMITIVE;
			curr -> in.data.number = get_primitive(FOR_POP_NAME); /* name dependency */
			curr -> next = new_inst();
			curr = curr -> next;
			curr -> no = nowords++;
			curr -> in.type = PROG_PRIMITIVE;
			curr -> in.data.number = get_primitive(EXIT_NAME); /* name dependency */
		}
		return new;
	}
#ifdef PUBLIC
	else if (!string_compare(token, "PUBLIC")) {
		struct PROC_LIST *p = NULL;
		struct publics *pub = NULL;

		if (curr_proc)
		abort_compile("Public declaration within procedure.");
		tok = next_token();
		if ((!tok) || !call(tok))
		abort_compile("Subroutine unknown in PUBLIC declaration.");
		for (p = procs; p; p = p->next)
		if (!string_compare(p->name, tok))
		break;
		if (!p)
		abort_compile("Subroutine unknown in PUBLIC declaration.");
		if (!currpubs) {
			currpubs = (struct publics *) malloc(sizeof(struct publics));
			currpubs->next = NULL;
			currpubs->subname = (char *) strdup(tok);
			if (tok)
			free((void *) tok);
			currpubs->addr.no = p->code->no;
			return 0;
		}
		for (pub = currpubs; pub;) {
			if (!string_compare(tok, pub->subname)) {
				abort_compile("Function already declared public.");
			} else {
				if (pub->next) {
					pub = pub->next;
				} else {
					pub->next = (struct publics *) malloc(sizeof(struct publics));
					pub = pub->next;
					pub->next = NULL;
					pub->subname = (char *) strdup(tok);
					if (tok)
					free((void *) tok);
					pub->addr.no = p->code->no;
					pub = NULL;
				}
			}
		}
		return 0;
	}
#endif /* PUBLIC */
	else if (!string_compare(token, "VAR")) {
		if (curr_proc)
			abort_compile("Variable declared within procedure.");
		tok = next_token();
		if (!tok || !add_variable(tok))
			abort_compile("Variable limit exceeded.");
		if (tok)
			free(tok);
		return 0;
	} else {
		snprintf(buf, BUFFER_LEN, "Unrecognized special form %s found on line %d.", token,
				lineno);
		abort_compile(buf);
	}
}

/* return primitive word. */
intermediate * primitive_word(char *token) {
	intermediate *new = NULL;

	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_PRIMITIVE;
	new -> in.data.number = get_primitive(token);
	return new;
}

/* return self pushing word (string) */
intermediate *string_word(char *token) {
	intermediate *new = NULL;

	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_STRING;
	new->in.data.string = dup_string(token);
	return new;
}

/* return self pushing word (number) */
intermediate *number_word(char *token) {
	intermediate *new = NULL;

	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_INTEGER;
	new -> in.data.number = strtol(token, NULL, 0);
	return new;
}

/* return self pushing word (floating) */
intermediate *floating_word(char *token) {
	intermediate *new = NULL;

	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_FLOAT;
	new -> in.data.fnum = atof(token);
	return new;
}

/* do a subroutine call --- push address onto stack, then make a primitive
 CALL.
 */
intermediate *call_word(char *token) {
	intermediate *new = NULL;
	proc_list *p = NULL;

	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_ADD;
	for (p = procs; p; p = p -> next)
		if (!string_compare(p -> name, token))
			break;

	new -> in.data.number = p -> code -> no;
	new -> next = new_inst();
	new -> next -> no = nowords++;
	new -> next -> in.type = PROG_PRIMITIVE;
	new -> next -> in.data.number = get_primitive(EXECUTE_NAME); /* name dependency */
	return new;
}

intermediate *quoted_word(char *token) {
	intermediate *new = NULL;
	proc_list *p = NULL;

	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_ADD;
	for (p = procs; p; p = p -> next)
		if (!string_compare(p -> name, token))
			break;

	new -> in.data.number = p -> code -> no;
	return new;
}

/* returns number corresponding to variable number.
 We assume that it DOES exist */
intermediate *var_word(char *token) {
	intermediate *new = NULL;
	int var_no = 0;

	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_VAR;
	for (var_no = 0; var_no < MAX_VAR; var_no++)
		if (!string_compare(token, variables[var_no]))
			break;
	new -> in.data.number = var_no;

	return new;
}

/* check if object is in database before putting it in */
intermediate *object_word(char *token) {
	intermediate *new = NULL;
	int objno = 0;

	objno = atol(token + 1);
	new = new_inst();
	new -> no = nowords++;
	new -> in.type = PROG_OBJECT;
	new -> in.data.objref = objno;
	return new;
}

/* support routines for internal data structures. */

/* add procedure to procedures list */
void add_proc(char *proc_name, intermediate *place) {
	proc_list *new = NULL;

	new = (proc_list *) malloc(sizeof(proc_list));
	new -> name = dup_string(proc_name);
	new -> code = place;
	new -> next = procs;
	procs = new;
}

/* add if to if stack */
void addif(intermediate *place, int from) {
	if_stack *new = NULL;

	new = (if_stack *) malloc(sizeof(if_stack));
	new->place = place;
	new->next = ifs;
	new->source = from;
	ifs = new;
}

/* queries the type of the top element on the if stack */
/* non destructive */
int query_if() {
	if (!ifs)
		return SOURCE_ERROR;
	return (ifs->source);
}

/* checks nested for depth */
int for_nest() {
	int result = 0;
	if_stack *temp = ifs;

	for (; temp; temp = temp->next)
		if (temp->source == SOURCE_FOR)
			result++;

	return (result);

}

/* pops topmost if off the stack */
intermediate *find_if() {
	intermediate *temp = NULL;
	if_stack *tofree = NULL;

	if (!ifs)
		return 0;

	temp = ifs->place;
	tofree = ifs;
	ifs = ifs->next;
	free(tofree);
	return temp;
}

/* adds variable.  Return 0 if no space left */
int add_variable(char *varname) {
	int i = 0;

	for (i = RES_VAR; i < MAX_VAR; i++)
		if (!variables[i])
			break;

	if (i == MAX_VAR)
		return 0;

	variables[i] = dup_string(varname);
	return i;
}

/* predicates for procedure calls */
int special(char *token) {
	return (token && !(string_compare(token, ":") && string_compare(token, ";")
			&& string_compare(token, "IF") && string_compare(token, "ELSE")
			&& string_compare(token, "THEN") && string_compare(token, "CALL")
			&& string_compare(token, "FOR") && string_compare(token, "BEGIN")
			&& string_compare(token, "WHILE") &&
#ifdef BREAK_CONTINUE
			string_compare(token, "BREAK") &&
			string_compare(token, "CONTINUE") &&
#endif
			string_compare(token, "DO") && string_compare(token, "LOOP")
			&& string_compare(token, "EXIT") && string_compare(token, "VAR")));
}

/* see if procedure call */
int call(char *token) {
	proc_list *i = NULL;

	for (i = procs; i; i = i -> next)
		if (!string_compare(i -> name, token))
			return 1;

	return 0;
}

/* see if it's a quoted procedure name */
int quoted(char *token) {
	return (*token == '\'' && call(token + 1));
}

/* see if it's an object # */
int object_check(char *token) {
	if (*token == '#' && number(token + 1))
		return 1;
	else
		return 0;
}

/* see if string */
int string(char *token) {
	return (token[0] == '"');
}

int variable(char *token) {
	int i = 0;

	for (i = 0; i < MAX_VAR && variables[i]; i++)
		if (!string_compare(token, variables[i]))
			return 1;

	return 0;
}

/* see if token is primitive */
int primitive(char *token) {
	return get_primitive(token);
}

/* return primitive instruction */
int get_primitive(char *token) {
	hash_data *hd = NULL;

	if ((hd = find_hash(token, primitive_list, COMP_HASH_SIZE)) == NULL)
		return 0;
	else
		return (hd -> ival);
}

/* clean up as nicely as we can. */
void cleanup() {
	intermediate *wd = NULL, *tempword = NULL;
	if_stack *eef = NULL, *tempif = NULL;
	proc_list *p = NULL, *tempp = NULL;
	int i = 0;

	for (wd = first_word; wd; wd = tempword) {
		tempword = wd -> next;
		if (wd -> in.type == PROG_STRING)
			if (wd -> in.data.string)
				free(wd->in.data.string);
		free(wd);
	}
	first_word = 0;

	for (eef = ifs; eef; eef = tempif) {
		tempif = eef -> next;
		free(eef);
	}
	ifs = NULL;

	for (p = procs; p; p = tempp) {
		tempp = p -> next;
		free(p -> name);
		free(p);
	}
	procs = 0;

	for (i = RES_VAR; i < MAX_VAR && variables[i]; i++) {
		free(variables[i]);
		variables[i] = 0;
	}
}

/* copy program to an array */
void copy_program() {
	/* Everything should be peachy keen now, so we don't do any error
	 checking                                                    */
	intermediate *curr = NULL;
	inst *code = NULL;
	int i = 0;

	if (!first_word)
		v_abort_compile("Nothing to compile.");

	code = (inst *) malloc(sizeof(inst) * (nowords + 1));

	i = 0;
	for (curr = first_word; curr; curr = curr -> next) {
		code[i].type = curr -> in.type;
		code[i].linenum = curr -> in.linenum;
		switch (code[i].type) {
		case PROG_PRIMITIVE:
		case PROG_INTEGER:
		case PROG_FLOAT:
		case PROG_VAR:
			code[i].data.number = curr -> in.data.number;
			break;
		case PROG_STRING:
			code[i].data.string = curr -> in.data.string ? dup_string(
					curr->in.data.string) : 0;
			break;
		case PROG_OBJECT:
			code[i].data.objref = curr -> in.data.objref;
			break;
		case PROG_ADD:
			code[i].data.call = code + curr -> in.data.number;
			break;
		default:
			v_abort_compile("Unknown type compile!  Internal error.")
			;
			break;
		}
		i++;
	}
	DBSTORE(program, sp.program.code, code);
}

void set_start() {
	DBSTORE(program, sp.program.siz, nowords);
	DBSTORE(program, sp.program.start,
			(DBFETCH(program)->sp.program.code + procs -> code -> no));
}

/* allocate and initialize data linked structure. */
intermediate * new_inst() {
	intermediate *new = NULL;

	new = (intermediate *) malloc(sizeof(intermediate));
	new -> next = 0;
	new -> no = 0;
	new -> in.type = 0;
	new -> in.linenum = 0;
	new -> in.data.number = 0;
	return new;
}

void free_prog(inst *c, int siz) {
	int i = 0;

	for (i = 0; i < siz; i++)
		if (c[i].type == PROG_STRING && c[i].data.string)
			free(c[i].data.string);

	if (c)
		free(c);
	DBSTORE(program, sp.program.code, 0);
	DBSTORE(program, sp.program.siz, 0);
}

static void add_primitive(int val) {
	hash_data hd;

	hd.ival = val;
	if (add_hash(base_inst[val - BASE_MIN], hd, primitive_list, COMP_HASH_SIZE)
			== NULL)
		panic("Out of memory");
	else
		return;
}

void clear_primitives() {
	kill_hash(primitive_list, COMP_HASH_SIZE, 0);
	return;
}

void init_primitives() {
	int i = 0;

	fprintf(stderr, "Initializing primitives %d thru %d\n", BASE_MIN, BASE_MAX);

	clear_primitives();
	for (i = BASE_MIN; i <= BASE_MAX; i++) {
		add_primitive(i);
#ifdef NOISY_PRIMS
		fprintf(stderr, "%d : %s\n",i,base_inst[i-BASE_MIN]);
#endif
	}
}

void uncompile_program(dbref i, dbref player1, char *buf) {
	/* free program */
	bump_frames(buf, i, player1);
	free_prog(DBFETCH(i)->sp.program.code, DBFETCH(i)->sp.program.siz);

	/*  cleanpubs(DBFETCH(i)->sp.program.pubs); */
	/*  DBFETCH(i)->sp.program.pubs = NULL; */

	DBFETCH(i)->sp.program.first = 0;
	DBFETCH(i)->sp.program.curr_line = 0;
	DBFETCH(i)->sp.program.siz = 0;
	DBFETCH(i)->sp.program.code = 0;
	DBFETCH(i)->sp.program.start = 0;
}

void do_uncompile(__DO_PROTO) /* Add program matching to this */
{
	dbref i = 0l;
	char buf[100];

	if (!Wizard(player)) {
		notify(player, player, "Permission denied.");
		return;
	}
	for (i = 0; i < db_top; i++) {
		if (Typeof(i) == TYPE_PROGRAM) {
			snprintf(buf, BUFFER_LEN, "Program %s uncompiled by %s", unparse_name(i),
					unparse_name(player));
			uncompile_program(i, player, buf);
		}
	}
	notify(player, player, "All programs decompiled.");
}
