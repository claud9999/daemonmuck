#ifndef DB_H
#define DB_H

#include "copyright.h"
#include "config.h"
#include <stdio.h>

#define BUFFER_LEN 10240

extern char match_args[BUFFER_LEN];

typedef long dbref; /* offset into db */

#define __DO_PROTO dbref player, char *arg1, char *arg2, char *argall

#define DBFETCH(x)      (db + (x))
#define DBDIRTY(x)      /* nothing */

#define DBSTORE(x, y, z)    {DBFETCH(x)->y = z; DBDIRTY(x);}
#define NAME(x)         (db[x].name)
#define FLAGS(x)        (db[x].flags)
#define OWNER(x)        (db[x].owner)

#define GET_NAME(x)	(DBFETCH(x)->name)
#define GET_DESC(x)	(DBFETCH(x)->desc)
#define GET_FAIL(x)	(DBFETCH(x)->fail)
#define GET_SUCC(x)	(DBFETCH(x)->succ)
#define GET_DROP(x)	(DBFETCH(x)->drop)
#define GET_OFAIL(x)	(DBFETCH(x)->ofail)
#define GET_OSUCC(x)	(DBFETCH(x)->osucc)
#define GET_ODROP(x)	(DBFETCH(x)->odrop)

#define SET_NAME(x,y)	{(DBFETCH(x)->name)=(y); DBDIRTY(x);}
#define SET_FAIL(x,y)	{(DBFETCH(x)->fail)=(y); DBDIRTY(x);}
#define SET_SUCC(x,y)	{(DBFETCH(x)->succ)=(y); DBDIRTY(x);}
#define SET_DROP(x,y)	{(DBFETCH(x)->drop)=(y); DBDIRTY(x);}
#define SET_OFAIL(x,y)	{(DBFETCH(x)->ofail)=(y); DBDIRTY(x);}
#define SET_OSUCC(x,y)	{(DBFETCH(x)->osucc)=(y); DBDIRTY(x);}
#define SET_ODROP(x,y)	{(DBFETCH(x)->odrop)=(y); DBDIRTY(x);}

#define TYPE_ROOM       0x0
#define TYPE_THING      0x1
#define TYPE_EXIT       0x2
#define TYPE_PLAYER     0x3
#define TYPE_PROGRAM    0x4     /* related structures */
#define TYPE_DAEMON     0x5     /* but not the same thing */
#define TYPE_GARBAGE    0x6
#define NOTYPE          0x7     /* no particular type */
#define TYPE_MASK       0x7     /* room for expansion */
#define ANTILOCK        0x8     /* negates key (*OBSOLETE*) */
#define WIZARD          0x10    /* gets automatic control */
#define LINK_OK         0x20    /* anybody can link to this room */
#define DARK            0x40    /* contents of room are not printed */
#define INTERNAL        0x80    /* internal-use-only flag */
#define STICKY          0x100   /* this object goes home when dropped */
#define BUILDER         0x200   /* this player can useruction commands */
#define CHOWN_OK        0x400   /* this player can be @chowned to */
#define JUMP_OK         0x800   /* A room/player can be Jumped from/or */
#define ENTER_OK        0x1000  /* object/player/program/exit is entered */
#define NOSPOOF         0x2000  /* No spoofing zone here bub */
#define INHERIT         0x4000  /* Objects Inherit owners privs */
#define OPAQUE          0x8000  /* Can't see inside of thing */
#define HAVEN           0x10000    /* can't kill here */
#define ABODE           0x20000    /* can set home here */
#define MUCKER          0x40000    /* programmer / monitor for WIZARD */
#define UNFIND          0x80000    /* Unfindable */
#define PUPPET          0x100000   /* PUPPET */
#define INTERACTIVE     0x200000   /* player either editing or in a READ.  */
#define AUDIBLE         0x400000   /* AUDIBLE */
#define NOCOMMAND       0x800000   /* NOCOMMAND */
#define GOD             0x1000000  /* GOD */
#define QUELL           0x2000000  /* Wizard but not wizard */
#define VISUAL          0x4000000  /* Non owners can look/examine */
#define VERBOSE         0x8000000  /* Notify owner if in parse_command */
#define SAFE            0x10000000 /* Can't @rec it */

