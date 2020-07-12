#include "copyright.h"
#include "config.h"
#include "db.h"
#include "params.h"
#include "externs.h"

#define BUFFER_LEN 1024
extern char *uppercase, *lowercase;
char *wptr[10];


int can_link_to(dbref foo,object_flag_type bar,dbref junk){return 1;}
int controls(dbref foo,dbref junk){return 1;}

/* extern char *unparse_object(dbref,dbref); */
extern char *unparse_boolexp(dbref, boolexp *);

void recycle(dbref foo, dbref bar){}

int notify(dbref i, dbref j, char *k)
{
return 1;
}

void bitch(dbref player, char *string)
{ printf("%s\n", uncompress(string));}

void violate(dbref i, char *s)
{
dbref content, exit, player, thing;
char buf[BUFFER_LEN];

player = (dbref) 1;
thing = i;

  printf("Object #%ld violates %s rules!\n", i, s);
/*  db_write_object(stdout, i);
*/

  switch(Typeof(thing))

  {
    case TYPE_ROOM:
      sprintf(buf, "%s  Owner: %s  Parent: ", unparse_object(player, thing),
        NAME(OWNER(thing)));
      strcat(buf, unparse_object(player, DBFETCH(thing)->location));
      break;
    case TYPE_THING:
      sprintf(buf, "%s  Owner: %s  Value: %d", unparse_object(player, thing),
        NAME(OWNER(thing)), DBFETCH(thing)->pennies);
      break;
    case TYPE_PLAYER:
      sprintf(buf, "%s  Coconuts: %d", unparse_object(player, thing),
        DBFETCH(thing)->pennies);
      break;
    case TYPE_EXIT:
    case TYPE_PROGRAM:
      sprintf(buf, "%s  Owner: %s", unparse_object(player, thing),
        NAME(OWNER(thing)));
      break;
    case TYPE_GARBAGE:
      strcpy(buf, unparse_object(player, thing));
      break;
  }
  bitch(player, buf);
  if(DBFETCH(thing)->desc) bitch(player, DBFETCH(thing)->desc);
  sprintf(buf, "Key: %s", unparse_boolexp(player, DBFETCH(thing)->key));
  bitch(player, buf);

  if(DBFETCH(thing)->fail)
  {
    sprintf(buf, "Fail: %s", DBFETCH(thing)->fail);
    bitch(player, buf);
  }

  if(DBFETCH(thing)->succ)
  {
    sprintf(buf, "Success: %s", DBFETCH(thing)->succ);
    bitch(player, buf);
  }

  if (DBFETCH(thing)->drop)
  {
    sprintf(buf, "Drop: %s", DBFETCH(thing)->drop);
    bitch(player, buf);
  }

  if(DBFETCH(thing)->ofail)
  {
    sprintf(buf, "Ofail: %s", DBFETCH(thing)->ofail);
    bitch(player, buf);
  }

  if(DBFETCH(thing)->osucc)
  {
    sprintf(buf, "Osuccess: %s", DBFETCH(thing)->osucc);
    bitch(player, buf);
  }

  if (DBFETCH(thing)->odrop)
  {
    sprintf(buf, "Odrop: %s", DBFETCH(thing)->odrop);
    bitch(player, buf);
  }

  switch(Typeof(thing))
  {
    case TYPE_THING:
      /* print link */
      sprintf(buf, "Home: %s",
        unparse_object(player, DBFETCH(thing)->link)); /* home */
      bitch(player, buf);
      /* print location if player can link to it */
      if(DBFETCH(thing)->location != NOTHING &&
        (controls(player, DBFETCH(thing)->location) ||
        can_link_to(player, NOTYPE, DBFETCH(thing)->location)))
      {
        sprintf(buf, "Location: %s",
          unparse_object(player, DBFETCH(thing)->location));
        bitch(player, buf);
      }
      break;
    case TYPE_PLAYER:

      /* print home */
      sprintf(buf, "Home: %s",
        unparse_object(player, DBFETCH(thing)->link)); /* home */
      bitch(player, buf);

      /* print location if player can link to it */
      if(DBFETCH(thing)->location != NOTHING &&
        (controls(player, DBFETCH(thing)->location) ||
        can_link_to(player, NOTYPE, DBFETCH(thing)->location)))
      {
        sprintf(buf, "Location: %s",
          unparse_object(player, DBFETCH(thing)->location));
        bitch(player, buf);
      }
      break;
    case TYPE_EXIT:
      if (DBFETCH(thing)->location != NOTHING)
      {
        sprintf(buf, "Source: %s",
          unparse_object(player, DBFETCH(thing)->location));
        bitch(player, buf);
      }
      /* print destinations */
      if (DBFETCH(thing)->sp.exit.ndest == 0) break;
      for (i = 0; i < DBFETCH(thing)->sp.exit.ndest; i++)
      {
        switch( (DBFETCH(thing)->sp.exit.dest)[i])
        {
          case NOTHING:
            break;
          case HOME:
            bitch(player, "Destination: *HOME*");
            break;
          default:
            sprintf(buf, "Destination: %s",
              unparse_object(player, (DBFETCH(thing)->sp.exit.dest)[i]));
            bitch(player, buf);
            break;
        }
      }
      break;
    case TYPE_PROGRAM:
      if (DBFETCH(thing)->sp.program.siz)
      {
        sprintf(buf, "Program compiled size: %d",
          DBFETCH(thing)->sp.program.siz);
        bitch(player, buf);
      }
      else bitch(player, "Program not compiled");

      /* print location if player can link to it */
      if(DBFETCH(thing)->location != NOTHING &&
        (controls(player, DBFETCH(thing)->location) ||
        can_link_to(player, NOTYPE, DBFETCH(thing)->location)))
      {
        sprintf(buf, "Location: %s",
          unparse_object(player, DBFETCH(thing)->location));
        bitch(player, buf);
      }
      break;
  }

  bitch(player, "Contents:");
  if(DBFETCH(thing)->contents != NOTHING)
  {
    DOLIST(content, DBFETCH(thing)->contents)
    {
      bitch(player, unparse_object(player, content));
    }
  }

  bitch (player, "Actions/exits:");
  switch(Typeof(thing))
  {
    case TYPE_ROOM:
      if(DBFETCH(thing)->exits != NOTHING)
      {
        DOLIST(exit, DBFETCH(thing)->exits)
        {
          bitch(player, unparse_object(player, exit));
        }
      }
      break;
    case TYPE_THING:
      if(DBFETCH(thing)->exits != NOTHING)
      {
        DOLIST(exit, DBFETCH(thing)->exits)
        {
          bitch(player, unparse_object(player, exit));
        }
      }
      break;
    case TYPE_PLAYER:
      if(DBFETCH(thing)->exits != NOTHING)
      {
        DOLIST(exit, DBFETCH(thing)->exits)
        {
          bitch(player, unparse_object(player, exit));
        }
      }
      break;
  }
  bitch(player,"\n");
}

