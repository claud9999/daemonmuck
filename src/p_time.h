#include "prims.h"

void prims_systime       (__P_PROTO);
void prims_ctime         (__P_PROTO);
void prims_time          (__P_PROTO);
void prims_date          (__P_PROTO);
void prims_gmtsplit      (__P_PROTO);
void prims_timesplit     (__P_PROTO);
void prims_strftime      (__P_PROTO);
void prims_gmtoffset     (__P_PROTO);
#ifdef TIMESTAMPS
void prims_touch         (__P_PROTO);
void prims_touch_created (__P_PROTO);
void prims_touch_modified(__P_PROTO);
void prims_time_created  (__P_PROTO);
void prims_time_modified (__P_PROTO);
void prims_time_used     (__P_PROTO);
#endif /*TIMESTAMPS*/


#ifdef TIMESTAMPS
#define PRIMS_TIME_FL prims_touch, prims_touch_created, prims_touch_modified, \
  prims_time_created, prims_time_modified, prims_time_used, prims_systime, \
  prims_ctime, prims_time, prims_date, prims_gmtsplit, prims_timesplit, \
  prims_strftime, prims_gmtoffset
#define PRIMS_TIME_TL "TOUCH", "TOUCH_CREATED", "TOUCH_MODIFIED", \
  "TIME_CREATED", "TIME_MODIFIED", "TIME_USED", "SYSTIME", "CTIME", "TIME", \
  "DATE", "GMTSPLIT", "TIMESPLIT", "STRFTIME", "GMTOFFSET"
#define PRIMS_TIME_LEN  14
#else /* !TIMESTAMPS */
#define PRIMS_TIME_FL prims_systime, prims_ctime, prims_time, prims_date, \
                      prims_gmtsplit, prims_timesplit, prims_strftime, \                              prims_gmtoffset
#define PRIMS_TIME_TL "SYSTIME", "CTIME", "TIME", "DATE", "GMTSPLIT", \
                      "TIMESPLIT", "STRFTIME", "GMTOFFSET"
#define PRIMS_TIME_LEN 8
#endif /*TIMESTAMPS*/