#define REACH_FLAG      0x40000000 /*reserved for extract.c*/
#define GENDER_MASK     0x3000  /* 2 bits of gender */
#define GENDER_SHIFT    12      /* 0x1000 is 12 bits over (for shifting) */
#define GENDER_UNASSIGNED       0x0     /* unassigned - the default */
#define GENDER_NEUTER   0x1     /* neuter */
#define GENDER_FEMALE   0x2     /* for women */
#define GENDER_MALE     0x3     /* for men */

#define ANY_TYPE        0x0|0x1|0x2|0x3|0x4
#define ALLBUTROOM      0x1|0x2|0x3|0x4
#define ALLBUTTHING     0x0|0x2|0x3|0x4
#define ALLBUTEXIT      0x0|0x1|0x3|0x4
#define ALLBUTPLAYER    0x0|0x1|0x2|0x4
#define ALLBUTPROGRAM   0x0|0x1|0x2|0x3
#define THINGANDPROGRAM 0x1|0x4

typedef int object_flag_type; /* 0xffffffff */
extern int epoch;

#define GOD_DBREF ((dbref) 1)
#define God(x) (((FLAGS(x) & GOD) != 0) || ((x) == (GOD_DBREF)))

#define DoNull(s) ((s) ? (s) : "")
#define Typeof(x) (((x) == HOME) ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))
#define Wizard(x) (((FLAGS(x) & WIZARD) != 0) && ((FLAGS(x) & QUELL) == 0))
#define TrueWizard(x) ((FLAGS(x) & WIZARD) != 0)
#define Sticky(x) ((FLAGS(x) & STICKY) != 0)
#define Mucker(x) ((FLAGS(x) & (WIZARD|MUCKER)) != 0)
#define Builder(x) ((FLAGS(x) & (WIZARD|BUILDER)) != 0)
#define Visual(x)  ((FLAGS(x) & VISUAL) != 0)
#define Verbose(x)  ((FLAGS(x) & VERBOSE) != 0)
#define Audible(x)  ((FLAGS(x) & AUDIBLE) != 0)
#define Unfindable(x) ((FLAGS(x) & UNFIND) != 0)
#define Findable(x)  ((FLAGS(x) & UNFIND) == 0)
#define Nocommand(x)  ((FLAGS(x) & NOCOMMAND) != 0)
#define Puppet(x)  ((FLAGS(x) & PUPPET) != 0)
#define Dark(x) ((FLAGS(x) & DARK) != 0 && !Puppet(x))
#define Safe(x)  ((FLAGS(x) & SAFE) != 0)
#define Haven(x)  ((FLAGS(x) & HAVEN) != 0)
#define LinkOk(x)  ((FLAGS(x) & LINK_OK) != 0)
#define Chown_Ok(x)  ((FLAGS(x) & CHOWN_OK) != 0)
#define Quiet(x)  (((FLAGS(x) & QUELL) != 0) && (Typeof(x) != TYPE_PLAYER))
#define Halted(x)  (((FLAGS(x) & HAVEN) != 0) && (Typeof(x) == TYPE_THING))

#define Linkable(x) ((x) == HOME || ((Typeof(x) == TYPE_ROOM ? \
                                     (FLAGS(x) & ABODE) : \
                                      (FLAGS(x) & LINK_OK)) != 0))

typedef struct propdir {
	char *name; /* name of property */
	char *data; /* data of property */
	struct propdir *next; /* next item on property list */
	struct propdir *child; /* child properties */
	char perms; /* permissions */
} propdir;

