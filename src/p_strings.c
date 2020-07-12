#include "prims.h"
#include "externs.h"
#include "version.h"
#include <fcntl.h>
#include <sys/stat.h>
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
static inst temp1, temp2;
extern int p_result;
extern char p_buf[BUFFER_LEN];
static char *string;
extern int p_nargs;

int file_ok(char *string) {
	if (index(string, '.') == NULL && index(string, '~') == NULL)
		return (1);
	else
		return (0);
}

int filesize_ok(char *filename) {
	int desn;
	struct stat stats;

	if ((desn = open(filename, O_RDONLY)) < 0) {
		close(desn);
		return (0);
	}

	fstat(desn, &stats);
	close(desn);

	if (stats.st_size > (MAX_OUTPUT - 350))
		return (0);
	else
		return (1);
}

void prims_stringcmp(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING || p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");

	p_result = string_compare(p_oper2->data.string ? p_oper2->data.string : "",
			p_oper1->data.string ? p_oper1->data.string : "");

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_stringncmp(__P_PROTO) {
	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	if (p_oper2->type != PROG_STRING || p_oper3->type != PROG_STRING)
		abort_interp("Non-string argument.");

	p_result = stringn_compare(
			p_oper3->data.string ? p_oper3->data.string : "",
			p_oper2->data.string ? p_oper2->data.string : "",
			p_oper1->data.number);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_strcmp(__P_PROTO) {
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING || p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");

	p_result = strcmp(p_oper2->data.string ? p_oper2->data.string : "",
			p_oper1->data.string ? p_oper1->data.string : "");

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_strncmp(__P_PROTO) {
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	if (p_oper2->type != PROG_STRING || p_oper3->type != PROG_STRING)
		abort_interp("Non-string argument.");

	p_result = strncmp(p_oper3->data.string ? p_oper3->data.string : "",
			p_oper2->data.string ? p_oper2->data.string : "",
			p_oper1->data.number);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_strcut(__P_PROTO) {
	CHECKOP(2);
	temp1 = *(p_oper1 = POP());
	temp2 = *(p_oper2 = POP());
	if (temp1.type != PROG_INTEGER)
		abort_interp("Non-integer argument (2)");
	if (temp1.data.number < 0)
		abort_interp("Argument must be a positive integer.");
	if (temp2.type != PROG_STRING)
		abort_interp("Non-string argument (1)");

	if (!temp2.data.string) {
		push(arg, top, PROG_STRING, 0);
		push(arg, top, PROG_STRING, 0);
	} else {
		if (temp1.data.number > strlen(temp2.data.string)) {
			push(arg, top, PROG_STRING, MIPSCAST dup_string(temp2.data.string));
			push(arg, top, PROG_STRING, 0);
		} else {
			strncpy(p_buf, temp2.data.string, temp1.data.number);
			p_buf[temp1.data.number] = '\0';
			push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
			if (strlen(temp2.data.string) > temp1.data.number) {
				strncpy(p_buf, temp2.data.string + temp1.data.number, strlen(
						temp2.data.string) - temp1.data.number + 1);
				push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
			} else {
				push(arg, top, PROG_STRING, 0);
			}
		}
	}

	CLEAR(&temp2);
}

void prims_strlen(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");

	if (!p_oper1->data.string)
		p_result = 0;
	else
		p_result = strlen(p_oper1->data.string);

	CLEAR(p_oper1);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_strcat(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING || p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");

	if (!p_oper1->data.string && !p_oper2->data.string)
		string = NULL;
	else if (!p_oper2->data.string)
		string = dup_string(p_oper1->data.string);
	else if (!p_oper1->data.string)
		string = dup_string(p_oper2->data.string);
	else if (strlen(p_oper1->data.string) + strlen(p_oper2->data.string)
			> (BUFFER_LEN) - 1) {
		abort_interp("Operation would p_result in overflow.");
	} else {
		strncpy(p_buf, p_oper2->data.string, strlen(p_oper2->data.string));
		strncpy(p_buf + strlen(p_oper2->data.string), p_oper1->data.string,
				strlen(p_oper1->data.string) + 1);
		string = dup_string(p_buf);
	}

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_STRING, MIPSCAST string);
}

void prims_explode(__P_PROTO) {
	int i;
	char *delimit;
	CHECKOP(2);
	temp1 = *(p_oper1 = POP());
	temp2 = *(p_oper2 = POP());
	if (temp1.type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (temp2.type != PROG_STRING)
		abort_interp("Non-string argument (1)");
	if (!temp1.data.string)
		abort_interp("Empty string argument (2)");

	delimit = temp1.data.string;
	if (!temp2.data.string) {
		p_result = 1;
		CLEAR(&temp1);
		CLEAR(&temp2);
		push(arg, top, PROG_STRING, 0);
		push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
		return;
	} else {
		p_result = 0;
		strncpy(p_buf, temp2.data.string, strlen(temp2.data.string) + 1);
		for (i = strlen(temp2.data.string) - 1; i >= 0; i--) {
			if (!strncmp(p_buf + i, delimit, strlen(temp1.data.string))) {
				p_buf[i] = '\0';
				if (*top >= STACK_SIZE)
					abort_interp("Stack Overflow.");
				push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf + i
						+ strlen(temp1.data.string)));
				p_result++;
			}
		}
		if (*top >= STACK_SIZE)
			abort_interp("Stack Overflow.");
		push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
		p_result++;
	}
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow.");

	CLEAR(&temp1);
	CLEAR(&temp2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_subst(__P_PROTO) {
	int i = 0, j = 0;
	char *match, *replacement;
	char xp_buf[BUFFER_LEN];

	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	if (!p_oper1->data.string)
		abort_interp("Empty string argument (3)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (3)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (p_oper3->type != PROG_STRING)
		abort_interp("Non-string argument (1)");

	p_buf[0] = '\0';
	if (p_oper3->data.string) {
		strncpy(xp_buf, p_oper3->data.string, strlen(p_oper3->data.string) + 1);
		match = p_oper1->data.string;
		replacement = DoNullInd(p_oper2->data.string);
		while (xp_buf[i]) {
			if (!strncmp(xp_buf + i, match, strlen(p_oper1->data.string))) {
				strcat(p_buf, replacement);
				i += strlen(p_oper1->data.string);
				j += *replacement ? strlen(p_oper2->data.string) : 0;
			} else {
				p_buf[j++] = xp_buf[i++];
				p_buf[j] = '\0';
			}
		}
	}

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_instr(__P_PROTO) {
	char *remaining, *match;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument type (2)");
	if (!(p_oper1->data.string))
		abort_interp("Empty string argument (2)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (1)");
	if (!p_oper2->data.string)
		p_result = 0;
	else {
		remaining = p_oper2->data.string;
		match = p_oper1->data.string;
		p_result = 0;
		do {
			if (!strncmp(remaining, match, strlen(p_oper1->data.string))) {
				p_result = remaining - p_oper2->data.string + 1;
				break;
			}
			remaining++;
		} while (remaining >= p_oper2->data.string && *remaining);
	}

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_rinstr(__P_PROTO) {
	char *remaining, *match;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument type (2)");
	if (!(p_oper1->data.string))
		abort_interp("Empty string argument (2)");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument (1)");

	if (!p_oper2->data.string)
		p_result = 0;
	else {
		remaining = p_oper2->data.string + strlen(p_oper2->data.string) - 1;
		match = p_oper1->data.string;
		p_result = 0;
		do {
			if (!strncmp(remaining, match, strlen(p_oper1->data.string))) {
				p_result = remaining - p_oper2->data.string + 1;
				break;
			}
			remaining--;
		} while (remaining >= p_oper2->data.string && *remaining);
	}
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_pronoun_sub(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP(); /* p_oper1 is a string, p_oper2 a dbref */
	if (!valid_object(p_oper2))
		abort_interp("Invalid argument (1)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Invalid argument (2)");

	if (p_oper1->data.string)
		strcpy(p_buf, p_oper1->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);

	push(arg, top, PROG_STRING, (p_oper1->data.string) ? MIPSCAST dup_string(
			pronoun_substitute(p_oper2->data.objref, p_buf)) : 0);
}

void prims_toupper(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");

	if (p_oper1->data.string) {
		strncpy(p_buf, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
		upperstring(p_buf);
	} else {
		p_buf[0] = '\0';
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_tolower(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");

	if (p_oper1->data.string) {
		strncpy(p_buf, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
		lowerstring(p_buf);
	} else {
		p_buf[0] = '\0';
	}

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_flagstr(__P_PROTO) {
	dbref object;
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Non-object argument.");

	object = p_oper1->data.objref;
	strcpy(p_buf, (unparse_flags(object)));

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_caps(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string)
		abort_interp("NULL string argument.");

	strncpy(p_buf, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
	lowerstring(p_buf);
	p_buf[0] = toupper(p_buf[0]);

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_unparse_lock(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Non-object argument.");

	snprintf(p_buf, BUFFER_LEN, "%s", unparse_boolexp(player,
			DBFETCH(p_oper1->data.objref)->key));

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_unparse_flags(__P_PROTO) {
	dbref object;
	CHECKOP(1);
	p_oper1 = POP();
	if (!valid_object(p_oper1))
		abort_interp("Non-object argument.");

	object = p_oper1->data.objref;
	strcpy(p_buf, (flag_description(object)));

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_wstrcmp(__P_PROTO) {
	char buf2[BUFFER_LEN];
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if ((!p_oper1->data.string) || (!p_oper2->data.string))
		abort_interp("NULL string argument.");

	strncpy(p_buf, p_oper2->data.string, strlen(p_oper2->data.string) + 1);
	strncpy(buf2, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
	p_result = (wild_match(buf2, p_buf));

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_spitfile(__P_PROTO) {
	FILE *f; /*  OK, this one is easy. Taken from spitfile in help.c */
	char buf2[BUFFER_LEN], filename[BUFFER_LEN];
	;

	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string)
		abort_interp("NULL string argument.");

	strncpy(filename, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
	if (file_ok(filename) == 0)
		abort_interp("Sorry, you may not access MUF text files with a '.' or '~'.");

	snprintf(buf2, BUFFER_LEN, "%s%s", MUF_DIR, filename);
	if ((f = fopen(buf2, "r")) == NULL) {
		fclose(f);
		abort_interp("Non-existant text file.");
	}

	if ((filesize_ok(buf2)) == 0) {
		fclose(f);
		abort_interp("File size is larger then MAX_OUTPUT");
	}

	while (fgets(p_buf, sizeof p_buf, f)) {
		p_buf[(strlen(p_buf) - 1)] = '\0';
		notify(player, player, p_buf);
	}

	fclose(f);
	CLEAR(p_oper1);
}

void prims_notifyfile(__P_PROTO) {
	FILE *f;
	char buf2[BUFFER_LEN], filename[BUFFER_LEN];

	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	p_oper3 = POP();
	if (!valid_object(p_oper3) || Typeof(p_oper3->data.number) != TYPE_ROOM)
		abort_interp("Non-room argument (1)");
	if (p_oper2->type != PROG_OBJECT)
		abort_interp("Invalid object argument (2)");
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument (3)");
	if (!p_oper1->data.string)
		abort_interp("NULL string argument.");

	strncpy(filename, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
	if (file_ok(filename) == 0)
		abort_interp("Sorry, you may not access MUF text files with a '.' or '~'.");

	snprintf(buf2, BUFFER_LEN, "%s%s", MUF_DIR, filename);
	if ((f = fopen(buf2, "r")) == NULL) {
		fclose(f);
		abort_interp("Non-existant text file.");
	}

	if ((filesize_ok(buf2)) == 0) {
		fclose(f);
		abort_interp("File size is larger then MAX_OUTPUT");
	}

	while (fgets(p_buf, sizeof p_buf, f)) {
		p_buf[(strlen(p_buf) - 1)] = '\0';
		notify_except(player, p_oper3->data.objref, p_oper2->data.objref, p_buf);
	}

	fclose(f);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	CLEAR(p_oper3);
}

void prims_touchfile(__P_PROTO) {
	FILE *f;
	char filename[BUFFER_LEN];
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string)
		abort_interp("NULL string argument.");

	strncpy(filename, p_oper1->data.string, strlen(p_oper1->data.string) + 1);
	if (file_ok(filename) == 0)
		abort_interp("Sorry, you may not access MUF text files with a '.' or '~'.");

	snprintf(p_buf, BUFFER_LEN, "%s%s", MUF_DIR, filename);
	if ((f = fopen(p_buf, "r")) != NULL)
		p_result = 1;
	else
		p_result = 0;
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
	fclose(f);
	CLEAR(p_oper1);
}

void prims_re_compile(__P_PROTO) {
#ifndef REGEXP
	abort_interp("This prim not enabled.");
#else
	regexp *re;

	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING) abort_interp("Non-string argument.");
	re = m_regcomp(DoNullInd(p_oper1->data.string));
	if (! re) abort_interp(m_regerror());

	CLEAR(p_oper1);
	push(arg, top, PROG_RE,MIPSCAST re);
#endif
}

void prims_re_match(__P_PROTO) {
#ifndef REGEXP
	abort_interp("This prim not enabled.");
#else
	regexp *re;
	char *str;
	int rv;
	char *err;
	int i;

	CHECKOP(2);
	temp2 = *(p_oper2 = POP());
	temp1 = *(p_oper1 = POP());

	if (temp1.type != PROG_STRING) abort_interp("Non-string argument (1).");
	if (temp2.type != PROG_RE) abort_interp("Non-regexp argument (2).");

	str = (char *) DoNullInd(temp1.data.string);
	re = temp2.data.re;
	rv = m_regexec(re,str);
	err = m_regerror();
	if (err) abort_interp(err);
	if (rv)
	{	for (i=0;i<re->nsubexp;i++)
		{	if (re->startp[i])
			{	rv = re->endp[i] - re->startp[i];
				if (rv == 0)
				push(arg, top, PROG_STRING, 0);
				else
				{	bcopy(re->startp[i],&buf[0],rv);
					buf[rv] = '\0';
					push(arg, top, PROG_STRING, dup_string(&buf[0]));
				}
			}
			else
			{	rv = 0;
				push(arg, top, PROG_INTEGER, MIPSCAST &rv);
			}
		}
		rv = re->nsubexp;
		push(arg, top, PROG_INTEGER, MIPSCAST &rv);
	}
	else
	{	rv = 0;
		push(arg, top, PROG_INTEGER, MIPSCAST &rv);
	}
	CLEAR(&temp1);
	CLEAR(&temp2);
#endif
}

void prims_stringpfx(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING || p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string || !p_oper2->data.string)
		abort_interp("NULL string argument.");

	if (p_oper1->data.string == p_oper2->data.string)
		p_result = 0;
	else if (!(p_oper2->data.string && p_oper1->data.string))
		p_result = p_oper1->data.string ? -1 : 1;
	else
		p_result = string_prefix(p_oper2->data.string, p_oper1->data.string);

	CLEAR(p_oper1);
	CLEAR(p_oper2);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_striplead(__P_PROTO) { /* string -- string' */
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Not a string argument.");
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow.");

	strcpy(p_buf, DoNullInd(p_oper1->data.string));
	for (string = p_buf; *string && isspace(*string); string++)
		;

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(string));
}

void prims_striptail(__P_PROTO) { /* string -- string' */
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_STRING)
		abort_interp("Not a string argument.");
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow.");

	strcpy(p_buf, DoNullInd(p_oper1->data.string));
	p_result = strlen(p_buf);
	while ((p_result-- > 0) && isspace(p_buf[p_result]))
		p_buf[p_result] = '\0';

	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_smatch(__P_PROTO) {
	char xbuf[BUFFER_LEN];
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_STRING || p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper1->data.string || !p_oper2->data.string)
		abort_interp("Null string argument.");

	strcpy(p_buf, p_oper1->data.string);
	strcpy(xbuf, p_oper2->data.string);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	p_result = equalstr(p_buf, xbuf);
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_version(__P_PROTO) {
	push(arg, top, PROG_STRING, MIPSCAST dup_string(VERSION));
	if (*top >= STACK_SIZE)
		abort_interp("Stack Overflow.");
}

void prims_unparseobj(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_OBJECT)
		abort_interp("Non-object argument.");
	p_result = p_oper1->data.objref;
	switch (p_result) {
	case NOTHING:
		strncpy(p_buf, "*NOTHING*", BUFFER_LEN);
		break;
	case HOME:
		strncpy(p_buf, "*HOME*", BUFFER_LEN);
		break;
	default:
		if (p_result < 0 || p_result > db_top)
			strncpy(p_buf, "*INVALID*", BUFFER_LEN);
		else
			snprintf(p_buf, BUFFER_LEN, "%s", unparse_object(fr->euid, p_result));
	}
	CLEAR(p_oper1);
	push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_spitline(__P_PROTO) {
	FILE *f;
	char buf2[BUFFER_LEN], filename[BUFFER_LEN];
	int i = 1;

	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();

	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	if (p_oper1->data.number < 1)
		abort_interp("Invalid line number.");

	if (p_oper2->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!p_oper2->data.string)
		abort_interp("NULL argument.");

	strncpy(filename, p_oper2->data.string, strlen(p_oper2->data.string) + 1);
	if (file_ok(filename) == 0)
		abort_interp("Sorry, you may not access MUF text files with a '.' or '~'.");

	snprintf(buf2, BUFFER_LEN, "%s%s", MUF_DIR, filename);
	if ((f = fopen(buf2, "r")) == NULL) {
		fclose(f);
		abort_interp("Non-existant text file.");
	}

	while (fgets(p_buf, sizeof p_buf, f)) {
		fprintf(stderr, "TOKE:%s", p_buf);
		if (p_oper1->data.number == i) {
			fprintf(stderr, "STRING:%s", p_buf);
			p_buf[(strlen(p_buf) - 1)] = '\0';
			i = 0;
			break;
		}
		i++;
	}

	fclose(f);

	fprintf(stderr, "PRE:%s\n", p_buf);

	if (i)
		string = NULL;
	else
		string = dup_string(p_buf);

	fprintf(stderr, "POST:%s\n", p_buf);
	push(arg, top, PROG_STRING, MIPSCAST string);
	CLEAR(p_oper2);
}
