#include "config.h"
#ifdef USE_MALLOC

#include "externs.h"

#include <signal.h>
#include <stdarg.h>

/* #define MALTRACE */
#define USE_MAGIC /**/
#define RECORD_CALLER /**/
#define DO_FILLS /**/
/* #define RECORD_BUSY */
#define ALIGNMENT 4
#define PANIC_POOL 65536

#define OFFSET ((((sizeof(int)-1)/ALIGNMENT)+1)*ALIGNMENT)

#ifdef NeXT
#define malloc  bsd_malloc
#define realloc bsd_realloc
#define free    bsd_free
#endif

#undef malloc
#undef realloc
#undef calloc
#undef free

extern void *malloc(int);
extern void *realloc(const void *,int);
extern void free(const void *);

static char panic_pool[PANIC_POOL];
static int panicked = 0;
static int used = 0;

void setpanicmalloc(void)
{
 panicked = 1;
}

static void chkused(void)
{
 if (used > PANIC_POOL)
  { write(2,"out of panic memory\n",20);
    abort();
  }
}

static void *panic_malloc(int nb)
{
 if (! panicked) return(malloc(nb));
 if (nb == 0) return(0);
 nb = (((nb-1)/ALIGNMENT)+1)*ALIGNMENT;
 used += nb + OFFSET;
 chkused();
 *((int *)&panic_pool[used-nb-OFFSET]) = nb;
 return(&panic_pool[used-nb]);
}

static void *panic_realloc(void *old, int newnb)
{
 int oldnb;

 if (! panicked) return(realloc(old,newnb));
 if (old == 0) return(panic_malloc(newnb));
 if (((char *)old < &panic_pool[0]) || ((char *)old >= &panic_pool[PANIC_POOL]))
  { write(2,"bad panic realloc\n",18);
    abort();
  }
 oldnb = *(int *)(((char *)old)-OFFSET);
 newnb = (((newnb-1)/ALIGNMENT)+1)*ALIGNMENT;
 if (((char *)old)+oldnb == &panic_pool[used])
  { used += newnb - oldnb;
    chkused();
    *(int *)(((char *)old)-OFFSET) = newnb;
    return(old);
  }
 else
  { void *new;
    new = panic_malloc(newnb);
    bcopy(old,new,(oldnb<newnb)?oldnb:newnb);
    return(new);
  }
}

static void panic_free(void *obj)
{
 if (! panicked)
  { free(obj);
    return;
  }
}

#undef malloc
#undef realloc
#undef free
#define malloc(nb)      panic_malloc(nb)
#define realloc(old,nb) panic_realloc(old,nb)
#define free(old)       panic_free(old)

#if defined(MALTRACE) || defined(RECORD_BUSY)
char *fmt_aux(char *bp,unsigned int n,int b)
{
 if (n < b)
  { *bp++ = "0123456789abcdef"[n];
  }
 else
  { bp = fmt_aux(bp,n/b,b);
    bp = fmt_aux(bp,n%b,b);
  }
 *bp = '\0';
 return(bp);
}
#endif

#if defined(MALTRACE) || defined(RECORD_BUSY)
#define fmt_dec(bp,i) (void)fmt_aux(bp,i,10)
#define fmt_hex(bp,i) (void)fmt_aux(bp,i,16)
#endif

#ifdef USE_MAGIC
#define MMAGIC 0xdeadbeefU
#endif

#ifdef RECORD_BUSY
static unsigned long int busy[BUSY_SIZE];
static int busy_fill = 0;
#endif

