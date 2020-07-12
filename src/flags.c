/* FLAG STUFF */
#include "config.h"
#include "params.h"
#include "db.h"
#include "interface.h"
#include "externs.h"

/*  Flag name,  letter,  Type,       FLAG    */
FLAG flag_table[] = { { "ABODE", 'A', TYPE_ROOM, ABODE }, { "AUTOSTART", 'A',
		TYPE_EXIT, ABODE }, { "AUTHOR", 'A', TYPE_PLAYER, ABODE }, { "BUILDER",
		'B', NOTYPE, BUILDER }, { "CHOWN_OK", 'C', NOTYPE, CHOWN_OK }, {
		"DARK", 'D', NOTYPE, DARK }, { "DEBUG", 'D', TYPE_PROGRAM, DARK }, {
		"ENTER_OK", 'e', NOTYPE, ENTER_OK }, { "GOD", 'G', NOTYPE, GOD }, {
		"HAVEN", 'H', NOTYPE, HAVEN }, { "HEARING", 'H', TYPE_EXIT, HAVEN }, {
		"INTERACTIVE", 'I', TYPE_PLAYER, INTERACTIVE }, { "JUMP_OK", 'J',
		NOTYPE, JUMP_OK }, { "LINK_OK", 'L', NOTYPE, LINK_OK }, { "MONITOR",
		'M', TYPE_PLAYER, MUCKER }, { "MUCKER", 'M', TYPE_PLAYER, MUCKER }, {
		"NOSPOOF", 'N', TYPE_PLAYER, NOSPOOF },
		{ "QUELL", 'Q', NOTYPE, QUELL }, { "SAFE", 's', NOTYPE, SAFE }, {
				"SETUID", 'S', TYPE_PROGRAM, STICKY }, { "SILENT", 'S',
				TYPE_PLAYER, STICKY }, { "STICKY", 'S', NOTYPE, STICKY }, {
				"UNFINDABLE", 'U', NOTYPE, UNFIND }, { "VERBOSE", 'V', NOTYPE,
				VERBOSE }, { "VISUAL", 'v', NOTYPE, VISUAL }, { "WIZARD", 'W',
				NOTYPE, WIZARD }, };

char *unparse_flags(dbref thing) {
	static char buf[BUFFER_LEN];
	char *type_codes = "R-EPFG*";
	char *p = NULL;
	char hold = '\0';
	int command_num = 0;

	p = buf;
	if (Typeof(thing) != TYPE_THING)
		*p++ = type_codes[Typeof(thing)];
	if (FLAGS(thing) & ~TYPE_MASK) {
		/* print flags */
		for (command_num = 0; command_num < sizeof(flag_table) / sizeof(FLAG); command_num++) {
			if (FLAGS(thing) & (flag_table[command_num].flag) && (Typeof(thing)
					== flag_table[command_num].type
					|| flag_table[command_num].type == NOTYPE) && hold
					!= flag_table[command_num].letter) {
				*p++ = (flag_table[command_num].letter);
				hold = (flag_table[command_num].letter);
			}
		}
	}
	*p = '\0';
	return buf;
}

int strn_cmp(char *sub, char *string) {
	if (!*sub || !*string)
		return 0;
	if (!strncasecmp(sub, string, strlen(sub)))
		return 1;
	return 0;
}

FLAG *flag_lookup(char *name, dbref thing) {
	int command_num;

	for (command_num = 0; (command_num < sizeof(flag_table) / sizeof(FLAG))
			&& (stringn_compare(name, flag_table[command_num].name,
					strlen(name)) > 0); command_num++)
		;
	if ((command_num < sizeof(flag_table) / sizeof(FLAG)) && (!stringn_compare(
			name, flag_table[command_num].name, strlen(name)))
			&& (Typeof(thing) == flag_table[command_num].type
					|| flag_table[command_num].type == NOTYPE))
		return (&flag_table[command_num]);
	return NULL;
}

void do_list_flags(__DO_PROTO) {
	int i = 0;
	char buff[BUFFER_LEN];
	char *bp = NULL;

	bp = buff;

	safe_copy_str("Flags:", buff, &bp, BUFFER_LEN);
	for (i = 0; i < sizeof(flag_table) / sizeof(FLAG); i++) {
		safe_chr(' ', buff, &bp);
		safe_copy_str(flag_table[i].name, buff, &bp, BUFFER_LEN);
	}
	*bp = '\0';

	notify(player, player, buff);
}
