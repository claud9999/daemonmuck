#include <stdio.h>
#include "config.h"
#include "interface.h"
#include "params.h"
#include "externs.h"

char cg = '\0';
int ig = 0;

int cf_str(char *opt, char *val, int *loc, char **str, int maxlen, int p,
		dbref player);
int cf_int(char *opt, char *val, int *loc, char **str, int maxval, int p,
		dbref player);
int cf_bool(char *opt, char *val, int *loc, char **str, int maxval, int p,
		dbref player);
void conf_notify(dbref player, char *name, int *loc, char **str, int flag);
void config_set(char *opt, char *val);
void config_list(dbref player);
void conf_default_set();

typedef struct confparm CONF;

struct confparm {
	char *name; /* name of option */
	int (*handler)(); /* set option with this handler */
	int *loc; /* place to put integer option */
	char **str; /* place to put character option */
	int max; /* max: string length, integer value */
};

CONF conftable[] = {
		{ "gripe_file", cf_str, &ig, (char **) &o_gripe_file, 512 }, {
				"status_file", cf_str, &ig, (char **) &o_status_file, 512 }, {
				"command_file", cf_str, &ig, (char **) &o_command_file, 512 },
		{ "muf_file", cf_str, &ig, (char **) &o_muf_file, 512 }, { "help_file",
				cf_str, &ig, (char **) &o_help_file, 512 }, { "help_index",
				cf_str, &ig, (char **) &o_help_index, 512 }, { "news_file",
				cf_str, &ig, (char **) &o_news_file, 512 }, { "news_index",
				cf_str, &ig, (char **) &o_news_index, 512 }, { "man_file",
				cf_str, &ig, (char **) &o_man_file, 512 }, { "man_index",
				cf_str, &ig, (char **) &o_man_index, 512 }, { "welcome_file",
				cf_str, &ig, (char **) &o_welcome_file, 512 }, { "leave_file",
				cf_str, &ig, (char **) &o_leave_file, 512 }, { "leave_file",
				cf_str, &ig, (char **) &o_leave_file, 512 }, {
				"editor_help_file", cf_str, &ig, (char **) &o_editor_help_file,
				512 }, { "register_file", cf_str, &ig,
				(char **) &o_register_file, 512 }, { "register_msg", cf_str,
				&ig, (char **) &o_register_msg, 512 }, { "lockout_file",
				cf_str, &ig, (char **) &o_lockout_file, 512 }, { "lockout_msg",
				cf_str, &ig, (char **) &o_lockout_msg, 512 }, { "taboo_file",
				cf_str, &ig, (char **) &o_taboo_file, 512 }, { "wiz_msg",
				cf_str, &ig, (char **) &o_wiz_msg, 512 }, { "s_money", cf_str,
				&ig, (char **) &o_s_money, 512 }, { "pl_money", cf_str, &ig,
				(char **) &o_pl_money, 512 }, { "cs_money", cf_str, &ig,
				(char **) &o_cs_money, 512 }, { "cpl_money", cf_str, &ig,
				(char **) &o_cpl_money, 512 }, { "macro_file", cf_str, &ig,
				(char **) &o_macro_file, 512 },

		{ "starting_money", cf_int, &o_starting_money, (char **) &cg,
				MAX_PENNIES }, { "max_object_endowment", cf_int,
				&o_max_object_endowment, (char **) &cg, MAX_PENNIES }, {
				"max_object_deposit", cf_int, &o_max_object_deposit,
				(char **) &cg, MAX_PENNIES }, { "object_cost", cf_int,
				&o_object_cost, (char **) &cg, MAX_PENNIES }, { "exit_cost",
				cf_int, &o_exit_cost, (char **) &cg, MAX_PENNIES },
		{ "link_cost", cf_int, &o_link_cost, (char **) &cg, MAX_PENNIES },
		{ "room_cost", cf_int, &o_room_cost, (char **) &cg, MAX_PENNIES }, {
				"lookup_cost", cf_int, &o_lookup_cost, (char **) &cg,
				MAX_PENNIES }, { "penny_rate", cf_int, &o_penny_rate,
				(char **) &cg, MAX_PENNIES }, { "kill_base_cost", cf_int,
				&o_kill_base_cost, (char **) &cg, MAX_PENNIES }, {
				"kill_min_cost", cf_int, &o_kill_min_cost, (char **) &cg,
				MAX_PENNIES }, { "kill_bonus", cf_int, &o_kill_bonus,
				(char **) &cg, MAX_PENNIES }, { "dump_interval", cf_int,
				&o_dump_interval, (char **) &cg, 100000 }, { "rwho_interval",
				cf_int, &o_rwho_interval, (char **) &cg, 32000 }, {
				"rwho_port", cf_int, &o_rwho_port, (char **) &cg, 99999 }, {
				"command_time_msec", cf_int, &o_command_time_msec,
				(char **) &cg, 2000 }, { "command_burst_size", cf_int,
				&o_command_burst_size, (char **) &cg, 900 }, {
				"commands_per_time", cf_int, &o_commands_per_time,
				(char **) &cg, 100 },
		/* Put this back
		 {"max_output", cf_int, &o_max_output, (char **) &cg, 131072},
		 */
		{ "max_output", cf_int, &o_max_output, (char **) &cg, 1024000 }, {
				"max_frames_user", cf_int, &o_max_frames_user, (char **) &cg,
				100 }, { "max_frames_wizard", cf_int, &o_max_frames_wizard,
				(char **) &cg, 500 }, { "player_start", cf_int,
				&o_player_start, (char **) &cg, 500000 }, {
				"global_environment", cf_int, &o_global_environment,
				(char **) &cg, 500000 }, { "master_room", cf_int,
				&o_master_room, (char **) &cg, 500000 }, { "max_mush_args",
				cf_int, &o_max_mush_args, (char **) &cg, 200 },
		{ "max_mush_queue", cf_int, &o_max_mush_queue, (char **) &cg, 500 }, {
				"queue_quota", cf_int, &o_queue_quota, (char **) &cg, 100 }, {
				"queue_cost", cf_int, &o_queue_cost, (char **) &cg, 100 }, {
				"max_parents", cf_int, &o_max_parents, (char **) &cg, 100 },
		{ "max_nest_level", cf_int, &o_max_nest_level, (char **) &cg, 100 }, {
				"player_user_functions", cf_bool, &o_player_user_functions,
				(char **) &cg, 2 }, { "log_commands", cf_bool, &o_log_commands,
				(char **) &cg, 2 }, { "registration", cf_bool, &o_registration,
				(char **) &cg, 2 }, { "lockouts", cf_bool, &o_lockouts,
				(char **) &cg, 2 }, { "taboonames", cf_bool, &o_taboonames,
				(char **) &cg, 2 }, { "fast_exits", cf_bool, &o_fast_exits,
				(char **) &cg, 2 }, { "wiz_recycle", cf_bool, &o_wiz_recycle,
				(char **) &cg, 2 }, { "player_names", cf_bool, &o_player_names,
				(char **) &cg, 2 }, { "liberal_dark", cf_bool, &o_liberal_dark,
				(char **) &cg, 2 }, { "killframes", cf_bool, &o_killframes,
				(char **) &cg, 2 }, { "log_huhs", cf_bool, &o_log_huhs,
				(char **) &cg, 2 }, { "copyobj", cf_bool, &o_copyobj,
				(char **) &cg, 2 }, { "mufconnects", cf_bool, &o_mufconnects,
				(char **) &cg, 2 }, { "muffail", cf_bool, &o_muffail,
				(char **) &cg, 2 }, { "notify_wiz", cf_bool, &o_notify_wiz,
				(char **) &cg, 2 }, { "rwho", cf_bool, &o_rwho, (char **) &cg,
				2 }, { NULL, NULL, NULL, 0 } };

