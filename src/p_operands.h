#include "prims.h"

void prims_add         (__P_PROTO);
void prims_subtract    (__P_PROTO);
void prims_multiply    (__P_PROTO);
void prims_divide      (__P_PROTO);
void prims_mod         (__P_PROTO);
void prims_lessthan    (__P_PROTO);
void prims_greathan    (__P_PROTO);
void prims_equal       (__P_PROTO);
void prims_lesseq      (__P_PROTO);
void prims_greateq     (__P_PROTO);
void prims_random      (__P_PROTO);
void prims_frandom     (__P_PROTO);
void prims_dbcomp      (__P_PROTO);
void prims_at          (__P_PROTO);
void prims_bang        (__P_PROTO);
void prims_pplus       (__P_PROTO);
void prims_mminus      (__P_PROTO);
void prims_bitor       (__P_PROTO);
void prims_bitand      (__P_PROTO);
void prims_bitnot      (__P_PROTO);
void prims_bitrotleft  (__P_PROTO);
void prims_bitrotright (__P_PROTO);
void prims_pi (__P_PROTO);
void prims_e (__P_PROTO);
void prims_sin (__P_PROTO);
void prims_cos (__P_PROTO);
void prims_tan (__P_PROTO);
void prims_asin (__P_PROTO);
void prims_acos (__P_PROTO);
void prims_atan (__P_PROTO);
void prims_atan2 (__P_PROTO);
void prims_log10 (__P_PROTO);
void prims_pow (__P_PROTO);
void prims_sqrt (__P_PROTO);
void prims_cbrt (__P_PROTO);
void prims_sinh (__P_PROTO);
void prims_cosh (__P_PROTO);
void prims_tanh (__P_PROTO);
void prims_asinh (__P_PROTO);
void prims_acosh (__P_PROTO);
void prims_atanh (__P_PROTO);
void prims_ceil (__P_PROTO);
void prims_floor (__P_PROTO);
void prims_finite (__P_PROTO);
void prims_isinf (__P_PROTO);
void prims_isnan (__P_PROTO);
void prims_isnormal (__P_PROTO);
void prims_issubnormal (__P_PROTO);
void prims_fabs (__P_PROTO);
void prims_remainder (__P_PROTO);
void prims_j0 (__P_PROTO);
void prims_j1 (__P_PROTO);
void prims_y0 (__P_PROTO);
void prims_y1 (__P_PROTO);
void prims_jn (__P_PROTO);
void prims_yn (__P_PROTO);
void prims_erf (__P_PROTO);
void prims_erfc (__P_PROTO);
void prims_lgamma (__P_PROTO);

#define PRIMS_OPERANDS_FL prims_add, prims_subtract, prims_multiply, \
       prims_divide, prims_mod, prims_lessthan, prims_greathan, \
       prims_equal, prims_lesseq, prims_greateq, prims_random, \
       prims_frandom, prims_dbcomp, prims_at, prims_bang, prims_pplus, \
       prims_mminus, prims_bitor, prims_bitand, \
       prims_bitnot, prims_bitrotleft, prims_bitrotright, \
prims_pi, prims_e, prims_sin, prims_cos, prims_tan, prims_asin, prims_acos,\
prims_atan, prims_atan2, prims_log10, prims_pow, prims_sqrt, prims_cbrt,\
prims_sinh, prims_cosh, prims_tanh, prims_asinh, prims_acosh, prims_atanh,\
prims_ceil, prims_floor, prims_finite, prims_isinf, prims_isnan,\
prims_isnormal, prims_issubnormal, prims_fabs, prims_remainder,\
prims_j0, prims_j1, prims_y0, prims_y1, prims_jn, prims_yn, prims_erf,\
prims_erfc, prims_lgamma

#define PRIMS_OPERANDS_TL "+", "-", "*", "/", "%", "<", ">", "=", \
          "<=", ">=", "RANDOM", "FRANDOM", "DBCMP", "@", "!", "++", "--", \
          "|", "&", "~", "<<", ">>", \
"PI", "E", "SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN", "ATAN2", "LOG10", \
"POW", "SQRT", "CBRT", "SINH", "COSH", "TANH", "ASINH", "ACOSH", "ATANH", \
"CEIL", "FLOOR", "FINITE", "ISINF", "ISNAN", "ISNORMAL", "ISSUBNORMAL", \
"FABS", "REMAINDER", "J0", "J1", "Y0", "Y1", "JN", "YN", "ERF", \
"ERFC", "LGAMMA"

#define PRIMS_OPERANDS_LEN 59
