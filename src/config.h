#ifndef CONFIG_H
#define CONFIG_H

#include "autoconf.h"

/* Tunable parameters -- Edit to you heart's content */
/* Don't forget to edit the muck.conf file!          */

/* Default values for program */

/* Use to compress string data */
#define COMPRESS

/*
 * Use this only if your realloc does not allocate in powers of 2
 * (if your realloc is clever, this option will cause you to waste space).
 */
#define DB_DOUBLING

/* Use gethostbyaddr() for hostnames in logs and the wizard WHO list.  */
#define HOSTNAMES

/*
 * Makes God (Player #1) immune to @force, @newpassword, and
 * being set !Wizard.
*/
#define GOD_PRIV

/* Prevent Many Fine Cores.  */
#undef NOCOREDUMP

/* Allow players to use @chown, this also adds the
 * CHOWN_OK flag to set an object to be @chownable
 */
#define PLAYER_CHOWN

/* define this to be the flags you want players to be created with...  */
/* #define DEFAULT_PFLAGS BUILDER | MUCKER */ /* i.e. for builder & mucker-all */
#undef DEFAULT_PFLAGS /* don't want ANY default flags */

/* Lets examine show integer properties on objects */
#undef SHOWINTPROP

/* Allow a player to use teleport-to-player
 * destinations for exits
 */
#define TELEPORT_TO_PLAYER

/*  Don't fork on dumps */
#undef NOFORK

/* To use vfork() instead of fork() */
#ifndef vfork
#undef USE_VFORK
#endif

/* To use internal MALLOC routines instead of system versions */
/* NeXT users should #define this */
#undef USE_MALLOC

/* the 's' flag on a player makes look and examine no longer show dbrefs on
 * objects they own.  */
#define SILENT_PLAYERS

/* timestamps on objects */
#define TIMESTAMPS

/* define this if you want the server to checkpoint the backlinks, backlocks,*/
/* and ownerlists every time it loads the db in.*/
#define VALIDATE_LISTS

/* resets lists each time the db is read in. */
#undef RESET_LISTS

/* limit on player name length */
#define PLAYER_NAME_LIMIT 64

/* Various messages used if text files can't be read */
#define DEFAULT_LEAVE_MESSAGE "\r\nGoodbye!\r\n"

#define DEFAULT_WELCOME_MESSAGE "Welcome to AfterFive.\r\nTo connect to your existing character, enter \"connect name password\"\r\nTo create a new character, enter \"create name password\"\r\n"

#define DEFAULT_REG_MSG "Sorry, but registration is in force for your site.\r\nPlease contact a wizard to create a character.\r\n"

#undef PLAYER_LISTEN         /* Let users set @listen on themselves? */

/* Use system strftime()  #undef this is you get undefined _strftime */
#define USE_STRFTIME

#define TINYPORT 9999   /* Port that tinymuck uses for playing */

/* Default name of the config file to be loaded at run time. */
/* Go edit this file!  */
#define CONFIG_FILE "muck.conf"

#endif /* CONFIG_H */
