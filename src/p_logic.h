#include "prims.h"

void prims_and (__P_PROTO);
void prims_or  (__P_PROTO); 
void prims_not (__P_PROTO);

#define PRIMS_LOGIC_FL prims_and, prims_or, prims_not
#define PRIMS_LOGIC_TL "AND", "OR", "NOT"
#define PRIMS_LOGIC_LEN 3
