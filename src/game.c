#include "copyright.h"
#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/times.h>

#include "db.h"
#include "params.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

/* declarations */
static char *dumpfile = 0;
static int alarm_PANIC = 0;
static int last_dump = 0;

#ifdef HAVE_DRAND48
void srand48(long);
#endif

/* funct (player, a, b, line) */
/* where the command line is "command a = b" */
typedef struct {
	char *name;
	void (*function)(dbref, char *, char *, char *);
	int flag;
} game_command;

void fork_and_dump(void);
void dump_database(void);
void clear_args(char *one, char *two, char *three);

/* more hacks, let them dump a lachesis file... */
static int dump_lachesis = 0;

void do_dump(__DO_PROTO) {
	if (Wizard(player)) {
		/* more hacks to dump a lachesis db */
		if (arg2 && !strcmp(arg2, "lachesis")) {
			dump_lachesis = 1;
			notify(player, player, "Switching to Lachesis format.");
		}

		if (*arg1
#ifdef GOD_PRIV
				&& God(player)
#endif /* GOD_PRIV */
		) {
			strcpy(dumpfile, arg1);
			notify(player, player, "Dumping to file %s...", arg1);
		}
		last_dump = 0;
		notify(player, player, "Dumped the DB to disk.");
	} else
		notify(player, player, "Sorry, you are in a no dumping zone.");
}

void do_shutdown(__DO_PROTO) {
	descriptor_data *d = NULL;
	if (FLAGS(player) &
#ifdef GOD_PRIV
			GOD)
#else /* !GOD_PRIV */
	WIZARD)
#endif /* GOD_PRIV */
	{
		for (d = descriptor_list; d; d = d->next) {
			queue_string(d, "%s has issued a shutdown.\r\n", unparse_name(player));
		}
		log_status("SHUTDOWN: by %s\n", unparse_object(player, player));
		shutdown_flag = 1;
	} else {
		notify(player, player,
				"Your delusions of grandeur have been duly noted.");
		log_status("ILLEGAL SHUTDOWN: tried by %s\n",
				unparse_object(player, player));
	}
}

int epoch = 0;

static void dump_database_internal() {
	char tpfile[BUFFER_LEN];
	FILE *f = NULL;

	snprintf(tpfile, BUFFER_LEN, "%s.#%d#", dumpfile, epoch - 1);
	(void) unlink(tpfile); /* nuke our predecessor */

	snprintf(tpfile, BUFFER_LEN, "%s.#%d#", dumpfile, epoch);

	if ((f = fopen(tpfile, "w")) != NULL) {
		dump_lachesis ? db_write_lachesis(f) : db_write(f);
		dump_lachesis = 0;
		fclose(f);
		if (rename(tpfile, dumpfile) < 0)
			perror(tpfile);
	} else
		perror(tpfile);

	/* Write out the macros */
	snprintf(tpfile, BUFFER_LEN, "%s.#%d#", MACRO_FILE, epoch - 1);
	unlink(tpfile);

	snprintf(tpfile, BUFFER_LEN, "%s.#%d#", MACRO_FILE, epoch);

	if ((f = fopen(tpfile, "w")) != NULL) {
		macrodump(macrotop, f);
		fclose(f);
		if (rename(tpfile, MACRO_FILE) < 0)
			perror(tpfile);
	} else
		perror(tpfile);
}