void check_common(dbref obj)
{
  int i;

  /* check location */
  if (DBFETCH(obj)->location >= db_top) violate(obj, "location");
  if ((Typeof (DBFETCH(obj)->location) != TYPE_ROOM) &&
      (Typeof (DBFETCH(obj)->location) != TYPE_PLAYER) &&
      (Typeof(obj) != TYPE_EXIT &&
            Typeof (DBFETCH(obj)->location) != TYPE_THING) &&
      /*garbage and the global environment may have a loc #-1 */
      (Typeof(obj) != TYPE_GARBAGE &&
	  DBFETCH(obj)->location != NOTHING ) &&
      (obj != GLOBAL_ENVIRONMENT &&
	  DBFETCH(obj)->location != NOTHING )
	  )
    violate(obj, "loc_type");

  /* check contents */
  for (i = DBFETCH(obj)->contents;
       i < db_top  &&  i != NOTHING;
       i =DBFETCH(i)->next)
    if (DBFETCH(i)->location != obj) violate(obj, "contents list");
  if (i != NOTHING) violate(obj, "contents rules");
}

void check_room(dbref obj)
{
  dbref  i;

  /*check exit type, etc */
  for (i = DBFETCH(obj)->exits;
       i != NOTHING && i < db_top;
       i = DBFETCH(i)->next)
    if (Typeof(i) != TYPE_EXIT) violate(obj,"exit_type");
  if (i != NOTHING)
    violate(obj, "exits");

  if (OWNER(obj) >= db_top || (Typeof(OWNER(obj)) != TYPE_PLAYER))
    violate(obj, "owner");
}

