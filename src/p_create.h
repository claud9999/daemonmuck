#include "prims.h"

void prims_copyobj (__P_PROTO);
void prims_recycle (__P_PROTO);
void prims_create  (__P_PROTO);
void prims_dig     (__P_PROTO);
void prims_open    (__P_PROTO);
void prims_unlink  (__P_PROTO);
void prims_addlink (__P_PROTO);

#ifndef COPYOBJ
#define PRIMS_CREATE_TL "CREATE", "DIG", "OPEN", "UNLINK", "ADDLINK", "RECYCLE"
#define PRIMS_CREATE_FL prims_create, prims_dig, prims_open, prims_unlink, \
        prims_addlink, prims_recycle
#define PRIMS_CREATE_LEN 6
#else /* COPYOBJ */
#define PRIMS_CREATE_TL "CREATE", "DIG", "OPEN", "UNLINK", "ADDLINK", \
        "COPYOBJ", "RECYCLE"
#define PRIMS_CREATE_FL prims_create, prims_dig, prims_open, prims_unlink, \
        prims_addlink, prims_copyobj, prims_recycle
#define PRIMS_CREATE_LEN 7
#endif /* COPYOBJ */
