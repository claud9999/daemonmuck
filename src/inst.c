#include "copyright.h"
#include "config.h"

#include "db.h"
#include "inst.h"
#include "externs.h"
#include "interface.h"

char *base_inst[] = { PRIMS_CONVERSIONS_TL,
        PRIMS_INTERACTION_TL,
        PRIMS_LOGIC_TL,
        PRIMS_OPERANDS_TL,
        PRIMS_OBJECTS_TL,
        PRIMS_CREATE_TL,
        PRIMS_PROPERTY_TL,
        PRIMS_STACK_TL,
        PRIMS_STRINGS_TL,
        PRIMS_TESTS_TL,
        PRIMS_TIME_TL,
        PRIMS_DESCRIPTOR_TL,
        PRIMS_FOR_TL, "IF" ,
"CALL",
"READ",
"EXIT",
"JMP",
"PROGRAM ",
"EXECUTE",
"SLEEP",
"VAR",
"LOOP",
"NOP",
};

/* converts an instruction into a printable string, stores the string in
 an internal buffer and returns a pointer to it */
char *insttotext(inst *theinst) {
	static char buffer[64];

	switch (theinst->type) {
	case PROG_PRIMITIVE:
		if (theinst->data.number >= BASE_MIN && theinst->data.number
				<= BASE_MAX)
			strcpy(buffer, base_inst[theinst->data.number - BASE_MIN]);
		else
			strcpy(buffer, "???");
		break;
	case PROG_STRING:
		if (!theinst->data.string) {
			strcpy(buffer, "\"\"");
			break;
		}
		snprintf(buffer, 64, "\"%1.29s", theinst->data.string);
		if (strlen(theinst->data.string) <= 30)
			strcat(buffer, "\"");
		else
			strcat(buffer, "_");
		break;
	case PROG_INTEGER:
		snprintf(buffer, 64, "%ld", theinst->data.number);
		break;
	case PROG_FLOAT:
		snprintf(buffer, 64, "%G", theinst->data.fnum);
		break;
	case PROG_ADD:
		strcpy(buffer, "addr");
		break;
	case PROG_OBJECT:
		snprintf(buffer, 64, "#%ld", theinst->data.objref);
		break;
	case PROG_VAR:
		snprintf(buffer, 64, "V%ld", theinst->data.number);
		break;
	default:
		strcpy(buffer, "???");
		break;
	}
	return buffer;
}

char *insttoerr(inst *theinst) {
	char *foo = NULL;
	char smallbuf[12];

	snprintf(smallbuf, 12, "%d", theinst->linenum);

	foo = insttotext(theinst);
	strcat(foo, " (Line ");
	strcat(foo, smallbuf);
	strcat(foo, ")");
	return foo;
}

char buffer[BUFFER_LEN];
/* produce one line summary of current state.  Note that sp is the next
 space on the stack -- 0..sp-1 is the current contents. */
char *debug_inst(inst *pc, inst *stack1, int sp) {
	int count = 0;
	int buffer_pos = 0;

	buffer_pos = snprintf(buffer, BUFFER_LEN - 1, "Debug %d> Stack( ", pc->linenum);
	if (sp > 5)
		buffer_pos += snprintf(buffer + buffer_pos, BUFFER_LEN - buffer_pos, "..., ");
	count = (sp > 5) ? sp - 5 : 0;
	while (count < sp && buffer_pos < BUFFER_LEN) {
		buffer_pos += snprintf(buffer + buffer_pos, BUFFER_LEN - buffer_pos, "%s", insttotext(stack1 + count));
		if (buffer_pos < BUFFER_LEN) buffer_pos += snprintf(buffer + buffer_pos, BUFFER_LEN - buffer_pos, "%s", (++count < sp) ? ", " : "");
	}
	if (buffer_pos < BUFFER_LEN) buffer_pos += snprintf(buffer + buffer_pos, BUFFER_LEN - buffer_pos, " ) %s", insttotext(pc));
	return buffer;
}