void panic(char *message) {
	char panicfile[BUFFER_LEN];
	FILE *f = NULL;
	int i = 0;

	log_status("PANIC: %s\n", message);
	fprintf(stderr, "PANIC: %s\n", message);

	/* turn off signals */
	for (i = 0; i < NSIG; i++)
		signal(i, SIG_IGN);

	/* shut down interface */
	close_sockets();

	/* dump panic file */
	snprintf(panicfile, BUFFER_LEN, "%s.PANIC", dumpfile);
	if ((f = fopen(panicfile, "w")) == NULL) {
		perror("CANNOT OPEN PANIC FILE, YOU LOSE");
#ifdef NOCOREDUMP
		_exit(135);
#else /* !NOCOREDUMP */
		signal(SIGIOT, SIG_DFL);
		abort();
#endif /* NOCOREDUMP */
	} else {
		log_status("DUMPING: %s\n", panicfile);
		fprintf(stderr, "DUMPING: %s\n", panicfile);
		dump_lachesis ? db_write_lachesis(f) : db_write(f);
		dump_lachesis = 0;
		fclose(f);
		log_status("DUMPING: %s (done)\n", panicfile);
		fprintf(stderr, "DUMPING: %s (done)\n", panicfile);
#ifdef NOCOREDUMP
		_exit(136);
#else /* !NOCOREDUMP */
		signal(SIGIOT, SIG_DFL);
		abort();
#endif /* NOCOREDUMP */
	}
}

void dump_database() {
	epoch++;

	log_status("DUMPING: %s.#%d#\n", dumpfile, epoch);
	dump_database_internal();
	log_status("DUMPING: %s.#%d# (done)\n", dumpfile, epoch);
}

void time_keeper() {
	static int last_rwho = 0;
	static int last_setup = 0;
	static int current_time = 0;

	if (!last_setup) {
		last_setup = 1;
		last_dump = time((long *) NULL);
		last_rwho = time((long *) NULL);
		autostart_frames();
	}

	current_time = time((long *) NULL);

	run_frames();

	if ((current_time - last_dump) > DUMP_INTERVAL || alarm_PANIC == 1) {
		alarm_PANIC = 0;
		fork_and_dump();
	}
}

void fork_and_dump() {
	epoch++;
	last_dump = time((long *) NULL);

	log_status("CHECKPOINTING: %s.#%d#\n", dumpfile, epoch);
	dump_database_internal();
}

int init_game(char *infile, char *outfile) {
	FILE *f = NULL;

	if ((f = fopen(MACRO_FILE, "r")) == NULL)
		log_status("INIT: Macro storage file %s is tweaked.\n", MACRO_FILE);
	else {
		macroload(f);
		fclose(f);
	}

	if ((f = fopen(infile, "r")) == NULL)
		return -1;

	/* ok, read it in */
	log_status("LOADING: %s\n", infile);
	fprintf(stderr, "LOADING: %s\n", infile);
	if (db_read(f) < 0)
		return -1;
	log_status("LOADING: %s (done)\n", infile);
	fprintf(stderr, "LOADING: %s (done)\n", infile);
	fclose(f);

	/* everything ok */

	/* initialize random number generator */
	srandom(getpid());

#ifdef HAVE_DRAND48
	srand48(getpid());
#endif

	/* set up dumper */
	if (dumpfile)
		free(dumpfile);
	dumpfile = dup_string(outfile);

	return 0;
}

