#include "prims.h"

void prims_moveto    (__P_PROTO);
void prims_contents  (__P_PROTO);
void prims_exits     (__P_PROTO);
void prims_next      (__P_PROTO);
void prims_match     (__P_PROTO);
void prims_rmatch    (__P_PROTO);
void prims_owner     (__P_PROTO);
void prims_location  (__P_PROTO);
void prims_getlink   (__P_PROTO);
void prims_online    (__P_PROTO);
void prims_db_top    (__P_PROTO);
void prims_dbtop     (__P_PROTO);
void prims_chown     (__P_PROTO);
void prims_linkcount (__P_PROTO);
void prims_getlinks  (__P_PROTO);
void prims_trig      (__P_PROTO);
void prims_callers   (__P_PROTO);
void prims_caller    (__P_PROTO);
void prims_prog      (__P_PROTO);
void prims_backlinks (__P_PROTO);
void prims_backlocks (__P_PROTO);
void prims_nextowned (__P_PROTO);
void prims_lock      (__P_PROTO);
void prims_unlock    (__P_PROTO);
void prims_shutdown  (__P_PROTO);
void prims_dump      (__P_PROTO);
void prims_toad      (__P_PROTO);
void prims_stats      (__P_PROTO);
void prims_part_pmatch (__P_PROTO);

#define PRIMS_OBJECTS_TL "MOVETO", "CONTENTS", "EXITS", \
  "NEXT", "MATCH", "RMATCH", "OWNER", "LOCATION", "GETLINK", \
  "ONLINE", "DB_TOP", "DBTOP", "CHOWN", "LINKCOUNT", "GETLINKS", \
  "TRIG", "CALLERS", "CALLER", "PROG", "BACKLINKS", "BACKLOCKS", \
  "NEXTOWNED", "LOCK", "UNLOCK", "SHUTDOWN", "DUMP", "TOAD", "STATS", \
  "PART_PMATCH"

#define PRIMS_OBJECTS_FL prims_moveto, prims_contents, prims_exits, \
  prims_next, prims_match, prims_rmatch, prims_owner, \
  prims_location, prims_getlink, prims_online, prims_db_top, \
  prims_dbtop, prims_chown, prims_linkcount, prims_getlinks, prims_trig, \
  prims_callers, prims_caller, prims_prog, prims_backlinks, \
  prims_backlocks, prims_nextowned, prims_lock, prims_unlock, prims_shutdown, \
  prims_dump, prims_toad, prims_stats, prims_part_pmatch

#define PRIMS_OBJECTS_LEN 29
