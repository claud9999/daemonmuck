#include "prims.h"

void prims_for_add   (__P_PROTO);
void prims_for_check (__P_PROTO);
void prims_for_pop   (__P_PROTO);

#define PRIMS_FOR_FL prims_for_add, prims_for_check, prims_for_pop
#define PRIMS_FOR_TL "FOR_ADD ", "FOR_CHECK ", "FOR_POP "
#define PRIMS_FOR_LEN 3
