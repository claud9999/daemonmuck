#include "prims.h"
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
extern int p_nargs;

void prims_and (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

  p_result = !false(p_oper1) && !false(p_oper2);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_or (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

  p_result = !false(p_oper1) || !false(p_oper2);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_not (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  p_result = false(p_oper1);

  CLEAR(p_oper1);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}