void do_parameter_set(__DO_PROTO) {
	CONF *cp = NULL;

	if (!arg1 || !*arg1) {
		notify(player, player, "No parameter specified.");
		return;
	}

	if (!arg2 || !*arg2) {
		if (!strcasecmp("list", arg1))
			config_list(player);
		else if (!strcasecmp("init", arg1) && Wizard(player)) {
			config_file_startup(CONFIG_FILE);
			notify(player, player, "Parameters re-initialized.");
		} else
			notify(player, player, "No parameter value specified.");
		return;
	}

	if (!God(player)) {
		notify(player, player, "Permission denied.");
		return;
	}

	for (cp = conftable; cp->name; cp++) {
		if (!strcmp(cp->name, arg1)) {
			if (cp->handler(arg1, arg2, cp->loc, cp->str, cp->max, 1, player)) {
				log_status("CONFIG: parameter %s changed by %s(#%ld) to %s\n",
						arg1, unparse_name(player), player, arg2);
				notify(player, player, "Parameter %s changed to %s.", arg1,
						arg2);
			}
			return;
		}
	}
	notify(player, player, "Parameter '%s' not found.", arg1);
	log_status("CONFIG: failed parameter change changed by %s(#%ld)\n",
			unparse_name(player), player);
}

/* enter boolean parameter */
int cf_bool(char *opt, char *val, int *loc, char **str, int maxval, int p,
		dbref player) {
	if (!strcasecmp(val, "yes") || !strcasecmp(val, "true") || !strcasecmp(val,
			"1")) {
		*loc = 1;
		return 1;
	}

	if (!strcasecmp(val, "no") || !strcasecmp(val, "false") || !strcasecmp(val,
			"0")) {
		*loc = 0;
		return 1;
	}

	if (p)
		notify(player, player, "Option %s value %s is invalid.", opt, val);
	else {
		fprintf(stderr, "CONFIGURATION: option %s value %s invalid.\n", opt,
				val);
		fflush(stderr);
	}
	return 0;
}

