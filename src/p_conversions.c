#include "prims.h"
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
extern double p_float;
extern char p_buf[BUFFER_LEN];
extern dbref p_ref;
static int p_nargs;

void prims_atoi (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  if (p_oper1->type != PROG_STRING || !p_oper1->data.string) p_result = 0;
  else p_result = atol(p_oper1->data.string);

  CLEAR(p_oper1);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_atof (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  if (p_oper1->type != PROG_STRING || !p_oper1->data.string) p_float = 0;
  else
      p_float = atof(p_oper1->data.string);

  CLEAR(p_oper1);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_str (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  switch(p_oper1->type)
  {
    case PROG_STRING:  snprintf (p_buf, BUFFER_LEN, "%s", p_oper1->data.string); break;
    case PROG_VAR:
    case PROG_INTEGER: snprintf (p_buf, BUFFER_LEN, "%ld", p_oper1->data.number); break;
    case PROG_FLOAT:   snprintf(p_buf, BUFFER_LEN, "%G", p_oper1->data.fnum); break;
    case PROG_OBJECT:  snprintf (p_buf, BUFFER_LEN, "%ld", p_oper1->data.objref); break;
    default:           abort_interp("Invalid argument type.");
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_STRING, MIPSCAST dup_string(p_buf));
}

void prims_dbref (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  switch(p_oper1->type)
  {
    case PROG_STRING:  p_ref = p_oper1->data.string ?
      (dbref)strtol(p_oper1->data.string, NULL, 0) : (dbref)0;
      break;
    case PROG_VAR:
    case PROG_INTEGER: p_ref = (dbref)p_oper1->data.number; break;
    case PROG_OBJECT:  p_ref = (dbref)p_oper1->data.objref; break;
    default:           abort_interp("Invalid argument type.");
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_int (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  switch (p_oper1->type)
  {
    case PROG_OBJECT:  p_result = p_oper1->data.objref; break;
    case PROG_VAR:     p_result = p_oper1->data.number; break;
    case PROG_INTEGER: p_result = p_oper1->data.number; break;
    case PROG_FLOAT:   p_result = p_oper1->data.fnum; break;
    case PROG_STRING:  p_result = p_oper1->data.string ?
      strtol(p_oper1->data.string, NULL, 0) : 0;
      break;
    default:           abort_interp("Invalid argument type.");
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_variable (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  switch (p_oper1->type)
  {
    case PROG_OBJECT:  p_result = p_oper1->data.objref; break;
    case PROG_VAR:     p_result = p_oper1->data.number; break;
    case PROG_INTEGER: p_result = p_oper1->data.number; break;
    case PROG_FLOAT:   p_result = p_oper1->data.fnum; break;
    case PROG_STRING:  p_result = (p_oper1->data.string) ?
      strtol(p_oper1->data.string, NULL, 0) : 0;
      break;
    default:           abort_interp("Invalid argument type.");
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_VAR, MIPSCAST &p_result);
}

void prims_ilimit (__P_PROTO)
{
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_INTEGER, MIPSCAST &ilimit);
}

void prims_setilimit (__P_PROTO)
{
  if (!fr->wizard) abort_interp("Permission denied.");
  CHECKOP(1);
  p_oper1 = POP();

  if (p_oper1->type != PROG_INTEGER) abort_interp("Operand not an integer.");
  if (p_oper1->data.number < 5) abort_interp("Operand is too small.");

  ilimit = p_oper1->data.number;
}

void prims_float (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  switch (p_oper1->type)
  {
    case PROG_OBJECT:  p_float = p_oper1->data.objref; break;
    case PROG_VAR:     p_float = p_oper1->data.number; break;
    case PROG_INTEGER: p_float = p_oper1->data.number; break;
    case PROG_FLOAT:   p_float = p_oper1->data.fnum; break;
    case PROG_STRING:  p_float = p_oper1->data.string ?
      atof(p_oper1->data.string) : 0;
      break;
    default:           abort_interp("Invalid argument type.");
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}
