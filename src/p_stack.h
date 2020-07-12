#include "prims.h"

void prims_pop    (__P_PROTO);
void prims_dup    (__P_PROTO);
void prims_swap   (__P_PROTO);
void prims_over   (__P_PROTO);
void prims_pick   (__P_PROTO);
void prims_put    (__P_PROTO);
void prims_rot    (__P_PROTO);
void prims_roll   (__P_PROTO);
void prims_rotate (__P_PROTO);
void prims_depth  (__P_PROTO);
void prims_pstack (__P_PROTO);

#define PRIMS_STACK_FL prims_pop, prims_dup, prims_swap, prims_over, \
        prims_pick, prims_put, prims_rot, prims_rotate, prims_depth, \
        prims_pstack, prims_roll

#define PRIMS_STACK_TL "POP", "DUP", "SWAP", "OVER", "PICK", "PUT", \
        "ROT", "ROTATE", "DEPTH", "PSTACK", "ROLL"

#define PRIMS_STACK_LEN 11