game_command command_array[] = { { "@action", do_action, 0 }, { "@attach",
		do_attach, 1 }, { "@backlinks", do_backlinks, 1 }, { "@backlocks",
		do_backlocks, 1 }, { "@boot", do_boot, 1 }, { "@chown", do_chown, 1 },
		{ "@contents", do_contents, 1 }, { "@create", do_create, 1 }, {
				"@describe", do_describe, 0 }, { "@dig", do_dig, 1 }, {
				"@drop", do_drop_message, 1 }, { "@dump", do_dump, 0 }, {
				"@earthquake", do_earthquake, 0 }, { "@edit", do_edit, 0 }, {
				"@examine", do_at_examine, 1 }, { "@exits", do_exits, 1 }, {
				"@fail", do_fail, 0 }, { "@find", do_find, 1 }, { "@flags",
				do_list_flags, 0 }, { "@force", do_force, 1 }, { "@go", do_go,
				1 }, { "@kill", do_pidkill, 0 }, { "@link", do_link, 1 }, {
				"@list", do_mlist, 0 }, { "@lock", do_lock, 0 }, { "@logins",
				do_login, 0 }, { "@name", do_name, 0 }, { "@newpassword",
				do_newpassword, 0 }, { "@odrop", do_odrop, 0 }, { "@ofail",
				do_ofail, 0 }, { "@open", do_open, 1 }, { "@osuccess",
				do_osuccess, 0 }, { "@owned", do_owned, 1 }, { "@parameter",
				do_parameter_set, 0 }, { "@password", do_password, 0 }, {
				"@pcreate", do_pcreate, 0 }, { "@permissions", do_perms, 1 }, {
				"@program", do_program, 0 },
		{ "@properties", do_properties, 1 }, { "@ps", do_ps, 0 }, { "@recycle",
				do_recycle, 1 }, { "@reset", do_reset, 0 },
		{ "@set", do_set, 0 }, { "@shutdown", do_shutdown, 0 }, { "@stats",
				do_stats, 1 }, { "@stuid", do_player_setuid, 1 }, { "@success",
				do_success, 0 }, { "@teleport", do_teleport, 1 }, { "@toad",
				do_toad, 1 }, { "@trace", do_trace, 0 }, { "@trimdb",
				do_trimdb, 0 }, { "@uncompile", do_uncompile, 0 }, { "@unlink",
				do_unlink, 1 }, { "@unlock", do_unlock, 1 }, { "@user",
				do_player_setuid, 1 }, { "@version", do_version, 0 }, {
				"@wall", do_wall, 1 }, { "drop", do_drop, 1 }, { "examine",
				do_examine, 1 }, { "get", do_get, 1 }, { "give", do_give, 1 },
		{ "goto", do_move, 1 }, { "gripe", do_gripe, 1 },
		{ "help", do_help, 0 }, { "inventory", do_inventory, 0 }, { "kill",
				do_kill, 1 }, { "look", do_look_at, 1 }, { "man", do_man, 0 },
		{ "move", do_move, 1 }, { "news", do_news, 0 }, { "page", do_page, 1 },
		{ "pose", do_pose, 1 }, { "put", do_drop, 1 },
		{ "read", do_look_at, 1 }, { "rob", do_rob, 1 }, { "say", do_say, 1 },
		{ "score", do_score, 0 }, { "take", do_get, 1 },
		{ "throw", do_drop, 1 }, { "whisper", do_whisper, 1 } };

