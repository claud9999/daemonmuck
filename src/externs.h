#ifndef EXTERNS_H
#define EXTERNS_H

#include <time.h>
#include "interface.h"

#include "copyright.h"

#include "db.h"

/* Prototypes for externs not defined elsewhere */

extern char match_args[BUFFER_LEN];

/* from db.c */
void db_chown(dbref, dbref);
void add_compost(dbref);
void add_ownerlist(dbref);
void remove_ownerlist(dbref);
int number(char *s);
void free_line(line *l);
void db_free_object(dbref i);
void db_clear_object(dbref i);
void macrodump(macrotable *node, FILE *f);
void macroload();
int check_password(char *plaintext, dbref player);
char *make_password(char *plaintext);
void add_backlocks_parse(dbref, boolexp *);
void remove_backlocks_parse(dbref, boolexp *);
void add_backlinks(dbref);
void remove_backlinks(dbref);
int db_freeze_object(dbref);
object *dbfetch(dbref);
void dbdirty(dbref);
void dbsync(void);
void fix_props_on_garbage();
void reset_lists();
void write_program(line *first, dbref i);
void free_prog_text(line *l);

/* From create.c */
void do_open(__DO_PROTO);
void do_link(__DO_PROTO);
void do_dig(__DO_PROTO);
void do_create(__DO_PROTO);
void do_program(__DO_PROTO);
void do_mlist(__DO_PROTO);
void do_edit(__DO_PROTO);
void do_action(__DO_PROTO);
void do_attach(__DO_PROTO);
line *read_program(dbref i);
int exit_loop_check(dbref source, dbref dest);

/* from edit.c */
macrotable *new_macro(char *name, char *definition, dbref player);
void match_and_list(dbref player, char *name, char *linespec);
int grow_macro_tree(macrotable *node, macrotable *newmacro);

/* From game.c */
void do_dump(__DO_PROTO);
void do_shutdown(__DO_PROTO);
void time_keeper();

/* From hashtab.c */
hash_data *find_hash(register char *s, hash_tab *table, unsigned size);
hash_entry
		*add_hash(char *name, hash_data data, hash_tab *table, unsigned size);
int free_hash(char *name, hash_tab *table, unsigned size);
void kill_hash(hash_tab * table, unsigned size, int freeptrs);

/* From help.c */
void spit_file(dbref player, char *filename);
void do_outputinfo(dbref player, char *, char *, char *, char *);
void do_help(__DO_PROTO);
void do_news(__DO_PROTO);
void do_man(__DO_PROTO);

/* From look.c */
void look_room(dbref player, dbref room);
void do_look_around(dbref player);
void do_look_at(__DO_PROTO);
void do_properties(__DO_PROTO);
void do_contents(__DO_PROTO);
void do_exits(__DO_PROTO);
void do_examine(__DO_PROTO);
void do_at_examine(__DO_PROTO);
void do_inventory(__DO_PROTO);
void do_find(__DO_PROTO);
void do_owned(__DO_PROTO);
void do_score(__DO_PROTO);
void do_trace(__DO_PROTO);
void do_version(__DO_PROTO);
void exec_or_notify(dbref player, dbref thing, char *message);
char *flag_description(dbref thing);

/* From move.c */
void moveto(dbref what, dbref where);
void enter_room(dbref what, dbref where, dbref exit1);
void send_home(dbref thing);
int parent_loop_check(dbref source, dbref dest);
int can_move(dbref player, char *direction);
void do_earthquake(__DO_PROTO);
void do_move(__DO_PROTO);
void do_get(__DO_PROTO);
void do_drop(__DO_PROTO);
void do_recycle(__DO_PROTO);
void recycle(dbref player, dbref thing);
void send_contents(dbref loc, dbref dest);

/* From player.c */
dbref lookup_player(char *name);
void do_password(__DO_PROTO);
void add_player(dbref who);
void delete_player(dbref who);
void do_player_setuid(__DO_PROTO);
int awake_count(dbref player);
void clear_players();

/* From predicates.c */
int can_link_to(dbref who, object_flag_type what_type, dbref where);
int can_link(dbref who, dbref what);
int could_doit(dbref player, dbref thing);
int can_doit(dbref player, dbref thing, char *default_fail_msg);
int can_see(dbref player, dbref thing, int can_see_location);
int controls(dbref who, dbref what);
int restricted(dbref player, dbref thing, object_flag_type flag);
int payfor(dbref who, int cost);
int ok_player_name(char *name, dbref player);
int ok_password(char *password);
int ok_name(char *name);

