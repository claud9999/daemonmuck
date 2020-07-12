#include "prims.h"
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
static int conditional;
extern int p_nargs;

void prims_for_add (__P_PROTO)
{
  for_list *loop;

  CHECKOP(3);
  p_oper1 = POP();
  p_oper2 = POP();
  p_oper3 = POP();

  if (p_oper1->type != PROG_INTEGER) abort_interp("Invalid step size.");
  if (p_oper2->type != PROG_INTEGER) abort_interp("Invalid end (for).");
  if (p_oper3->type != PROG_INTEGER) abort_interp("Invalid start (for).");

  loop = (for_list *) malloc(sizeof(for_list));
  loop->next = fr->for_loop;
  loop->current = p_oper3->data.number;
  loop->target = p_oper2->data.number;
  loop->stepsize = p_oper1->data.number;

  fr->for_loop = loop;

  p_result = p_oper3->data.number;

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  CLEAR(p_oper3);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}


void prims_for_check (__P_PROTO)
{
  for_list *loop;

  loop = fr->for_loop;

  if (!loop) abort_interp ("Invalid check of FOR loop.");

  loop->current += loop->stepsize;

  if (loop->stepsize == 0) conditional = 0;
  else if (loop->stepsize>0) conditional = (loop->current <= loop->target)?1:0;
  else conditional = (loop->current >= loop->target)?1:0;

  p_result = loop->current;

  if (conditional == 0)
  {
    fr->for_loop = loop->next;
    free (loop);
  }
  else
  {
    if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
    push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
  }

  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_INTEGER, MIPSCAST &conditional);
}

void prims_for_pop (__P_PROTO)
{
  for_list *loop;

  CHECKOP(1);
  p_oper1 = POP();

  if (p_oper1->type != PROG_INTEGER)
    abort_interp("Internal FOR error: Invalid argument to clear registers.");

  for (p_result = 0; p_result < p_oper1->data.number; p_result++)
  {

    loop = fr->for_loop;

    if (!loop) abort_interp("Internal FOR error: FOR register underflow.");

    fr->for_loop = loop->next;
    free (loop);
  }
  CLEAR(p_oper1);
}
