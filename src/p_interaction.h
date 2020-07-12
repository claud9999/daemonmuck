#include "prims.h"

void prims_notify                 (__P_PROTO);
void prims_notify_nolisten        (__P_PROTO);
void prims_notify_except          (__P_PROTO);
void prims_notify_except_nolisten (__P_PROTO);
void prims_force                  (__P_PROTO);
void prims_processes              (__P_PROTO);
void prims_pidkill                (__P_PROTO);
void prims_go                     (__P_PROTO);
void prims_sleeptime              (__P_PROTO);
void prims_piddbref               (__P_PROTO);
void prims_pid                    (__P_PROTO);
void prims_isapid                 (__P_PROTO);
void prims_foreground             (__P_PROTO);
void prims_compile                (__P_PROTO);
void prims_uncompile              (__P_PROTO);
void prims_newprogram             (__P_PROTO);
void prims_prog_size              (__P_PROTO);
void prims_new_macro              (__P_PROTO);
void prims_kill_macro             (__P_PROTO);
void prims_get_macro              (__P_PROTO);
void prims_delete                 (__P_PROTO);
void prims_insert                 (__P_PROTO);
void prims_getlines               (__P_PROTO);
void prims_prog_lines             (__P_PROTO);

#define PRIMS_INTERACTION_FL prims_notify, prims_notify_except, prims_force, \
        prims_processes, prims_pidkill, prims_go, prims_sleeptime, \
        prims_piddbref, prims_pid, prims_isapid, prims_notify_nolisten, \
        prims_notify_except_nolisten, prims_compile, prims_uncompile, \
        prims_newprogram, prims_prog_size, prims_new_macro, prims_kill_macro, \
        prims_get_macro, prims_delete, prims_insert, prims_getlines, \
        prims_prog_lines, prims_foreground

#define PRIMS_INTERACTION_TL "NOTIFY", "NOTIFY_EXCEPT", "FORCE", "PROCESSES",\
        "KILL", "GO", "SLEEPTIME", "PIDDBREF", "PID", "ISAPID?", \
        "NOTIFY_NOLISTEN", "NOTIFY_EXCEPT_NOLISTEN", "COMPILE", "UNCOMPILE", \
        "NEWPROGRAM", "PROG_SIZE", "NEW_MACRO", "KILL_MACRO", "GET_MACRO", \
        "DELETE", "INSERT", "GETLINES", "PROG_LINES", "FOREGROUND"

#define PRIMS_INTERACTION_LEN 24