/* From rob.c */
void do_kill(__DO_PROTO);
void do_give(__DO_PROTO);
void do_rob(__DO_PROTO);

/* From set.c */
dbref match_controlled(dbref player, char *name);
int ok_taboo_name(dbref player, char *name, int flag);
void do_name(__DO_PROTO);
void do_describe(__DO_PROTO);
void do_fail(__DO_PROTO);
void do_success(__DO_PROTO);
void do_drop_message(__DO_PROTO);
void do_osuccess(__DO_PROTO);
void do_ofail(__DO_PROTO);
void do_odrop(__DO_PROTO);
void do_lock(__DO_PROTO);
void do_unlock(__DO_PROTO);
void do_unlink(__DO_PROTO);
void do_chown(__DO_PROTO);
void do_set(__DO_PROTO);
void do_perms(__DO_PROTO);

/* From speech.c */
void do_wall(__DO_PROTO);
void do_gripe(__DO_PROTO);
void do_say(__DO_PROTO);
void do_page(__DO_PROTO);
void do_pose(__DO_PROTO);
void do_whisper(__DO_PROTO);
int notify(dbref, dbref, char *, ...);
void notify_except(dbref, dbref, dbref, char *, ...);
int notify_listener(dbref, dbref, dbref, char *, ...);
void notify_wizards(char *, ...);
void listener_sweep(dbref, dbref, dbref, char *, ...);
void notify_except_nolisten(dbref player, dbref location, dbref exception,
		char *msg, ...);
void vnotify_except_nolisten(dbref player, dbref location, dbref exception,
		char *msg, va_list args);

/* From stringutil.c */
int stringn_compare(char *s1, char *s2, int n);
int string_compare(char *, char *);
int string_prefix(char *, char *);
char *string_match(char *, char *);
char *pronoun_substitute(dbref, char *);
char *dup_string(char *);
#define strdup dup_string
void lowerstring(char *);
void upperstring(char *);
char *splitstring(char *, char);
char *muf_write_reformat(char *msg);
char *dup_it(char *string);
int safe_copy_str(char *c, char *buff, char **bp, int maxlen);

/* From utils.c */
int member(dbref, dbref);
dbref remove_first(dbref, dbref);

/* From wiz.c */
void do_cache(__DO_PROTO);
void do_teleport(__DO_PROTO);
void do_force(__DO_PROTO);
void do_stats(__DO_PROTO);
void init_stat_time(void);
void do_toad(__DO_PROTO);
void do_trimdb(__DO_PROTO);
void do_boot(__DO_PROTO);
void do_pcreate(__DO_PROTO);
void do_newpassword(__DO_PROTO);
void do_backlinks(__DO_PROTO);
void do_backlocks(__DO_PROTO);
void do_editlocks(__DO_PROTO);
void do_login(__DO_PROTO);
void do_reset(__DO_PROTO);
void do_db_sync(__DO_PROTO);
void do_db_flush(__DO_PROTO);
void toad(dbref player, dbref recipient);

/* From boolexp.c */
int eval_boolexp(dbref, boolexp *, dbref);
boolexp *parse_boolexp(dbref, char *);
boolexp *copy_bool(boolexp *);
void bool_dgsr(boolexp *, dbref, dbref);
void sanitize_lock(boolexp *lock);

/* From unparse.c */
char *unparse_name(dbref);
char *unparse_flags(dbref);
char *unparse_owner(dbref);
char *unparse_object_do(dbref, dbref, int);
char *unparse_boolexp(dbref, boolexp *);
#define unparse_object(A,B) unparse_object_do(A,B,1)
#define unparse_object_full(A,B) unparse_object_do(A,B,0)

/* from property.c */
char parse_perms(char *);
char *unparse_perms(char); /* do not free value returned */
char access_rights(dbref, dbref, dbref);
int add_property(dbref, char *, char *, char, char);
void burn_proptree(propdir *);
int remove_property(dbref, char *, char);
int has_property(dbref, char *, char *, char);
propdir *find_property(dbref, char *, char);
int validate_property(dbref, char *, char *, char);
char *get_property_data(dbref, char *, char);
void copy_prop(dbref, dbref, char);
int genderof(dbref, char);
void notify_propdir(dbref, dbref, char *, char, char);
void change_perms(dbref, char *, char, char);
void next_property(char *, dbref, char *, char);
int has_next_property(dbref, char *, char, int);
void pdq_burn();
int is_propdir(dbref player, dbref object1, char *string, dbref program);
void freeze_properties(char *, dbref);
propdir *thaw_properties(char *, char);
char default_perms(char *line1);
void putproperties(FILE *f, dbref obj);
void getproperties(FILE *f, dbref obj, char permflag);