void check_thing(dbref obj)
{
  dbref i;

  if (DBFETCH(obj)->link >= db_top
    || ((Typeof(DBFETCH(obj)->link) != TYPE_ROOM)
    && (Typeof(DBFETCH(obj)->link) != TYPE_PLAYER)))
    violate(obj, "link");

  for (i = DBFETCH(obj)->exits;
       i < db_top && i != NOTHING;
       i = DBFETCH(i)->next)
    {
    if (Typeof(i) != TYPE_EXIT) violate(obj,"exit_type");
    if (DBFETCH(i)->location != obj) violate(obj,"exit_list");
    }

  if (i != NOTHING)
    violate(obj, "exits");

  if (OWNER(obj) >= db_top || Typeof(OWNER(obj)) != TYPE_PLAYER)
    violate(obj, "owner");
}

void
check_exit(dbref obj)
{
  int      i;

  for (i = 0; i < DBFETCH(obj)->sp.exit.ndest; i++)
    if ((DBFETCH(obj)->sp.exit.dest)[i] >= db_top) violate(obj, "destination");

  if (OWNER(obj) >= db_top || Typeof(OWNER(obj)) != TYPE_PLAYER)
    violate(obj, "owner");
}

void
check_player(dbref obj)
{
  dbref        i;

  if (DBFETCH(obj)->link >= db_top
     || Typeof(DBFETCH(obj)->link) != TYPE_ROOM)
    violate(obj, "link");

  for (i = DBFETCH(obj)->exits;
       i < db_top && i != NOTHING;
       i = DBFETCH(i)->next)
    {
      if (Typeof(i) != TYPE_EXIT) violate(obj,"exit_type");
      if (DBFETCH(i)->location != obj) violate(obj,"exit_list");
    }
  if (i != NOTHING) violate(obj, "exits");
}

void
main(int argc, char **argv)
{
  int        i;

  db_read(stdin);

  printf("dbtop = %ld\n", db_top);

  for (i = 0; i < db_top; i++)
    {
      fprintf(stderr, "Checking object %ld...\n", i);
      check_common(i);
      switch( db[i].flags & TYPE_MASK )
        {
        case TYPE_ROOM:
          check_room(i);
          break;
        case TYPE_THING:
          check_thing(i);
          break;
        case TYPE_EXIT:
          check_exit(i);
          break;
        case TYPE_PLAYER:
          check_player(i);
          break;
        case TYPE_PROGRAM:
        case TYPE_GARBAGE:
          break;
        default:
          violate(i, "type");
          break;
        }
    }
  printf("\n");
}

/* dummy compiler */
void
do_compile(dbref p, dbref pr)
{
}
macrotable *new_macro(char *name, char *definition, dbref player)
{
}
void init_primitives()
{
}
void clear_primitives()
{
}
void clear_players()
{
}
void add_player(dbref i)
{
}

char *alloc_string(char *string)
{
  char *s;

  /* NULL, "" -> NULL */
  if(string == 0 || *string == '\0') return 0;

  if((s = (char *) malloc(strlen(string)+1)) == 0) {
    abort();
  }
  strcpy(s, string);
  return s;
}