#define PERMS_LOCKED  0x80  /* property locked by a wizard? */
#define PERMS_HIDDEN  0x40  /* property hidden? */
#define PERMS_COREAD  0x20  /* property readable by controller? */
#define PERMS_COWRITE 0x10  /* property writable by controller? */
#define PERMS_COSRCH  0x08  /* property searchable by controller? */
#define PERMS_OTREAD  0x04  /* property readable by others? */
#define PERMS_OTWRITE 0x02  /* property writable by others? */
#define PERMS_OTSRCH  0x01  /* property searchable by others? */

#define ACCESS_WI     0x02  /* access is as wizard */
#define ACCESS_CO     0x01  /* access is as controller */
#define ACCESS_OT     0x00  /* access is as other */

#define PT_SEE     0x04     /* See a propdir     */
#define PT_CHANGE  0x03     /* Change a propdir  */
#define PT_READ    0x02     /* READ a propdir    */
#define PT_WRITE   0x01     /* WRITE a propdir   */
#define PT_SRCH    0x00     /* SEARCH a propdir  */

/* property lists are used thus:

 @set me=property:class

 As of right now, the value field is not used, but it is there for later
 expansion.

 SEX is no longer going to be a flag, it's going to be a property, so that
 locks can lock against the sex of the character.                         */

/* Boolean expressions, for locks */
typedef char boolexp_type;

#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_PROP 4

typedef struct boolexp {
	boolexp_type type;
	struct boolexp *sub1;
	struct boolexp *sub2;
	dbref thing;
	char *prop_name;
	char *prop_data;
} boolexp;

#define TRUE_BOOLEXP ((boolexp *) NULL)

/* special dbref's */
#define NOTHING ((dbref) -1)            /* null dbref */
#define AMBIGUOUS ((dbref) -2)        /* multiple possibilities, for matchers */
#define HOME ((dbref) -3)            /* virtual room, represents mover's home */

/* FLAG structure */
#define FLAG_HASH_SIZE 256
#define FLAG_HASH_MASK 255

typedef struct flag_info FLAG;
struct flag_info {
	char *name;
	char letter;
	object_flag_type type;
	object_flag_type flag;
	FLAG *next;
};

typedef struct flag_entry FLAGENT;
struct flag_entry {
	char *name;
	FLAG *entry;
	FLAGENT *next;
};

/* Line data structure */
typedef struct line {
	char *this_line; /* the line itself */
	struct line *next, *prev; /* the next line and the previous line */
} line;

/* stack and object declarations */
#define PROG_PRIMITIVE   1     /* forth primitives and hard-coded C routines */
#define PROG_STRING      2         /* string types */
#define PROG_INTEGER     3         /* integer types */
#define PROG_ADD         4    /* program address --- used in calls and jumps */
#define PROG_OBJECT      5         /* database objects */
#define PROG_VAR         6         /* variables */
#define PROG_FLOAT       7         /* floating types */

#define MAX_VAR         53         /* maximum number of variables including
                                      the basic ME and LOC                */
#define RES_VAR          3         /* no of reserved variables */

#define STACK_SIZE       512       /* maximum size of stack */

#define ILIMIT_DEFAULT   500000    /* maximum iterations of interp_loop */

#define QUANTUM		 3000

typedef struct inst /* instruction */
{
	char type;
	short linenum;
	union {
		char *string;
		long number; /* used for both primitives and integers */
		dbref objref; /* object reference */
		double fnum;
		struct inst *call; /* use in IF and JMPs */
	} data;
} inst;

typedef struct stack {
	int top;
	inst st[STACK_SIZE];
} stack;

typedef struct dbref_list {
	dbref object;
	struct dbref_list *next;
} dbref_list;

typedef struct for_list /* used for loops */
{
	int current;
	int target;
	int stepsize;
	struct for_list *next;
} for_list;

