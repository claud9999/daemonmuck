#ifndef INTERFACE_H
#define INTERFACE_H

#include "copyright.h"
#include "db.h"

/* these symbols must be defined by the interface */
int notify_nolisten(dbref player, char *msg, ...);
int awakep(dbref player);
extern int shutdown_flag; /* Should interface should shut down */
extern int wiz_only_flag; /* Should Non-Wizard logins should not be allowed */
extern int mush_interp;   /* Don't allow objects to go INTERACTIVE */
void emergency_shutdown(void);
int boot_off(dbref player);

/* the following symbols are provided by game.c */

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 2048
void process_command(dbref player, char *command, dbref cause);

dbref connect_player(char *name, char *password);
dbref create_player(char *name, char *password, dbref doer);
void do_look_around(dbref player);

int init_game(char *infile, char *outfile);
void dump_database(void);

typedef struct text_block
{
  int nchars;
  struct text_block *nxt;
  char *start,
    *buf;
} text_block;

typedef struct text_queue
{
  text_block *head,
    **tail;
} text_queue;

typedef struct descriptor_data
{
  int descriptor,
    output_size,
    quota;
  dbref player;
  char connected,
    *output_prefix,
    *output_suffix,
    *raw_input,
    *raw_input_at,
    *hostname;
  text_queue output,
    input;
  long last_time,
    connected_at;
  struct descriptor_data *next,
    **prev;
} descriptor_data;

extern descriptor_data *descriptor_list;
extern void panic(char *);
extern void dump_rusers(descriptor_data *d);
extern char match_cmdname[BUFFER_LEN];

#endif /* INTERFACE_H */