void process_command(dbref player, char *command, dbref cause) {
	char *arg1 = NULL, *arg2 = NULL, *full_command = NULL, *p = NULL;
	char pbuf[BUFFER_LEN], xbuf[BUFFER_LEN];
	int command_num = 0, i = 0, count = 0, flag = 1;
	frame *fr = NULL;

	if (!command)
		abort();

	if (player < 0 || player >= db_top) {
		log_status("process_command: bad player %d\n", player);
		return;
	}

	if (o_log_commands) {
		if (!(FLAGS(player) & INTERACTIVE))
			log_command("%s(%ld) in %s(%ld):%s %s\n", NAME(player), player,
					NAME(DBFETCH(player)->location),
					DBFETCH(player)->location,
					(FLAGS(player) & INTERACTIVE) ? " [interactive]" : " ",
					command);
	}

	/* Only players can go Interactive. */
	if (FLAGS(player) & INTERACTIVE) {
		if (Typeof(player) != TYPE_PLAYER) {
			notify(OWNER(player), OWNER(player), "%s is interactive!",
					NAME(player));
			log_status("%s(%ld) is INTERACTIVE\n", NAME(player), player);
			FLAGS(player) &= ~INTERACTIVE;
			DBDIRTY(player);
		} else
			interactive(player, command);
		return;
	}

	/* eat leading whitespace */
	while (*command && isspace(*command))
		command++;

	/* check for single-character commands */
	if ((*command == SAY_TOKEN) || (*command == SAY_TOKEN_TWO)) {
		snprintf(pbuf, BUFFER_LEN, "say %s", command + 1);
		command = pbuf;
	} else if ((*command == POSE_TOKEN) || (*command == POSE_TOKEN_TWO)) {
		snprintf(pbuf, BUFFER_LEN, "pose %s", command + 1);
		command = pbuf;
	}

	if ((can_move(player, command)) && ((*command != '!') || (!Wizard(player)))) {
		/* command is an exact match for an exit */
		if (*command == '!')
			command++;
		do_move(player, "", "", command);
		*match_args = 0;
	} else {
		if (*command == '!')
			command++;
		if (*command == '.')
			*command = '@';
		full_command = strcpy(xbuf, command);
		for (; *full_command && !isspace(*full_command); full_command++)
			;
		if (*full_command)
			full_command++;

		/* find arg1 -- move over command word */
		for (arg1 = command; *arg1 && !isspace(*arg1); arg1++)
			;
		/* truncate command */
		if (*arg1)
			*arg1++ = '\0';

		/* move over spaces */
		while (*arg1 && isspace(*arg1))
			arg1++;

		/* find end of arg1, start of arg2 */
		for (arg2 = arg1; *arg2 && *arg2 != ARG_DELIMITER; arg2++)
			;

		/* truncate arg1 */
		for (p = arg2 - 1; p >= arg1 && isspace(*p); p--)
			*p = '\0';

		/* go past delimiter if present */
		if (*arg2)
			*arg2++ = '\0';
		while (*arg2 && isspace(*arg2))
			arg2++;

		for (command_num = 0; (command_num < sizeof(command_array)
				/ sizeof(game_command)) && (stringn_compare(command,
				command_array[command_num].name, strlen(command)) > 0); command_num++)
			;

		if ((command_num < sizeof(command_array) / sizeof(game_command))
				&& (!stringn_compare(command, command_array[command_num].name,
						strlen(command)))
				&& (((command_num + 1 >= sizeof(command_array)
						/ sizeof(game_command)) || (stringn_compare(command,
						command_array[command_num + 1].name, strlen(command)))))) {
			(*command_array[command_num].function)(player, arg1, arg2,
					full_command);
			return;
		}

		if (o_muffail) {
			i = DBFETCH(GLOBAL_ENVIRONMENT)->exits;
			DOLIST (i, i) {
				if (!strcmp(DBFETCH(i)->name, "do_fail") && (FLAGS(i) & WIZARD)) {
					for (count = 0; count < DBFETCH(i)->sp.exit.ndest; count++) {
						if (Typeof(DBFETCH(i)->sp.exit.dest[count])
								== TYPE_PROGRAM) {
							fr = new_frame(player,
									DBFETCH(i)->sp.exit.dest[count], i,
									DBFETCH(player)->location, 1);
							run_frame(fr, 1);
							if (fr && (fr->status != STATUS_SLEEP))
								free_frame(fr);
							flag = 0;
						}
					}
				}
			}
		}
		if (flag)
			notify(player, player, "Huh?  (Type \"help\" for help.)");

		if (o_log_huhs) {
			if (!controls(player, DBFETCH(player)->location))
				log_status("HUH from %s(%d) in %s(%d)[%s]: %s %s\n",
						NAME(player), player,
						NAME(DBFETCH(player)->location),
						DBFETCH(player)->location,
						NAME(OWNER(DBFETCH(player)->location)), command,
						full_command);
		}
	}
}

void do_list_commands(__DO_PROTO) {
	int i;
	char buff[BUFFER_LEN];
	char *bp;
	bp = buff;

	safe_copy_str("Builtins:", buff, &bp, BUFFER_LEN);
	for (i = 0; i < sizeof(command_array) / sizeof(game_command); i++) {
		safe_chr(' ', buff, &bp);
		safe_copy_str(command_array[i].name, buff, &bp, BUFFER_LEN);
	}
	*bp = '\0';

	notify(player, player, buff);
}