typedef struct frame /* frame data structure necessary
 for executing programs */
{
	stack system, /* system stack */
	argument; /* argument stack */
	inst variables[MAX_VAR], /* variables */
	*pc; /* next executing instruction */
	char writeonly; /* This program should not do reads */
	int iterations; /* the number of iterations done */
	dbref_list *caller;
	dbref trigger, /* object which was used to start frame*/
	player, prog, euid; /* for permissions checking */
	for_list *for_loop;
	struct frame *next;
	char wizard, /* does program have wizard access? */
	status, /* frame status [see below for values] */
	endless; /* Should frame run to completion? */
	int pid; /* for unique identification */
	long sleeptime; /* time to sleep for */
	int waitpid;
} frame;

#define STATUS_RUN	0
#define STATUS_READ	1
#define STATUS_WAIT	2
#define STATUS_SLEEP	3
#define STATUS_DEAD	4
#define STATUS_X_READ	5

extern frame *running_frames;
extern frame *stopped_frames;

/* union of type-specific fields */

typedef union {
	struct /* EXIT-specific fields */
	{
		int ndest;
		dbref *dest;
	} exit;

	struct /* PLAYER-specific fields */
	{
		char insert_mode; /* in insert mode? */
		char *password;
		int pid; /* used for READs. */
	} player;

	struct /* PROGRAM-specific fields */
	{
		dbref_list *editlocks; /* who is editing this program? */
		line *first; /* first line */
		int siz, /* size of code */
		curr_line; /* current-line */
		inst *code, /* byte-compiled code */
		*start; /* place to start executing */
	} program;
} specific;

typedef struct object {
	char *name, *desc, *fail, /* what you see if op fails */
			*succ, /* what you see if op succeeds */
			*drop, /* what you see if this is dropped */
			*ofail, /* what others see if op fails */
			*osucc, /* what others see if op succeeds */
			*odrop /* what others see if this is dropped */
	;
	dbref location, /* pointer to container */
	owner, contents, exits, next, /* pointer to next in contents/exits/daemons
	 chain */
	nextowned, /* pointer to the next in the owned list */
	link, /* where is this object linked to? */
	curr_prog; /* program I'm currently editing */
	dbref_list *backlinks; /* linked list of objects linked to obj */
	dbref_list *backlocks; /* list of objects locked to/against obj */

	boolexp *key; /* if not NOTHING, must have this to do op */

	propdir *properties;
	object_flag_type flags;
	long pennies;
#ifdef TIMESTAMPS
	long time_created, time_modified, time_used;
#endif
	specific sp;
} object;

typedef struct macrotable {
	char *name;
	char *definition;
	dbref implementor;
	struct macrotable *left;
	struct macrotable *right;
} macrotable;

/* Possible data types that may be stored in a hash table */
union u_hash_data {
	int ival; /* Store compiler tokens here */
	dbref dbval; /* Player hashing will want this */
	void *pval; /* Can't hurt, we might need it someday */
};

/* The actual hash entry for each item */
typedef struct t_hash_entry {
	struct t_hash_entry *next; /* Pointer for conflict resolution */
	char *name; /* The name of the item */
	union u_hash_data dat; /* Data value for item */
} t_hash_entry;

typedef union u_hash_data hash_data;
typedef t_hash_entry hash_entry;
typedef hash_entry *hash_tab;

#define PLAYER_HASH_SIZE   (1 << 12) /* Table for player lookups */
#define COMP_HASH_SIZE     (1 << 6)  /* Table for compiler keywords */

#define COMPOST_NAME "<compost>"
#define COMPOST_DESC "<a pile of gunky garbage.>"

extern object *db;
extern macrotable *macrotop;
extern dbref db_top;

extern char *dup_string(char *);

extern dbref new_object(); /* return a new object */

extern dbref getref(FILE *); /* Read a database reference from a file. */

extern void putref(FILE *, dbref); /* Write one ref to the file */

extern boolexp *getboolexp(FILE *); /* get a boolexp */
extern void putboolexp(FILE *, struct boolexp *); /* put a boolexp */

extern int db_write_object(FILE *, dbref); /* write one object to file */
extern int db_write_object_lachesis(FILE *, dbref);
/* write one object to file */