/* From compress.c */
#ifdef COMPRESS
char *compress(const char *s);
char *uncompress(const char *s);
#endif /* COMPRESS */

/* From edit.c */
void interactive(dbref player, char *command);
char *macro_expansion(macrotable *node, char *match);
void do_list(dbref player, dbref program, int arg[], int argc);
int erase_node(macrotable *oldnode, macrotable *node, char *killname);
int grow_macro_tree(macrotable *node, macrotable *newmacro);

/* From compile.c */
void do_uncompile(__DO_PROTO);
void do_compile(dbref in_player, dbref in_program);
void clear_primitives(void);
void init_primitives(void);
char *envpropstr(dbref *where, char *propname);
void free_prog(inst *c, int siz);

/* From interp.c */
void do_ps(__DO_PROTO);
void do_go(__DO_PROTO);
void do_pidkill(__DO_PROTO);
void prog_clean(frame *);
void bump_frames(char *, dbref, dbref);
frame *new_frame(dbref, dbref, dbref, dbref, char);
void add_frame(frame *);
void autostart_frames();
void run_frames();
int run_frame(frame *, int);
void free_frame(frame *);
frame *find_frame(int);
int find_a_frame(int frid);
extern long ilimit;
void kill_on_disconnect(descriptor_data *d);
int float_type(inst *op1, inst *op2);

/* from dbreflist.c */
void dbreflist_dump(FILE *, dbref_list *);
dbref_list *dbreflist_add(dbref_list *, dbref);
dbref_list *dbreflist_shuffle(dbref_list *, dbref);
dbref_list *dbreflist_remove(dbref_list *, dbref);
dbref dbreflist_last(dbref_list *);
void dbreflist_burn(dbref_list *);
dbref_list *dbreflist_read(FILE *);
int dbreflist_find(dbref_list *, dbref);
void freeze_dbreflist(char *, dbref_list *);
dbref_list *thaw_dbreflist(char *);

/* from wild.c */
extern char *wptr[10];
int wild_match(char *s, char *d);

/* from eval.c */
void fval(char *buff, double result);
void init_func_hashtab();
char *exec(dbref cause, char *str, dbref player, int eflags);
char *replace_string(char *old, char *new, char *string);
char *atr_get(dbref thing, char *name);
char *seek_char(char *s, char c);
char *skip_space(char *s);
char *upcasestr(char *s);
int do_real_open(dbref player, char *direction, char *linkto, dbref pseudo);
int do_mush_create(char *arg1, char *arg2, dbref player);
int do_mush_dig(char *arg1, char *arg2, dbref player);
int safe_chr(char c, char *buf, char **bufp);
int local_wild(char *s, char *d, int p, int os);
int local_wild_match(char *s, char *d);
int GoodObject(dbref object1);
int is_number(char *str);
int Connected(dbref target);

/* From smatch.c */
int equalstr(char *s, char *t);

/* From strftime.c */
int format_time(char *buf, int max_len, char *fmt, struct tm * tmval);

/* from interface.c */
extern int maxplayer;
void warn_users(char *msg);
int queue_string(descriptor_data *d, char *s, ...);
int vqueue_string(descriptor_data *d, char *s, va_list args);
int process_output(descriptor_data *d);
int process_input(descriptor_data *d);
void shutdownsock(descriptor_data *, int);
void notify_user(descriptor_data *d, char *text_file, char *default_text);
int queue_write(descriptor_data *, char *, int);
void close_sockets();
void make_nonblocking(int s);
dbref partial_pmatch(char *name);
int vnotify_nolisten(dbref player, char *msg, va_list args);

/* from flags.c */
FLAG *flag_lookup(char *name, dbref thing);
int strn_cmp(char *sub, char *string);
void do_list_flags(__DO_PROTO);

/* from config.c */
void do_parameter_set(__DO_PROTO);
void do_prop_load(__DO_PROTO);
int config_file_startup(char *conf);

/* from log.c */
void log_status(char *format, ...);
void log_muf(char *format, ...);
void log_gripe(char *format, ...);
void log_command(char *format, ...);

/* from disassem.c */
void disassemble(dbref player, dbref program);

#endif /* EXTERNS_H */
