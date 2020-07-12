/* autoconf.h.  Generated automatically by configure.  */
/* autoconf.h.in -- System-dependent configuration information */

#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "copyright.h"

/* ---------------------------------------------------------------------------
 * Configuration section:
 *
 * These defines are written by the configure script.
 * Change them if need be
 */

/* Define if we have stdlib.h et al */
#define STDC_HEADERS 1
/* Define if we have string.h instead of strings.h */
#undef USG
/* Define if we have unistd.h */
#define HAVE_UNISTD_H 1
/* Define if we have memory.h and need it to get memcmp et al */
#undef NEED_MEMORY_H
/* Decl for pid_t */
#undef pid_t
/* Define if we have vfork.h */
#undef HAVE_VFORK_H
/* Define if vfork is broken */
#undef vfork
/* Define if wait3 exists and works */
#undef HAVE_WAIT3
/* Define if struct tm is not in time.h */
#undef TM_IN_SYS_TIME
/* Define if struct tm has a timezone member */
#define HAVE_TM_ZONE 1
/* Define if tzname[] exists */
#undef HAVE_TZNAME
/* Define if setrlimit exists */
#define HAVE_SETRLIMIT 1
/* Define if getrusage exists */
#define HAVE_GETRUSAGE 1
/* Define if timelocal exists */
#define HAVE_TIMELOCAL 1
/* Define if mktime exists */
#define HAVE_MKTIME 1
/* Define if getdtablesize exists */
#define HAVE_GETDTABLESIZE 1
/* Define if getpagesize exists */
#define HAVE_GETPAGESIZE 1
/* Define if srandom exists */
#define HAVE_SRANDOM 1
/* Define if srandom exists */
#define HAVE_DRAND48 1
/* Define if strftime exists */
#define HAVE_STRFTIME 1
/* Define if cbrt exists */
#define HAVE_CBRT 1
/* Define if asinh exists */
#define HAVE_ASINH 1
/* Define if remainer exists */
#undef HAVE_REMAINER
/* Define if lgamma exists */
#define HAVE_LGAMMA 1
/* Define if erfc exists */
#define HAVE_ERFC 1
/* Define if y1 exists */
#define HAVE_Y1 1
/* Define if j1 exists */
#define HAVE_J1 1
/* Define if fabs exists */
#define HAVE_FABS 1
/* Define if isnormal exists */
#undef HAVE_ISNORMAL
/* Define if issubnormal exists */
#undef HAVE_ISSUBNORMAL
/* Define if finite exists */
#define HAVE_FINITE 1
/* Define if isinf exists */
#define HAVE_ISINF 1
/* Define if isnan exists */
#define HAVE_ISNAN 1
/* Define if sys_siglist[] exists */
#define HAVE_SYS_SIGLIST 1
/* Define if _sys_siglist[] exists */
#undef HAVE__SYS_SIGLIST
/* Define if sys/socketvar.h exists */
#define HAVE_SYS_SOCKETVAR_H 1
/* Define if const is broken */
#undef const
/* Define if char type is unsigned */
#undef __CHAR_UNSIGNED__
/* Define if inline keyword is broken or nonstandard */
#undef inline
/* Define if you need to declare sys_siglist yourself */
#undef NEED_SYS_SIGLIST_DCL
/* Define if you need to declare _sys_siglist yourself */
#undef NEED__SYS_SIGLIST_DCL
/* Define if you need to declare sys_errlist yourself */
#define NEED_SYS_ERRLIST_DCL 1
/* Define if you need to declare _sys_errlist yourself */
#undef NEED_SYS__ERRLIST_DCL
/* Define if you need to declare perror yourself */
#define NEED_PERROR_DCL 1
/* Define if you need to declare getrusage yourself */
#undef NEED_GETRUSAGE_DCL
/* Define if struct linger is defined */
#define HAVE_LINGER 1
/* Define if stdio.h defines lots of extra functions */
#define EXTENDED_STDIO_DCLS 1
/* Define if sys/socket.h defines lots of extra functions */
#define EXTENDED_SOCKET_DCLS 1
/* Define if sys/wait.h defines union wait. */
#define HAVE_UNION_WAIT 1
/* Define if we have system-supplied ndbm routines */
#define HAVE_NDBM 1
/* Define if we have system-supplied (old) dbm routines */
#undef HAVE_DBM
/* Define if sys/time.h includes time.h but time.h doesn't protect against
 * multiple inclusions. */
#undef TIME_H_BRAINDAMAGE
/* Define this if your compiler is smart enough to allocate variable length
 * local character variables */
#undef HAVE_VAR_LOCAL_VAR

/* ---------------------------------------------------------------------------
 * Setup section:
 *
 * Load system-dependent header files.
 */

#ifdef STDC_HEADERS
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#else
#include <varargs.h>
extern int atoi(const char *);
extern double atof(const char *);
#endif

#ifdef NEED_MEMORY_H
#include <memory.h>
#endif

#if defined(USG) || defined(STDC_HEADERS)
#include <string.h>
#define	index	strchr
#define	rindex	strrchr
#define	bcopy(s,d,n)	memcpy(d,s,n)
#define	bcmp(s1,s2,n)	memcmp(s1,s2,n)
#define	bzero(s,n)	memset(s,0,n)
#else
#include <strings.h>
extern char *strtok(char *, char *);
extern void bcopy(char *, char *, int);
extern void bzero(char *, int);
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef TIME_H_BRAINDAMAGE
#include <time.h>
#endif
#include <sys/time.h>

#if defined(HAVE_SETRLIMIT) || defined(HAVE_GETRUSAGE)
#include <sys/resource.h>
#ifdef NEED_GETRUSAGE_DCL
extern int getrusage(int, struct rusage *);
#endif
#endif

#include <sys/param.h>
#ifndef HAVE_GETPAGESIZE
#ifdef EXEC_PAGESIZE
#define getpagesize() EXEC_PAGESIZE
#else
#ifdef NBPG
#ifndef CLSIZE
#define CLSIZE 1
#endif /* no CLSIZE */
#define getpagesize() NBPG * CLSIZE
#else /* no NBPG */
#define getpagesize() NBPC
#endif /* no NBPG */
#endif /* no EXEC_PAGESIZE */
#else
#ifndef HAVE_UNISTD_H
extern int getpagesize(void);
#endif /* HAVE_UNISTD_H */
#endif /* HAVE_GETPAGESIZE_H */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#ifdef NEED_PERROR_DCL
extern void perror(const char *);
#endif
#else
extern int errno;
extern void perror(const char *);
#endif

#ifndef HAVE_TIMELOCAL

#ifndef HAVE_MKTIME
#define NEED_TIMELOCAL
extern time_t timelocal (struct tm *);
#else
#define timelocal mktime
#endif /* HAVE_MKTIME */

#endif /* HAVE_TIMELOCAL */

#ifndef HAVE_SRANDOM
#define random rand
#define srandom srand
#else
#ifndef random	/* only if not a macro */
extern long random(void);
#endif
#endif /* HAVE_SRANDOM */

#ifdef HAVE_DRAND48
extern void srand48(long);
extern double drand48(void);
#endif

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/socket.h>

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#endif /* AUTOCONF_H */
