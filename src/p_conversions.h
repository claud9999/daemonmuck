#include "prims.h"

void prims_atoi     (__P_PROTO);
void prims_str      (__P_PROTO);
void prims_dbref    (__P_PROTO);
void prims_int      (__P_PROTO);
void prims_variable (__P_PROTO);
void prims_ilimit   (__P_PROTO);
void prims_setilimit(__P_PROTO);
void prims_atof     (__P_PROTO);
void prims_float     (__P_PROTO);

#define PRIMS_CONVERSIONS_FL prims_atoi, prims_str, prims_dbref, \
  prims_int, prims_variable, prims_ilimit, prims_setilimit, prims_atof, \
  prims_float

#define PRIMS_CONVERSIONS_TL "ATOI", "INTOSTR", "DBREF", "INT", "VARIABLE", \
  "ILIMIT", "SETILIMIT", "ATOF", "FLOAT"

#define PRIMS_CONVERSIONS_LEN 9
