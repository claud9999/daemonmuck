#include "copyright.h"
#include "config.h"

/* Predicates for testing various conditions */

#include <ctype.h>

#include "db.h"
#include "interface.h"
#include "params.h"
#include "externs.h"

int can_link_to(dbref who, object_flag_type what_type, dbref where)
{
  if (where == HOME) return 1;
  if (where < 0 || where > db_top) return 0;
  switch (what_type)
  {
    case TYPE_EXIT:
      return (controls(who, where) || (FLAGS(where) & LINK_OK));
    case TYPE_PLAYER:
      return (Typeof(where) == TYPE_ROOM && (controls(who, where)
        || Linkable(where)));
    case TYPE_ROOM:
    case TYPE_THING:
      return (((Typeof(where) == TYPE_ROOM) ||
        (Typeof(where) == TYPE_PLAYER)) && (controls(who, where) ||
        Linkable(where)));
    case NOTYPE:
      return ((controls(who,where)) || (FLAGS(where) & LINK_OK)
        || ((Typeof(where) != TYPE_PLAYER) && (FLAGS(where) & ABODE)));
  }
  return 0;
}

int can_link(dbref who, dbref what)
{
  return (controls(who, what) || ((Typeof(what) == TYPE_EXIT)
    && DBFETCH(what)->sp.exit.ndest == 0));
}

int could_doit(dbref player, dbref thing)
{
  if(Typeof(thing) == TYPE_EXIT && DBFETCH(thing)->sp.exit.ndest == 0) return 0;
  return(eval_boolexp (player, DBFETCH(thing)->key, thing));
}

int can_doit(dbref player, dbref thing, char *default_fail_msg)
{
  dbref loc;
  char buf[BUFFER_LEN];

  if((loc = getloc(player)) == NOTHING) return 0;

  if(!could_doit(player, thing))
  {
    /* can't do it */
    if(GET_FAIL(thing))
      exec_or_notify(player, thing, GET_FAIL(thing));
    else if(default_fail_msg)
      notify(player, player, default_fail_msg);

    if(GET_OFAIL(thing) && !Dark(player))
    {
      strcpy(buf, unparse_name(player));
      strcat(buf, " ");
      strcat(buf, pronoun_substitute(player, GET_OFAIL(thing)));
      notify_except(player, loc, player, buf);
    }
    return 0;
  }
  else
  {
    /* can do it */
    if(GET_SUCC(thing))
      exec_or_notify(player, thing, GET_SUCC(thing));

    if(GET_OSUCC(thing) && !Dark(player))
    {
      strcpy(buf, unparse_name(player));
      strcat(buf, " ");
      strcat(buf, pronoun_substitute(player, GET_OSUCC(thing)));
      notify_except(player, loc, player, buf);
    }
    return 1;
  }
}

int can_see(dbref player, dbref thing, int can_see_loc)
{
  if (can_see_loc)
  {
    switch (Typeof(thing))
    {
      case TYPE_PROGRAM:
        return((FLAGS(thing) & LINK_OK) || controls(player, thing));
      default:
        return (!Dark(thing) || controls(player, thing));
    }
  }
  else
  {
    /* can't see loc */
#ifndef SILENT_PLAYERS
    return(controls(player, thing));
#else  /* SILENT_PLAYERS */
    return(Sticky(player) ? 0 : controls(player, thing));
#endif
  }
}

int controls(dbref who, dbref what)
{
  /* Wizard controls everything */
  /* owners control their stuff */
  if (what < 0 || what >= db_top) return 0;
  if (Typeof(what) == TYPE_GARBAGE) return 0;
  if (FLAGS(who) & GOD) return 1;
#ifdef GOD_PRIV
  if (Wizard(who)) return (!(FLAGS(what) & GOD));
  if (FLAGS(what) & GOD) return 0;
#else /* !GOD_PRIV */
  if (Wizard(who)) return 1;
  if (Wizard(what)) return 0;
#endif /* GOD_PRIV */
  return (who == OWNER(what));
}

int restricted(dbref player, dbref thing, object_flag_type flag)
{
  switch (flag)
  {
    case INTERACTIVE:
      return 1;
    case DARK:
 if(o_liberal_dark) {
      return (!Wizard(player) && Typeof(thing) == TYPE_PLAYER);
      } else {
      return (!Wizard(player) && (Typeof(thing) != TYPE_ROOM)
        && (Typeof(thing) != TYPE_PROGRAM));
     }
    case MUCKER:
    case BUILDER:
      return (!Wizard(player));
    case WIZARD:
      if (Wizard(player))
#ifdef GOD_PRIV
        return ((Typeof(thing) == TYPE_PLAYER) && !God(player));
#else /* !GOD_PRIV */
        return 0;
#endif /* GOD_PRIV */
      else return 1;
    case GOD:
      return(!God(player));
    default:
      return 0;
  }
}

int payfor(dbref who, int cost)
{
  if(Wizard(who)) return 1;
  else if(DBFETCH(who)->pennies >= cost)
  {
    DBFETCH(who)->pennies -= cost;
    DBDIRTY(who);
    return 1;
  }
  else return 0;
}

int word_start (char *str, char let)
{
  int chk;

  for (chk = 1; *str; str++)
  {
    if (chk && *str == let) return 1;
    chk = *str == ' ';
  }
  return 0;
}

int ok_name(char *name)
{
  return (name
    && *name
    && *name != LOOKUP_TOKEN
    && *name != NUMBER_TOKEN
    && *name != EXIT_DELIMITER
    && !index(name, ARG_DELIMITER)
    && !index(name, AND_TOKEN)
    && !index(name, OR_TOKEN)
    && !word_start(name, NOT_TOKEN)
    && string_compare(name, "me")
    && string_compare(name, "home")
    && string_compare(name, "here"));
}

int ok_player_name(char *name, dbref player)
{
  char *scan;
  dbref found;

  if(!ok_name(name) || strlen(name) > PLAYER_NAME_LIMIT) return 0;

  for(scan = name; *scan; scan++)
  {
    if(!(isprint(*scan) && !isspace(*scan))) return 0;
  }

  /* lookup name to avoid conflicts */
  found = lookup_player(name);
  return ((found == NOTHING) || (player == found));
}

int ok_password(char *password)
{
  char *scan;

  if(*password == '\0') return 0;

  for(scan = password; *scan; scan++)
  {
    if(!(isprint(*scan) && !isspace(*scan))) return 0;
  }

  return 1;
}
