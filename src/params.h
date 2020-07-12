#ifndef PARAMS_H
#define PARAMS_H

#include "copyright.h"
#include "config.h"
/* STOP!!!!!!!!   Do NOT modify ANY parameter that has options!!! */
/* Go edit the muck.conf file instead.  */


/* amount of object endowment, based on cost */
#define MAX_OBJECT_ENDOWMENT (o_max_object_endowment)
#define MAX_OBJECT_DEPOSIT (o_max_object_deposit)
#define OBJECT_ENDOWMENT(cost) (((cost)-5)/5)
#define OBJECT_DEPOSIT(endow) ((endow)*5+4)

/* minimum costs for various things */
#define MAX_PENNIES 10000    /* amount at which temple gets cheap */

/* WARRNING!  Do NOT make any values below LARGER then MAX_PENNIES */
#define OBJECT_COST (o_object_cost)  /* Amount to make an object    */
#define EXIT_COST (o_exit_cost)  /* Amount it costs to make an exit */
#define LINK_COST (o_link_cost)  /* Amount it costs to make a link  */
#define ROOM_COST (o_room_cost)  /* Amount it costs to dig a room   */
#define LOOKUP_COST (o_lookup_cost)  /* cost to do a scan  */
#define PENNY_RATE (o_penny_rate)  /* Chance of getting penny per room */

/* costs of kill command */
#define KILL_BASE_COST (o_kill_base_cost) /* prob = expenditure/KILL  */
#define KILL_MIN_COST (o_kill_min_cost)  /* min needed to kill        */
#define KILL_BONUS (o_kill_bonus)       /* "insurance" paid to victim */


/* timing stuff */
#define TIME_MINUTE(x)  (60 * (x))              /* 60 seconds */
#define TIME_HOUR(x)    (60 * (TIME_MINUTE(x))) /* 60 minutes */
#define TIME_DAY(x)     (24 * (TIME_HOUR(x)))   /* 24 hours   */
#define DO_PERCENT(what) ((what*100.0)/total), what

#define DUMP_INTERVAL (o_dump_interval) /* seconds between dumps */
#define RWHO_INTERVAL (o_rwho_interval) /* seconds between RWHO updates */

/* time slice length in milliseconds    */
#define COMMAND_TIME_MSEC (o_command_time_msec)  

/* commands allowed per user in a burst */
#define COMMAND_BURST_SIZE (o_command_burst_size) 

/* commands per time slice after burst  */
#define COMMANDS_PER_TIME (o_commands_per_time)    


/* maximum amount of queued output */
#define MAX_OUTPUT (o_max_output)

#define DB_INITIAL_SIZE 100

#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"
#define PREFIX_COMMAND "OUTPUTPREFIX"
#define SUFFIX_COMMAND "OUTPUTSUFFIX"

/* Turn this back on when you want MUD to set from root to some user_id */
/* #define MUD_ID "MUCK" */

/* Change to 'home' if you don't want it to be too confusing to novices. */

#define BREAK_COMMAND "@Q"

/* @edit'or stuff */

#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
#define COMPILE_COMMAND 'c'
#define LIST_COMMAND   'l'
#define EDITOR_HELP_COMMAND 'h'
#define KILL_COMMAND 'k'
#define SHOW_COMMAND 's'
#define SHORTSHOW_COMMAND 'a'
#define VIEW_COMMAND 'v'
#define UNASSEMBLE_COMMAND 'u'
#define NUMBER_COMMAND 'n'

/* maximum number of arguments */
#define MAX_ARG  2

/* max frames for users, to prevent endless loops */
#define MAX_FRAMES_USER (o_max_frames_user)
#define MAX_FRAMES_WIZARD (o_max_frames_wizard)
/*  Minimum sleep time for failed password checking attempts */
#define PWSLEEP 3

/* Usage comments:
   Line numbers start from 1, so when an argument variable is equal to 0, it
   means that it is non existent.

   I've chosen to put the parameters before the command, because this should
   more or less make the players get used to the idea of forth coding..     */

#define EXIT_INSERT "."         /* character to exit from the editor    */
#define EXIT_DELIMITER ';'      /* delimiter for lists of exit aliases  */

#define MAX_LINKS 50    /* maximum number of destinations for an exit */

/* room number of player start location */
#define PLAYER_START ((dbref) (o_player_start))

/* parent of all rooms */
#define GLOBAL_ENVIRONMENT ((dbref) (o_global_environment))  

/* magic cookies (not chocolate chip) :) */

#define NOT_TOKEN '!'
#define AND_TOKEN '&'
#define OR_TOKEN '|'
#define LOOKUP_TOKEN '*'
#define NUMBER_TOKEN '#'
#define ARG_DELIMITER '='
#define PROP_DELIMITER ':'
#define PROP_RDONLY '_'
#define PROP_PRIVATE '.'
#define PROP_MUF '*'

/* magic command cookies (oh me, oh my!) */

#define SAY_TOKEN '"'
#define SAY_TOKEN_TWO '\''
#define POSE_TOKEN ':'
#define POSE_TOKEN_TWO ';'
#define REGISTERED_TOKEN '$'

#define RWHO_SERVER  ".rwho_server"
#define RWHO_PASS    ".rwho_pass"
#define RWHO_NAME    ".rwho_name"
#define RWHO_COMMENT ".rwho_comment"

#define RWHO_PORT (o_rwho_port)

/* Location of files used by tinymuck */
#define WELC_FILE  (o_welcome_file)  /* For the opening screen */
#define LEAVE_FILE (o_leave_file)    /* For the closing screen */
#define HELP_FILE (o_help_file)     /* For the 'help' command */
#define NEWS_FILE (o_news_file)     /* For the 'news' command */
#define MAN_FILE  (o_man_file)      /* For the 'man' command */
#define EDITOR_HELP_FILE (o_editor_help_file) /* editor help file */

/* index files are automatically generated whenever the text files change */
#define HELP_INDEX  (o_help_index)
#define NEWS_INDEX  (o_news_index)
#define MAN_INDEX   (o_man_index)
#define MUF_DIR     "muf/text/"

#define LOG_GRIPE   (o_gripe_file)  /* Gripes Log */
#define LOG_STATUS  (o_status_file) /* System errors and stats */
#define LOG_MUF     (o_muf_file)   /* Muf compiler errors and warnings. */
#define COMMAND_LOG (o_command_file) /* Player commands */
#define MACRO_FILE  (o_macro_file)   /*  Where the MUF macros live */
#define REG_FILE (o_register_file) /* Host list that MUST Register */
#define REG_MSG  (o_register_msg)  /* Text file on failed creates  */
#define WIZ_TEXT  (o_wiz_msg)
#define LOCKOUT_FILE (o_lockout_file)    /* The banned host list */
#define LOCKOUT_TEXT (o_lockout_msg)   /* File sent to banned hosts */
#define TABOO_FILE (o_taboo_file)  /* Illegal substring name list */

/* define this to the default pennies to start a player out with */
#define DEFAULT_PENNIES (o_starting_money)

#endif /* PARAMS_H */
