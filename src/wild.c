#include "copyright.h"
#include "config.h"
#include "interface.h"
#include <stdio.h>

char *wptr[10];           /* wild card routine(s) created by Lawrence Foard */
int wlen[10];
char wbuff[BUFFER_LEN];

int wild(char *s, char *d, int p, int os)
{
  switch (*s) {
    case '?':			/* match any character in d, note end of
				 * string is considered a match */
      /* if just in nonwildcard state record location of change */
      if (!os && (p < 10))
	wptr[p] = d;
      return (wild(s + 1, (*d) ? d + 1 : d, p, 1));
    case '*':			/* match a range of characters */
      if (!os && (p < 10)) {
	wptr[p] = d;
      }
      return (wild(s + 1, d, p, 1) || ((*d) ? wild(s, d + 1, p, 1) : 0));
    default:
      if (os && (p < 10)) {
	wlen[p] = d - wptr[p];
	p++;
      }
      return ((toupper(*s) != toupper(*d)) ? 0 :
	      ((*s) ? wild(s + 1, d + 1, p, 0) : 1));
  }
}

int wild_match(char *s, char *d)
{
  int a;

  if(!*s || !*d) return 0;
  for (a = 0; a < 10; a++) wptr[a] = NULL;

  switch (*s) {
    case '>':
      s++;
      /* if both first letters are #'s then numeric compare */
      if (isdigit(s[0]) || (*s == '-'))
	return (atoi(s) < atoi(d));
      else
	return (strcmp(s, d) < 0);
    case '<':
      s++;
      if (isdigit(s[0]) || (*s == '-'))
	return (atoi(s) > atoi(d));
      else
	return (strcmp(s, d) > 0);
    default:
      if (wild(s, d, 0, 0)) {
	int b;
	char *e, *f = wbuff;
	for (a = 0; a < 10; a++)
	  if ((e = wptr[a])) {
	    wptr[a] = f;
	    for (b = wlen[a]; b--; *f++ = *e++) ;
	    *f++ = 0;
	  }
	return (1);
      } else
	return (0);
  }
}