#ifdef RECORD_BUSY
static void record_busy(unsigned long int p)
{
 char buf[128];
 register int l;
 register int m;
 register int h;
 register unsigned long int v;

 l = -1;
 h = busy_fill;
 if (h >= BUSY_SIZE)
  { strcpy(&buf[0],"malloc busy table overflow!\n");
    write(2,&buf[0],strlen(&buf[0]));
    abort();
  }
 while (h-l > 1)
  { m = (h + l) / 2;
    v = busy[m];
    if (v <= p) l = m;
    if (v >= p) h = m;
  }
 if (l == h)
  { strcpy(&buf[0],"malloc returned 0x");
    fmt_hex(&buf[strlen(&buf[0])],(int)p);
    strcpy(&buf[strlen(&buf[0])],"twice!\n");
    write(2,&buf[0],strlen(&buf[0]));
    abort();
  }
 if (h < busy_fill)
  { bcopy((char *)&busy[h],(char *)&busy[h+1],(busy_fill-h)*sizeof(busy[0]));
  }
 busy[h] = p;
 busy_fill ++;
}
#endif

#ifdef RECORD_BUSY
static void record_unbusy(unsigned long int p)
{
 char buf[128];
 register int l;
 register int m;
 register int h;
 register unsigned long int v;

 l = -1;
 h = busy_fill;
 if (h < 1)
  { strcpy(&buf[0],"malloc busy table underflow!\n");
    write(2,&buf[0],strlen(&buf[0]));
    abort();
  }
 while (h-l > 1)
  { m = (h + l) / 2;
    v = busy[m];
    if (v <= p) l = m;
    if (v >= p) h = m;
  }
 if (l != h)
  { strcpy(&buf[0],"freeing unallocated 0x");
    fmt_hex(&buf[strlen(&buf[0])],(int)p);
    strcpy(&buf[strlen(&buf[0])],"!\n");
    write(2,&buf[0],strlen(&buf[0]));
    abort();
  }
 busy_fill --;
 if (h < busy_fill)
  { bcopy((char *)&busy[h+1],(char *)&busy[h],(busy_fill-h)*sizeof(busy[0]));
  }
}
#endif

#ifdef DO_FILLS
static void fill(char *p, int n, int i)
{
 unsigned int l;

 i = '@' + ((i-1) << 3);
 l = n;
 for (;n>0;n--)
  { *p++ = i + (l & 7);
    l >>= 3;
  }
 p[-1] |= 040;
}
#endif

#define XOFF0 0

#if defined(RECORD_CALLER) || defined(DO_FILLS)
#define BLK_NB(p) (*(int *)((p)+(XOFF0*ALIGNMENT)))
#define XOFF1 (XOFF0+1)
#else
#define XOFF1 (XOFF0)
#endif

#ifdef RECORD_CALLER
#define BLK_FL(p) (*(void **)            ((p)+((XOFF1  )*ALIGNMENT)))
#define BLK_BL(p) (*(void **)            ((p)+((XOFF1+1)*ALIGNMENT)))
#define BLK_FN(p) (*(const char **)      ((p)+((XOFF1+2)*ALIGNMENT)))
#define BLK_LN(p) (*(int *)              ((p)+((XOFF1+3)*ALIGNMENT)))
#define BLK_TM(p) (*(int *)              ((p)+((XOFF1+4)*ALIGNMENT)))
#define XOFF2 (XOFF1+5)
static char *malloc_root = 0;
#else
#define XOFF2 (XOFF1)
#endif

#ifdef USE_MAGIC
#define BLK_MG(p) (*(unsigned long int *)((p)+(XOFF2*ALIGNMENT)))
#define XOFF3 (XOFF2+1)
#else
#define XOFF3 (XOFF2)
#endif

#define EXTRABYTES (XOFF3*ALIGNMENT)

