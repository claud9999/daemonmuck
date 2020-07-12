#ifndef PRIMS_H
#define PRIMS_H

#define __P_PROTO dbref player, dbref program, inst *pc, inst *arg, int *top, \
                   frame *fr

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"

#ifdef MIPS
typedef char *voidptr;
#define MIPSCAST (char *)
#else
typedef void *voidptr;
#define MIPSCAST
#endif
extern char *uppercase, *lowercase;
#define UPCASE(x) (uppercase[x])
#define DOWNCASE(x) (lowercase[x])
#ifdef COMPRESS
#define alloc_compressed(x) dup_string(compress(x))
#define get_compress(x) compress(x)
#define get_uncompress(x) uncompress(x)
#else /* COMPRESS */
#define alloc_compressed(x) dup_string(x)
#define get_compress(x) (x)
#define get_uncompress(x) (x)
#endif /* COMPRESS */
#define DoNullInd(x) ((x) ? (x) : "")

#define CLEAR(C) \
{ \
  if ((C)->type == PROG_STRING) free((void *) (C)->data.string); \
}

void push (inst *, int *, int, voidptr);
int valid_object(inst *);
int false (inst *);
void copyinst(inst *, inst *);
void push (inst *, int *, int, voidptr);
int valid_player(inst *);
void copyobj(dbref, dbref, dbref);
int valid_object(inst *);
int is_home(inst *);
int permissions(dbref, dbref);
int arith_type(inst *, inst *);
void interp_err(dbref, char *, char *, dbref);

#define CHECKOP(N) \
{ \
  if ((*top) < (N)) \
  { \
    interp_err(player, insttoerr(pc), "Stack underflow.", program); \
    return; \
  } \
  p_nargs = (N); \
}

#define POP() (arg + --(*top))

#define abort_interp(C) \
{ \
  interp_err(player, insttoerr(pc), (C), program); \
  switch(p_nargs) \
  { \
    case 4: CLEAR(p_oper4); \
    case 3: CLEAR(p_oper3); \
    case 2: CLEAR(p_oper2); \
    case 1: CLEAR(p_oper1); \
  } \
  return; \
}
#endif /* PRIMS_H */