extern dbref db_write(FILE *f); /* write db to file, return # of objects */
extern dbref db_write_lachesis(FILE *f); /* write db to file, return # of objects */

extern dbref db_read(FILE *f); /* read db from file, return # of objects */
/* Warning: destroys existing db contents! */

extern void free_boolexp(struct boolexp *);
extern void db_free(void);

extern dbref parse_dbref(char *); /* parse a dbref */

#define DOLIST(var, first) \
  for((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)
#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}
#define getloc(thing) (DBFETCH(thing)->location)

/*
 Usage guidelines:

 To obtain an object pointer use DBFETCH(i).  Pointers returned by DBFETCH
 may become invalid after a call to new_object().

 To update an object, use DBSTORE(i, f, v), where i is the object number,
 f is the field (after ->), and v is the new value.

 If you have updated an object without using DBSTORE, use DBDIRTY(i) before
 leaving the routine that did the update.

 When using PUSH, be sure to call DBDIRTY on the object which contains
 the locative (see PUSH definition above).

 Some fields are now handled in a unique way, since they are always memory
 resident, even in the GDBM_DATABASE disk-based muck.  These are: name,
 flags and owner.  Refer to these by NAME(i), FLAGS(i) and OWNER(i).
 Always call DBDIRTY(i) after updating one of these fields.

 The programmer is responsible for managing storage for string
 components of entries; db_read will produce malloc'd strings.  The
 alloc_string routine is provided for generating malloc'd strings
 duplicates of other strings.  Note that db_free and db_read will
 attempt to free any non-NULL string that exists in db when they are
 invoked.
 */

extern char *o_gripe_file;
extern char *o_status_file;
extern char *o_command_file;
extern char *o_macro_file;
extern char *o_muf_file;
extern char *o_help_file;
extern char *o_help_index;
extern char *o_news_file;
extern char *o_news_index;
extern char *o_man_file;
extern char *o_man_index;
extern char *o_welcome_file;
extern char *o_leave_file;
extern char *o_editor_help_file;
extern char *o_register_file;
extern char *o_register_msg;
extern char *o_lockout_file;
extern char *o_lockout_msg;
extern char *o_taboo_file;
extern char *o_s_money;
extern char *o_pl_money;
extern char *o_cs_money;
extern char *o_cpl_money;
extern char *o_wiz_msg;
extern int o_port;
extern int o_starting_money;
extern int o_max_object_endowment;
extern int o_max_object_deposit;
extern int o_object_cost;
extern int o_exit_cost;
extern int o_link_cost;
extern int o_room_cost;
extern int o_lookup_cost;
extern int o_penny_rate;
extern int o_kill_base_cost;
extern int o_kill_min_cost;
extern int o_kill_bonus;
extern int o_dump_interval;
extern int o_rwho_interval;
extern int o_rwho_port;
extern int o_command_time_msec;
extern int o_command_burst_size;
extern int o_commands_per_time;
extern int o_max_output;
extern int o_max_frames_user;
extern int o_max_frames_wizard;
extern int o_player_start;
extern int o_global_environment;
extern int o_master_room;
extern int o_max_mush_args;
extern int o_max_mush_queue;
extern int o_queue_quota;
extern int o_queue_cost;
extern int o_max_parents;
extern int o_max_nest_level;
extern int o_player_user_functions;
extern int o_log_commands;
extern int o_registration;
extern int o_lockouts;
extern int o_taboonames;
extern int o_fast_exits;
extern int o_wiz_recycle;
extern int o_player_names;
extern int o_liberal_dark;
extern int o_killframes;
extern int o_log_huhs;
extern int o_copyobj;
extern int o_mufconnects;
extern int o_muffail;
extern int o_notify_wiz;
extern int o_rwho;
extern int o_db_sync;

#define DBFETCHPROP(x) (DBFETCH((x))->properties)
#define DBSTOREPROP(x,y) (DBSTORE((x), properties, (y)))

#endif /* DB_H */