void *muck_malloc(int nb, const char *file, int line)
{
 char *p;
#ifdef MALTRACE
 char buf[128];
#endif

#ifdef MALTRACE
 strcpy(&buf[0],"[malloc ");
 fmt_dec(&buf[strlen(&buf[0])],nb);
 strcpy(&buf[strlen(&buf[0])]," -> ");
#endif
 p = malloc(nb+EXTRABYTES);
#ifdef RECORD_BUSY
 record_busy((unsigned long int)p);
#endif
#ifdef BLK_NB
 BLK_NB(p) = nb;
#endif
#ifdef BLK_FL
 BLK_FL(p) = malloc_root;
 BLK_BL(p) = 0;
 if (malloc_root) BLK_BL(malloc_root) = p;
 malloc_root = p;
#endif
#ifdef BLK_FN
 BLK_FN(p) = file;
#endif
#ifdef BLK_LN
 BLK_LN(p) = line;
#endif
#ifdef BLK_TM
 BLK_TM(p) = time(0);
#endif
#ifdef BLK_MG
 BLK_MG(p) = MMAGIC;
#endif
 p += EXTRABYTES;
#ifdef DO_FILLS
 fill(p,nb,1);
#endif
#ifdef MALTRACE
 fmt_hex(&buf[strlen(&buf[0])],(int)p);
 strcpy(&buf[strlen(&buf[0])],"]\n");
 write(2,&buf[0],strlen(&buf[0]));
#endif
 return(p);
}

void *muck_realloc(void *old, int nb, const char *file, int line)
{
#ifdef BLK_NB
 int onb;
#endif
 char *p;
#ifdef MALTRACE
 char buf[128];
#endif

#ifdef MALTRACE
 strcpy(&buf[0],"[realloc ");
 fmt_hex(&buf[strlen(&buf[0])],(int)old);
 strcpy(&buf[strlen(&buf[0])],",");
 fmt_dec(&buf[strlen(&buf[0])],nb);
 strcpy(&buf[strlen(&buf[0])]," -> ");
#endif
 if (old == 0)
  { p = malloc(nb+EXTRABYTES);
#ifdef RECORD_BUSY
    record_busy((unsigned long int)p);
#endif
#ifdef BLK_FL
    BLK_FL(p) = malloc_root;
    BLK_BL(p) = 0;
    if (malloc_root) BLK_BL(malloc_root) = p;
    malloc_root = p;
#endif
#ifdef DO_FILLS
    fill(p+EXTRABYTES,nb,2);
#endif
  }
 else
  { p = ((char *)old) - EXTRABYTES;
#ifdef BLK_MG
    if (BLK_MG(p) != MMAGIC)
     { write(2,"realloc: bad magic number\n",26);
       abort();
     }
#endif
#ifdef BLK_NB
    onb = BLK_NB(p);
#endif
    p = realloc(p,nb+EXTRABYTES);
#ifdef RECORD_BUSY
    if (p != ((char *)old)-EXTRABYTES)
     { record_unbusy((unsigned long int)(((char *)old)-EXTRABYTES));
       record_busy((unsigned long int)p);
     }
#endif
#ifdef DO_FILLS
    if (onb < nb) fill(onb+p+EXTRABYTES,nb-onb,3);
#endif
  }
#ifdef BLK_NB
 BLK_NB(p) = nb;
#endif
#ifdef BLK_FN
 BLK_FN(p) = file;
#endif
#ifdef BLK_LN
 BLK_LN(p) = line;
#endif
#ifdef BLK_TM
 BLK_TM(p) = time(0);
#endif
#ifdef BLK_MG
 BLK_MG(p) = MMAGIC;
#endif
 p += EXTRABYTES;
#ifdef MALTRACE
 fmt_hex(&buf[strlen(&buf[0])],(int)p);
 strcpy(&buf[strlen(&buf[0])],"]\n");
 write(2,&buf[0],strlen(&buf[0]));
#endif
 return(p);
}

void *muck_calloc(int siz, int n, const char *file, int line)
{
 write(2,"[calloc]\n",9);
 abort();
 return(0);
}

void *free_break = 0; /* for debugger patching */

