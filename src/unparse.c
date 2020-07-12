#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "params.h"
#include "interface.h"

char *unparse_name(dbref thing)
{
  static char buf[BUFFER_LEN], *first;
  strcpy(buf, DBFETCH(thing)->name);
  first = (char *)strchr(buf, ';');
  if (first) *first = '\0';
  return buf;
}

char *unparse_object_do(dbref player, dbref loc, int shortname)
{
  static char buf[BUFFER_LEN];

  switch(loc)
  {
    case NOTHING:
      return "*NOTHING*";
    case HOME:
      return "*HOME*";
    default:
      if ((can_link_to(player, NOTYPE, loc)
#ifdef PLAYER_CHOWN
        || ((Typeof(loc) != TYPE_PLAYER)
        && (FLAGS(loc) & CHOWN_OK))
#endif /* PLAYER_CHOWN */
        )
#ifdef SILENT_PLAYERS
        && !(FLAGS(player) & STICKY)
#endif /* SILENT_PLAYERS */
        )
      {
        /* show everything */
#ifdef ABODE
        snprintf(buf, BUFFER_LEN, "%s(#%ld%s%s%s)", shortname ? unparse_name(loc) : NAME(loc),
          loc, unparse_flags(loc), FLAGS(player) & ABODE ? " " : "", FLAGS(player) & ABODE ? unparse_name(OWNER(loc)) : "");
#else /* !ABODE */
        snprintf(buf, BUFFER_LEN, "%s(#%ld%s)", shortname ? unparse_name(loc) : NAME(loc),
          loc, unparse_flags(loc));
#endif
        return buf;
      }
      else return unparse_name(loc);
  }
}

#define OVERFLOW 512

static char boolexp_buf[BUFFER_LEN];
static char *buftop;

static void unparse_boolexp1(dbref player, boolexp *b, boolexp_type outer_type)
{

  if ((buftop - boolexp_buf) > (BUFFER_LEN - OVERFLOW))
  {
    strcpy(buftop, "... (ovflw)");
    buftop += strlen(buftop);
  }
  else if(b == TRUE_BOOLEXP)
  {
    strcpy(buftop, "*UNLOCKED*");
    buftop += strlen(buftop);
  }
  else
  {
    switch(b->type)
    {
      case BOOLEXP_AND:
        if(outer_type == BOOLEXP_NOT) *buftop++ = '(';
        unparse_boolexp1(player, b->sub1, b->type);
        *buftop++ = AND_TOKEN;
        unparse_boolexp1(player, b->sub2, b->type);
        if(outer_type == BOOLEXP_NOT) *buftop++ = ')';
        break;
      case BOOLEXP_OR:
        if(outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND)
          *buftop++ = '(';
        unparse_boolexp1(player, b->sub1, b->type);
        *buftop++ = OR_TOKEN;
        unparse_boolexp1(player, b->sub2, b->type);
        if(outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND)
          *buftop++ = ')';
        break;
      case BOOLEXP_NOT:
        *buftop++ = '!';
        unparse_boolexp1(player, b->sub1, b->type);
        break;
      case BOOLEXP_CONST:
        strcpy(buftop, unparse_object(player, b->thing));
        buftop += strlen(buftop);
        break;
      case BOOLEXP_PROP:
        strcpy(buftop, b->prop_name);
        strcat(buftop, ":");
        if (b->prop_data) strcat(buftop, b->prop_data);
        buftop += strlen(buftop);
        break;
      default:
        abort();          /* bad type */
        break;
    }
  }
}

char *unparse_boolexp(dbref player, boolexp *b)
{
  buftop = boolexp_buf;
  unparse_boolexp1(player, b, BOOLEXP_CONST);   /* no outer type */
  *buftop++ = '\0';

  return boolexp_buf;
}
