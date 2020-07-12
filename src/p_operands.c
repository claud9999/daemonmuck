#include "prims.h"
#include "math.h"

/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
static inst temp1;
extern int p_result;
extern double p_float;
extern int p_nargs;

void prims_add (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

     if (arith_type(p_oper2, p_oper1)) {
        p_result = p_oper1->data.number + p_oper2->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
    else if (float_type(p_oper2, p_oper1)) {
        p_float = p_oper1->data.fnum + p_oper2->data.fnum;
        push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
       }
     else
         abort_interp("Invalid argument types. (2)");
}

void prims_subtract (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

     if (arith_type(p_oper2, p_oper1)) {
        p_result = p_oper2->data.number - p_oper1->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
    else if (float_type(p_oper2, p_oper1)) {
        p_float = p_oper2->data.fnum - p_oper1->data.fnum;
        push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
       }
     else
         abort_interp("Invalid argument types.");
}

void prims_multiply (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

     if (arith_type(p_oper2, p_oper1)) {
        p_result = p_oper1->data.number * p_oper2->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
    else if (float_type(p_oper2, p_oper1)) {
        p_float = p_oper1->data.fnum * p_oper2->data.fnum;
        push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
       }
     else
         abort_interp("Invalid argument types.");
}

void prims_divide (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

     if (arith_type(p_oper2, p_oper1)) {
        if (p_oper1->data.number) p_result = p_oper2->data.number / 
            p_oper1->data.number;
        else p_result = 0;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
    else if (float_type(p_oper2, p_oper1)) {
        if (p_oper1->data.fnum) p_float = p_oper2->data.fnum / 
            p_oper1->data.fnum;
        else p_float = 0;
        push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
       }
     else
         abort_interp("Invalid argument types.");
}

void prims_mod (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

  if (!arith_type(p_oper2, p_oper1)) abort_interp("Invalid argument type.");
   {
     if (p_oper1->data.number) p_result = p_oper2->data.number % 
         p_oper1->data.number;
     else p_result = 0;
   }

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, p_oper2->type, MIPSCAST &p_result);
}

void prims_lessthan (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP(); 
  p_oper2 = POP();

  if (p_oper1->type == PROG_INTEGER && p_oper2->type == PROG_INTEGER) {
        p_result = p_oper2->data.number < p_oper1->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else if (p_oper1->type == PROG_FLOAT && p_oper2->type == PROG_FLOAT) {
        p_result = p_oper2->data.fnum < p_oper1->data.fnum;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else
      abort_interp("Invalid argument type.");
}

void prims_greathan (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP(); 
  p_oper2 = POP();

  if (p_oper1->type == PROG_INTEGER && p_oper2->type == PROG_INTEGER) {
        p_result = p_oper2->data.number > p_oper1->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else if (p_oper1->type == PROG_FLOAT && p_oper2->type == PROG_FLOAT) {
        p_result = p_oper2->data.fnum > p_oper1->data.fnum;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else
      abort_interp("Invalid argument type.");
}

void prims_equal (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP(); 
  p_oper2 = POP();

  if (p_oper1->type == PROG_INTEGER && p_oper2->type == PROG_INTEGER) {
        p_result = p_oper2->data.number == p_oper1->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else if (p_oper1->type == PROG_FLOAT && p_oper2->type == PROG_FLOAT) {
        p_result = p_oper2->data.fnum == p_oper1->data.fnum;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else
      abort_interp("Invalid argument type.");
}

void prims_lesseq (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP(); 
  p_oper2 = POP();

  if (p_oper1->type == PROG_INTEGER && p_oper2->type == PROG_INTEGER) {
        p_result = p_oper2->data.number <= p_oper1->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else if (p_oper1->type == PROG_FLOAT && p_oper2->type == PROG_FLOAT) {
        p_result = p_oper2->data.fnum <= p_oper1->data.fnum;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else
      abort_interp("Invalid argument type.");
}

void prims_greateq (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP(); 
  p_oper2 = POP();

  if (p_oper1->type == PROG_INTEGER && p_oper2->type == PROG_INTEGER) {
        p_result = p_oper2->data.number >= p_oper1->data.number;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else if (p_oper1->type == PROG_FLOAT && p_oper2->type == PROG_FLOAT) {
        p_result = p_oper2->data.fnum >= p_oper1->data.fnum;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
       }
  else
      abort_interp("Invalid argument type.");
}

void prims_random (__P_PROTO)
{
  p_result = random();
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_frandom (__P_PROTO)
{
#ifdef HAVE_DRAND48 
  p_float = drand48();
#else
  abort_interp("Sorry, this computer doesn't support the drand48 function.");
#endif
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_dbcomp (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP(); p_oper2 = POP();
  if (p_oper1->type != PROG_OBJECT || p_oper2->type != PROG_OBJECT)
    abort_interp("Invalid argument type.");
  p_result = p_oper1->data.objref == p_oper2->data.objref;
  CLEAR(p_oper1); CLEAR(p_oper2);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_at (__P_PROTO)
{
  CHECKOP(1);
  temp1 = *(p_oper1 = POP());

  if (temp1.type != PROG_VAR || temp1.data.number >= MAX_VAR)
    abort_interp("Non-variable argument.");
  copyinst(&(fr -> variables[temp1.data.number]), &arg[(*top)++]);

  CLEAR(&temp1);
}

void prims_bang (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();

  if ((p_oper1->type != PROG_VAR) ||
    (p_oper1->data.number >= MAX_VAR) ||
    (p_oper1->data.number < 0))
    abort_interp("Non-variable argument (2)");

  CLEAR(&fr -> variables[p_oper1->data.number]);
  copyinst(p_oper2, &(fr -> variables[p_oper1->data.number]));
  CLEAR(p_oper1);
  CLEAR(p_oper2);
}

void prims_pplus (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  if ((p_oper1->type != PROG_VAR) ||
    (p_oper1->data.number >= MAX_VAR) ||
    (p_oper1->data.number < 0))
    abort_interp("Non-variable argument (1)");

  switch (fr -> variables[p_oper1->data.number].type)
  {
    case PROG_INTEGER:
      fr -> variables[p_oper1->data.number].data.number++;
      break;
    case PROG_OBJECT:
      fr -> variables[p_oper1->data.number].data.objref++;
      break;
    default:
       abort_interp("Variable contains non-integer");
       break;
  }

  CLEAR(p_oper1);
}

void prims_mminus (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();

  if ((p_oper1->type != PROG_VAR) ||
    (p_oper1->data.number >= MAX_VAR) ||
    (p_oper1->data.number < 0))
    abort_interp("Non-variable argument (1)");

  switch (fr -> variables[p_oper1->data.number].type)
  {
    case PROG_INTEGER:
      fr -> variables[p_oper1->data.number].data.number--;
      break;
    case PROG_OBJECT:
      fr -> variables[p_oper1->data.number].data.objref--;
      break;
    default:
       abort_interp("Variable contains non-integer");
       break;
  }
  CLEAR(p_oper1);
}

void prims_bitor (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (!arith_type(p_oper2, p_oper1)) abort_interp("Invalid argument type.");

  p_result = p_oper1->data.number | p_oper2->data.number;

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, p_oper2->type, MIPSCAST &p_result);
}

void prims_bitand (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (!arith_type(p_oper2, p_oper1)) abort_interp("Invalid argument type.");

  p_result = p_oper1->data.number & p_oper2->data.number;

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, p_oper2->type, MIPSCAST &p_result);
}

void prims_bitnot (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_INTEGER) abort_interp("Invalid argument type.");

  p_result = ~p_oper1->data.number;

  CLEAR(p_oper1);
  push(arg, top, p_oper1->type, MIPSCAST &p_result);
}

void prims_bitrotleft (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (!arith_type(p_oper2, p_oper1)) abort_interp("Invalid argument type.");

  if (p_oper1->data.number < 0)
    p_result = p_oper2->data.number >> (-p_oper1->data.number);
  else
    p_result = p_oper2->data.number << p_oper1->data.number;

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, p_oper2->type, MIPSCAST &p_result);
}

void prims_bitrotright (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (!arith_type(p_oper2, p_oper1)) abort_interp("Invalid argument type.");

  if (p_oper1->data.number < 0)
    p_result = p_oper2->data.number << (-p_oper1->data.number);
  else
    p_result = p_oper2->data.number >> p_oper1->data.number;

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, p_oper2->type, MIPSCAST &p_result);
}

void prims_pi (__P_PROTO)
{
#ifndef M_PI
  p_float = 3.14159265358979323846;
#else
  p_float = M_PI;
#endif
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_e (__P_PROTO)
{
#ifndef M_E
  p_float = 2.7182818284590452354;
#else
  p_float = M_E;
#endif
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_sin (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = sin(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_cos (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = cos(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_tan (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = tan(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_asin (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = asin(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_acos (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = acos(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_atan (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = atan(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_atan2 (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");
  if (p_oper2->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = atan2(p_oper2->data.fnum, p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_log10 (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = exp(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_pow (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");
  if (p_oper2->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = pow(p_oper2->data.fnum, p_oper2->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_sqrt (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = sqrt(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_cbrt (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_CBRT
  abort_interp("Sorry, this computer doesn't support the cbrt function.");
#else
  p_float = cbrt(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_sinh (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = sinh(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_cosh (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = cosh(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_tanh (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = tanh(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_asinh (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ASINH
  abort_interp("Sorry, this computer doesn't support the asinh function.");
#else
  p_float = asinh(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_acosh (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ASINH
  abort_interp("Sorry, this computer doesn't support the acosh function.");
#else
  p_float = acosh(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_atanh (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ASINH
  abort_interp("Sorry, this computer doesn't support the atanh function.");
#else
  p_float = atanh(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_ceil (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = ceil(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_floor (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

  p_float = floor(p_oper1->data.fnum);
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_finite (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_FINITE
   abort_interp("Sorry, this computer doesn't support the finite function.");
#else
  p_result = finite(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_isinf (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ISINF
   abort_interp("Sorry, this computer doesn't support the isinf function.");
#else
  p_result = isinf(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_isnan (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ISNAN
   abort_interp("Sorry, this computer doesn't support the isnan function.");
#else
  p_result = isnan(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_isnormal (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ISNORMAL
   abort_interp("Sorry, this computer doesn't support the isnormal function.");
#else
  p_result = isnormal(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_issubnormal (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ISSUBNORMAL
abort_interp("Sorry, this computer doesn't support the issubnormal function.");
#else
  p_result = issubnormal(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_fabs (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_FABS
  abort_interp("Sorry, this computer doesn't support the fabs function.");
#else
  p_float = fabs(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_remainder (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");
  if (p_oper2->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_REMAINDER
  p_float = fmod(p_oper2->data.fnum, p_oper1->data.fnum);
#else
  p_float = remainder(p_oper2->data.fnum, p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_j0 (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_J1
  abort_interp("Sorry, this computer doesn't support the j0 function.");
#else
  p_float = j0(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_j1 (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_J1
  abort_interp("Sorry, this computer doesn't support the j1 function.");
#else
  p_float = j1(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_y0 (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_Y1
  abort_interp("Sorry, this computer doesn't support the y0 function.");
#else
  p_float = y0(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_y1 (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_Y1
  abort_interp("Sorry, this computer doesn't support the y1 function.");
#else
  p_float = y1(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_jn (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (p_oper1->type != PROG_INTEGER) abort_interp("Invalid argument type.");
  if (p_oper2->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_J1
  abort_interp("Sorry, this computer doesn't support the jn function.");
#else
  p_float = jn(p_oper2->data.fnum, p_oper2->data.number);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_yn (__P_PROTO)
{
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (p_oper1->type != PROG_INTEGER) abort_interp("Invalid argument type.");
  if (p_oper2->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_Y1
  abort_interp("Sorry, this computer doesn't support the yn function.");
#else
  p_float = yn(p_oper2->data.fnum, p_oper2->data.number);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_erf (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ERFC
  abort_interp("Sorry, this computer doesn't support the erf function.");
#else
  p_float = erf(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

void prims_erfc (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");

#ifndef HAVE_ERFC
  abort_interp("Sorry, this computer doesn't support the erfc function.");
#else
  p_float = erfc(p_oper1->data.fnum);
#endif
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
}

#ifdef HAVE_LGAMMA
int signgam;
#endif

void prims_lgamma (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_FLOAT) abort_interp("Invalid argument type.");


#ifndef HAVE_LGAMMA
  abort_interp("Sorry, this computer doesn't support the lgamma function.");
#else
  p_float = lgamma(p_oper1->data.fnum);
#endif
  p_result = signgam;
  push(arg, top, PROG_FLOAT, MIPSCAST &p_float);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}