void muck_free(void *obj, const char *file, int line)
{
 char *p;
#ifdef MALTRACE
 char buf[128];
#endif

#ifdef MALTRACE
 strcpy(&buf[0],"[free ");
 fmt_hex(&buf[strlen(&buf[0])],(int)obj);
 strcpy(&buf[strlen(&buf[0])],"]\n");
 write(2,&buf[0],strlen(&buf[0]));
#endif
 if (obj == 0) return;
 if (obj == free_break) kill(getpid(),SIGINT);
 p = ((char *)obj) - EXTRABYTES;
#ifdef BLK_MG
 if (BLK_MG(p) != MMAGIC)
  { write(2,"free: bad magic number\n",23);
    abort();
  }
#endif
#ifdef BLK_FL
  { char *f;
    char *b;
    f = BLK_FL(p);
    b = BLK_BL(p);
    if (f) BLK_BL(f) = b;
    if (b) BLK_FL(b) = f;
    if (! b) malloc_root = f;
  }
#endif
#ifdef DO_FILLS
 fill(p,BLK_NB(p)+EXTRABYTES,4);
#endif
#ifdef RECORD_BUSY
 record_unbusy((unsigned long int)p);
#endif
 free(p);
}

static void cfree_(int i, ...)
{
 va_list ap;
 void *old;
 const char *file;
 int line;

 va_start(ap,i);
 old = va_arg(ap,void *);
 file = va_arg(ap,const char *);
 line = va_arg(ap,int);
 muck_free(old,file,line);
 va_end(ap);
}

void muck_cfree(const void *obj, const char *file, int line)
{
 cfree_(0,obj,file,line);
}

#ifdef RECORD_CALLER
static void mlcprintf(const char *fmt, ...)
{
 static FILE *mlcfile = 0;
 va_list ap;

 if (! mlcfile)
  { mlcfile = fopen("mlcfile","w");
    if (mlcfile == 0) return;
  }
 va_start(ap,fmt);
 vfprintf(mlcfile,fmt,ap);
 va_end(ap);
}
#endif

#ifdef RECORD_CALLER
static void printbytes(char *bytes, int nb)
{
 int i;
 int c;
 int nbin;
 const char *trailer;

 if (nb > 25)
  { nb = 25;
    trailer = " ...";
  }
 else
  { trailer = "";
  }
 nbin = 0;
 for (i=0;i<nb;i++)
  { c = ((unsigned char *)bytes)[i];
    if ( (c > 126) ||
	 ( (c < 32) &&
	   (c != '\t') &&
	   (c != '\n') &&
	   (c != '\r') ) ) nbin ++;
  }
 if ((nbin > 1) || ((nbin == 1) && (bytes[nb-1] != '\0')))
  { for (i=0;i<nb;i++)
     { mlcprintf(" %02x",((unsigned char *)bytes)[i]);
     }
  }
 else
  { mlcprintf(" ");
    for (i=0;i<nb;i++)
     { c = ((unsigned char *)bytes)[i];
       switch (c)
	{ case '\0': mlcprintf("\\0"); break;
	  case '\r': mlcprintf("\\r"); break;
	  case '\t': mlcprintf("\\t"); break;
	  case '\n': mlcprintf("\\n"); break;
	  default: mlcprintf("%c",c);
	}
     }
  }
 mlcprintf("%s\n",trailer);
}
#endif

void malloc_leakcheck(void)
{
#ifdef RECORD_CALLER
 char *p;
 char *p2;
 int flip;

 p = malloc_root;
 p2 = malloc_root;
 flip = 0;
 while (p)
  { mlcprintf("\"%s\", line %d: %d byte%s at %.24s\n",
	BLK_FN(p),
	BLK_LN(p),
	BLK_NB(p), (BLK_NB(p)==1)?"":"s",
	ctime(&BLK_TM(p)) );
    printbytes(p+EXTRABYTES,BLK_NB(p));
    p = BLK_FL(p);
    if (flip)
     { p2 = BLK_FL(p2);
     }
    else
     { if (p == p2)
	{ mlcprintf("BLK_FL loop\n");
	  break;
	}
     }
    flip = ! flip;
  }
#endif
}

#endif /* USE_MALLOC */
