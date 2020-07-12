#include "prims.h"

void prims_getpropval  (__P_PROTO);
void prims_getpropstr  (__P_PROTO);
void prims_remove_prop (__P_PROTO);
void prims_addprop     (__P_PROTO);
void prims_name        (__P_PROTO);
void prims_fullname    (__P_PROTO);
void prims_desc        (__P_PROTO);
void prims_succ        (__P_PROTO);
void prims_fail        (__P_PROTO);
void prims_drop        (__P_PROTO);
void prims_osucc       (__P_PROTO);
void prims_ofail       (__P_PROTO);
void prims_odrop       (__P_PROTO);
void prims_setname     (__P_PROTO);
void prims_setdesc     (__P_PROTO);
void prims_setsucc     (__P_PROTO);
void prims_setfail     (__P_PROTO);
void prims_setdrop     (__P_PROTO);
void prims_setosucc    (__P_PROTO);
void prims_setofail    (__P_PROTO);
void prims_setodrop    (__P_PROTO);
void prims_pennies     (__P_PROTO);
void prims_addpennies  (__P_PROTO);
void prims_setprop     (__P_PROTO);
void prims_perms       (__P_PROTO);
void prims_setperms    (__P_PROTO);
void prims_nextprop    (__P_PROTO);
void prims_nextpropp   (__P_PROTO);
void prims_is_propdir  (__P_PROTO);

#define PRIMS_PROPERTY_FL prims_getpropval, prims_getpropstr, \
        prims_remove_prop, prims_addprop, prims_name, prims_fullname, \
	prims_desc, prims_succ, prims_fail, prims_drop, prims_osucc, \
	prims_ofail, prims_odrop, prims_setname, prims_setdesc, \
	prims_setsucc, prims_setfail, prims_setdrop, prims_setosucc, \
	prims_setofail, prims_setodrop, prims_pennies, prims_addpennies, \
	prims_setprop, prims_perms, prims_setperms, prims_nextprop, \
	prims_nextpropp, prims_is_propdir

#define PRIMS_PROPERTY_TL "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP", \
	"ADDPROP", "NAME", "FULLNAME", "DESC", "SUCC", "FAIL", "DROP", \
	"OSUCC", "OFAIL", "ODROP", "SETNAME", "SETDESC", "SETSUCC", \
	"SETFAIL", "SETDROP", "SETOSUCC", "SETOFAIL", "SETODROP", \
	"PENNIES", "ADDPENNIES", "SETPROP", "PERMS", "SETPERMS", \
	"NEXTPROP", "NEXTPROP?", "PROPDIR?"

#define PRIMS_PROPERTY_LEN 29
