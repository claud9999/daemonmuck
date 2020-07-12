#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "interface.h"
#include "inst.h"

void disassemble(dbref player, dbref program) {
	inst *curr = NULL;
	inst *codestart = NULL;
	int i = 0;

	codestart = curr = DBFETCH(program)->sp.program.code;
	if (!DBFETCH(program)->sp.program.siz) {
		notify(player, player, "Nothing to disassemble!");
		return;
	}
	for (i = 0; i < DBFETCH(program)->sp.program.siz; i++, curr++) {
		switch (curr->type) {
		case PROG_PRIMITIVE:
			if (curr->data.number >= BASE_MIN && curr->data.number <= BASE_MAX)
				notify(player, player, "%d: PRIMITIVE: %s", i,
						base_inst[curr->data.number - BASE_MIN]);
			else
				notify(player, player, "%d: PRIMITIVE: %ld", i, curr->data.number);
			break;
		case PROG_STRING:
			notify(player, player, "%d: STRING: \"%s\"", i,
					curr->data.string ? curr->data.string : "");
			break;
		case PROG_INTEGER:
			notify(player, player, "%d: INTEGER: %ld", i, curr->data.number);
			break;
		case PROG_FLOAT:
			notify(player, player, "%d: FLOAT: %G", i, curr->data.fnum);
			break;
		case PROG_ADD:
			notify(player, player, "%d: ADDRESS: %d", i, curr->data.call - codestart);
			break;
		case PROG_OBJECT:
			notify(player, player, "%d: OBJECT REF: %ld", i, curr->data.number);
			break;
		case PROG_VAR:
			notify(player, player, "%d: VARIABLE: %ld", i, curr->data.number);
		}
	}
}
