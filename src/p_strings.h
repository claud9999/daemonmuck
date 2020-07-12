#include "prims.h"

void prims_stringcmp     (__P_PROTO);
void prims_stringncmp    (__P_PROTO);
void prims_strcmp        (__P_PROTO);
void prims_strncmp       (__P_PROTO);
void prims_strcut        (__P_PROTO);
void prims_strlen        (__P_PROTO);
void prims_strcat        (__P_PROTO);
void prims_explode       (__P_PROTO);
void prims_subst         (__P_PROTO);
void prims_instr         (__P_PROTO);
void prims_rinstr        (__P_PROTO);
void prims_pronoun_sub   (__P_PROTO);
void prims_toupper       (__P_PROTO);
void prims_tolower       (__P_PROTO);
void prims_flagstr       (__P_PROTO);
void prims_caps          (__P_PROTO);
void prims_unparse_lock  (__P_PROTO);
void prims_unparse_flags (__P_PROTO);
void prims_wstrcmp       (__P_PROTO);
void prims_spitfile      (__P_PROTO);
void prims_notifyfile    (__P_PROTO);
void prims_touchfile     (__P_PROTO);
void prims_spitline      (__P_PROTO);
void prims_stringpfx     (__P_PROTO);
void prims_striplead     (__P_PROTO);
void prims_striptail     (__P_PROTO);
void prims_smatch        (__P_PROTO);
void prims_version       (__P_PROTO);
void prims_unparseobj    (__P_PROTO);

#define PRIMS_STRINGS_FL prims_stringcmp, prims_stringncmp, prims_strcmp, \
  prims_strncmp, prims_strcut, prims_strlen, prims_strcat, prims_explode, \
  prims_subst, prims_instr, prims_rinstr, prims_pronoun_sub, prims_toupper, \
  prims_tolower, prims_flagstr, prims_caps, prims_unparse_lock, \
  prims_unparse_flags, prims_wstrcmp, prims_spitfile, prims_notifyfile, \
  prims_touchfile, prims_stringpfx, prims_striplead, \
  prims_striptail, prims_smatch, prims_version, prims_unparseobj

#define PRIMS_STRINGS_TL "STRINGCMP", "STRINGNCMP", "STRCMP", "STRNCMP", \
  "STRCUT", "STRLEN", "STRCAT", "EXPLODE", "SUBST", "INSTR", "RINSTR", \
  "PRONOUN_SUB", "TOUPPER", "TOLOWER", "FLAGSTR", "CAPS", "UNPARSE_BOOL", \
  "UNPARSE_FLAGS", "WSTRCMP", "SPITFILE", "NOTIFYFILE", "TOUCHFILE", \
  "STRINGPFX", "STRIPLEAD", "STRIPTAIL", "SMATCH", \
  "VERSION", "UNPARSEOBJ"

#define PRIMS_STRINGS_LEN 28