/* enter string parameter */
int cf_str(char *opt, char *val, int *loc, char **str, int maxlen, int p,
		dbref player) {
	/* truncate if necessary */
	if (strlen(val) >= maxlen) {
		if (p)
			notify(player, player, "Option %s value %s is too long.", opt, val);
		else {
			fprintf(stderr, "CONFIGURATION: option %s value %s too long\n",
					opt, val);
			fflush(stderr);
		}
		return 0;
	}

	if (*str)
		free(*str);
	*str = dup_string(val);
	return 1;
}

/* enter integer parameter */
int cf_int(char *opt, char *val, int *loc, char **str, int maxval, int p,
		dbref player) {
	int n = atoi(val);
	/* enforce limits */
	if (n > maxval) {
		n = maxval;
		if (p)
			notify(player, player, "Option %s max value is limited to %d.",
					opt, maxval);
		else {
			fprintf(stderr, "CONFIGURATION: option %s value limited to %d\n",
					opt, maxval);
			fflush(stderr);
		}
	}
	*loc = n;
	return 1;
}

/* search conf table for option; if found, add it, if not found, complain  */
void config_set(char *opt, char *val) {
	CONF *cp = NULL;

	for (cp = conftable; cp->name; cp++) {
		if (!strcmp(cp->name, opt)) {
			cp->handler(opt, val, cp->loc, cp->str, cp->max, 0, NOTHING);
			return;
		}
	}
	fprintf(stderr, "CONFIGURATION: directive '%s' not found.\n", opt);
	fflush(stderr);
}

void conf_default_set() {
	config_set("s_money", "penny");
	config_set("pl_money", "pennies");
	config_set("cs_money", "Penny");
	config_set("cpl_money", "Pennies");
	config_set("welcome_file", "data/welcome.text");
	config_set("leave_file", "data/leave.text");
	config_set("register_file", "data/reg.text");
	config_set("register_msg", "data/reg.msg");
	config_set("lockout_file", "data/lockout.text");
	config_set("lockout_msg", "data/lockout.msg");
	config_set("taboo_file", "data/tabooname.text");
	config_set("wiz_msg", "data/wizonly.text");
	config_set("editor_help_file", "data/edit-help.text");
	config_set("help_file", "data/help.text");
	config_set("help_index", "data/help.indx");
	config_set("news_file", "data/news.text");
	config_set("news_index", "data/news.indx");
	config_set("man_file", "data/man.text");
	config_set("man_index", "data/man.indx");
	config_set("gripe_file", "logs/gripes");
	config_set("status_file", "logs/status");
	config_set("command_file", "logs/commands");
	config_set("muf_file", "logs/muf-erros");
	config_set("macro_file", "muf/macros");
	config_set("starting_money", "50");
	config_set("max_object_endowment", "100");
	config_set("max_object_deposit", "504");
	config_set("object_cost", "10");
	config_set("exit_cost", "1");
	config_set("link_cost", "1");
	config_set("room_cost", "10");
	config_set("lookup_cost", "1");
	config_set("penny_rate", "1");
	config_set("kill_base_cost", "100");
	config_set("kill_min_cost", "10");
	config_set("kill_bonus", "50");
	config_set("dump_interval", "1800");
	config_set("rwho_interval", "180");
	config_set("rwho_port", "6889");
	config_set("command_time_msec", "1000");
	config_set("command_burst_size", "100");
	config_set("commands_per_time", "10");
	config_set("max_output", "42768");
	config_set("max_frames_user", "20");
	config_set("max_frames_wizard", "50");
	config_set("player_start", "2");
	config_set("master_room", "0");
	config_set("max_mush_args", "100");
	config_set("max_mush_queue", "10");
	config_set("queue_quota", "50");
	config_set("queue_cost", "1");
	config_set("max_parents", "10");
	config_set("max_nest_level", "20");
	config_set("player_user_functions", "0");
	config_set("log_commands", "0");
	config_set("registration", "0");
	config_set("lockouts", "0");
	config_set("taboonames", "0");
	config_set("fast_exits", "0");
	config_set("wiz_recycle", "0");
	config_set("player_names", "0");
	config_set("liberal_dark", "0");
	config_set("killframes", "1");
	config_set("log_huhs", "0");
	config_set("copyobj", "1");
	config_set("mufconnects", "1");
	config_set("muffail", "1");
	config_set("notify_wiz", "1");
}

