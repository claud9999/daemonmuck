#include "prims.h"

void prims_addressp  (__P_PROTO);
void prims_numberp   (__P_PROTO);
void prims_playerp   (__P_PROTO);
void prims_thingp    (__P_PROTO);
void prims_roomp     (__P_PROTO);
void prims_programp  (__P_PROTO);
void prims_exitp     (__P_PROTO);
void prims_okp       (__P_PROTO);
void prims_flagp     (__P_PROTO);
void prims_set       (__P_PROTO);
void prims_stringp   (__P_PROTO);
void prims_dbrefp    (__P_PROTO);
void prims_intp      (__P_PROTO);
void prims_varp      (__P_PROTO);
void prims_getflags  (__P_PROTO);
void prims_locked    (__P_PROTO);
void prims_passlockp (__P_PROTO);
void prims_okplayer  (__P_PROTO);
void prims_controls  (__P_PROTO);
void prims_abort     (__P_PROTO);
void prims_checkargs (__P_PROTO);
void prims_password (__P_PROTO);
void prims_floatp (__P_PROTO);

#define PRIMS_TESTS_FL prims_addressp, prims_numberp, prims_playerp, \
        prims_thingp, prims_roomp, prims_programp, \
        prims_exitp, prims_okp, prims_flagp, prims_set, \
        prims_stringp, prims_dbrefp, prims_intp, prims_varp, prims_getflags, \
        prims_locked, prims_passlockp, prims_okplayer, prims_controls, \
        prims_abort, prims_checkargs, prims_password, prims_floatp

#define PRIMS_TESTS_TL "ADDRESS?", "NUMBER?", "PLAYER?", "THING?", "ROOM?", \
        "PROGRAM?", "EXIT?", "OK?", "FLAG?", "SET", \
        "STRING?", "DBREF?", "INT?", "VAR?", "GETFLAGS", "LOCKED?", \
        "PASSLOCK?", "OKPLAYER?", "CONTROLS?", "ABORT", "CHECKARGS", \
        "PASSWORD?", "FLOAT?"

#define PRIMS_TESTS_LEN 23