int config_file_startup(char *conf) { /* read a configuration file. Return 0 on failure, 1 on success */
	FILE *fp = NULL;
	char tbuf1[BUFFER_LEN];
	char *p = NULL, *q = NULL, *s = NULL;

	fp = fopen(conf, "r");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: Cannot open configuration file %s.\n", conf);
		return 0;
	}

	conf_default_set(); /* initialize defaults */

	fgets(tbuf1, BUFFER_LEN, fp);
	while (!feof(fp)) {
		p = tbuf1;
		if (*p == '#') {
			/* comment line */
			fgets(tbuf1, BUFFER_LEN, fp);
			continue;
		}
		/* this is a real line. Strip the newline and characters following it.
		 * Split the line into command and argument portions. If it exists,
		 * also strip off the trailing comment.
		 */
		for (p = tbuf1; *p && (*p != '\n'); p++)
			;
		*p = '\0'; /* strip '\n' */
		for (p = tbuf1; *p && isspace(*p); p++)
			/* strip spaces */
			;
		for (q = p; *q && !isspace(*q); q++)
			/* move over command */
			;
		if (*q)
			*q++ = '\0'; /* split off command */
		for (; *q && isspace(*q); q++)
			/* skip spaces */
			;
		for (s = q; *s && (*s != '#'); s++)
			/* look for comment */
			;
		if (*s) /* if found nuke it */
			*s = '\0';
		for (s = s - 1; (s >= q) && isspace(*s); s--) /* smash trailing stuff */
			*s = '\0';

		if (strlen(p) != 0) /* skip blank lines */
			config_set(p, q);

		fgets(tbuf1, BUFFER_LEN, fp);
	}

	fclose(fp);
	return 1;
}

void config_list(dbref player) {
	CONF *cp = NULL;

	notify(player, player, "PARAMETER            VALUE");
	notify(player, player,
			"---------------------------------------------------");
	for (cp = conftable; cp->name; cp++) {
		if (cp->handler == cf_str)
			conf_notify(player, cp->name, cp->loc, cp->str, 1);
		else
			conf_notify(player, cp->name, cp->loc, cp->str, 0);
	}
}

void conf_notify(dbref player, char *name, int *loc, char **str, int flag) {
	if (flag)
		notify(player, player, "%-20.20s | %s", name, *str);
	else
		notify(player, player, "%-20.20s | %d", name, *loc);
}

char *o_gripe_file;
char *o_status_file;
char *o_command_file;
char *o_macro_file;
char *o_muf_file;
char *o_help_file;
char *o_help_index;
char *o_news_file;
char *o_news_index;
char *o_man_file;
char *o_man_index;
char *o_welcome_file;
char *o_leave_file;
char *o_editor_help_file;
char *o_register_file;
char *o_register_msg;
char *o_lockout_file;
char *o_lockout_msg;
char *o_taboo_file;
char *o_s_money;
char *o_pl_money;
char *o_cs_money;
char *o_cpl_money;
char *o_wiz_msg;
int o_port;
int o_starting_money;
int o_max_object_endowment;
int o_max_object_deposit;
int o_object_cost;
int o_exit_cost;
int o_link_cost;
int o_room_cost;
int o_lookup_cost;
int o_penny_rate;
int o_kill_base_cost;
int o_kill_min_cost;
int o_kill_bonus;
int o_dump_interval;
int o_rwho_interval;
int o_rwho_port;
int o_command_time_msec;
int o_command_burst_size;
int o_commands_per_time;
int o_max_output;
int o_max_frames_user;
int o_max_frames_wizard;
int o_player_start;
int o_global_environment;
int o_master_room;
int o_max_mush_args;
int o_max_mush_queue;
int o_queue_quota;
int o_queue_cost;
int o_max_parents;
int o_max_nest_level;
int o_player_user_functions;
int o_log_commands;
int o_registration;
int o_lockouts;
int o_taboonames;
int o_fast_exits;
int o_wiz_recycle;
int o_player_names;
int o_liberal_dark;
int o_killframes;
int o_log_huhs;
int o_copyobj;
int o_mufconnects;
int o_muffail;
int o_notify_wiz;
int o_rwho;
