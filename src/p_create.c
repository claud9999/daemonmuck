#include "prims.h"
#include "db.h"
#include "money.h"

/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern int p_result;
extern int p_nargs;
extern dbref p_ref;
char buffer[500];

#ifdef COPYOBJ
/****************************************
 * copyobj ( d -- d ) - copy an object
 ****************************************/
void prims_copyobj (__P_PROTO)
{
  dbref newobj;
  if (!Builder(fr->euid) && !fr->wizard && !(FLAGS(program) & BUILDER))
    abort_interp ("You're not a builder.");
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  p_ref = p_oper1->data.objref;
  if (Typeof(p_ref) != TYPE_THING) abort_interp("Invalid object type.");
  if (!fr->wizard)
  {
      if (!payfor(fr->euid, OBJECT_DEPOSIT(DBFETCH(p_ref)->pennies)))
        snprintf(buffer, BUFFER_LEN, "You don't have enough %s.", PL_MONEY);
        abort_interp(buffer);   /* defined in money.h */
  }

  newobj = new_object();
  *DBFETCH(newobj)= *DBFETCH(p_ref);
  DBSTORE(newobj, desc, dup_string(GET_DESC(p_ref)));
  DBSTORE(newobj, name, dup_string(NAME(p_ref)));
  DBSTORE(newobj, succ, dup_string(GET_SUCC(p_ref)));
  DBSTORE(newobj, fail, dup_string(GET_FAIL(p_ref)));
  DBSTORE(newobj, drop, dup_string(GET_DROP(p_ref)));
  DBSTORE(newobj, osucc, dup_string(GET_OSUCC(p_ref)));
  DBSTORE(newobj, ofail, dup_string(GET_OFAIL(p_ref)));
  DBSTORE(newobj, odrop, dup_string(GET_ODROP(p_ref)));
  copy_prop(p_ref, newobj, access_rights(player, p_ref, program));
  DBSTORE(newobj, key, copy_bool(DBFETCH(p_ref)->key));
  add_backlocks_parse(newobj, DBFETCH(newobj)->key);
  DBSTORE(newobj, exits, NOTHING);

  add_ownerlist(newobj);
  add_backlinks(newobj);

  moveto(newobj, fr->euid);
  DBDIRTY(newobj);

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &newobj);
}
#endif /* COPYOBJ */

int prims_recycle_checkcallers (dbref_list *drl, dbref object)
{
  for (;drl;drl = drl->next)
  {
    if (drl->object == object) return 1;
  }
  return 0;
}

/****************************************
 * recycle ( d -- ) recycles an object
 ****************************************/
void prims_recycle (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid arguement.");
  if (OWNER(p_oper1->data.objref) != fr->euid)
    abort_interp("Permission denied.");

  p_ref = p_oper1->data.objref;

  if (Typeof(p_ref) == program) abort_interp("Can't recycle this program.");
  if (prims_recycle_checkcallers (fr->caller, p_ref))
    abort_interp("Can't recycle a caller.");

  switch (Typeof(p_ref))
  {
    case TYPE_ROOM:
      if (p_ref == PLAYER_START || p_ref == GLOBAL_ENVIRONMENT)
        abort_interp("Can't recycle that room.");
      break;
    case TYPE_PLAYER: abort_interp("Can't recycle players.");
    case TYPE_GARBAGE: abort_interp("Can't recycle garbage.");
  }
  recycle (player, p_ref);

  CLEAR(p_oper1);
}

/*NEEDS REWRITING!*/
void prims_kill (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!p_oper1->type != PROG_INTEGER) abort_interp("Invalid arguement.");
  CLEAR(p_oper1);
}

/****************************************
 * create ( s i -- d ) - create a new THING
 ****************************************/
void prims_create (__P_PROTO)
{
  if (!Builder(fr->euid) && !fr->wizard && !(FLAGS(program) & BUILDER))
    abort_interp ("You're not a builder.");

  CHECKOP(2);
  p_oper2 = POP();
  p_oper1 = POP();

  if (p_oper1->type != PROG_STRING) abort_interp("Non-string arguement. (1)");
  if (p_oper2->type != PROG_INTEGER) abort_interp("Non-integer arguement. (2)");

  if ((*(p_oper1->data.string) == '\0') ||
    !ok_name(p_oper1->data.string))
    abort_interp ("That's a silly name for an object!");

  p_result = (p_oper2->data.number < OBJECT_COST) ?
    OBJECT_COST :
    p_oper2->data.number;

  if (!fr->wizard && !payfor(fr->euid, p_result)) {
     snprintf(buffer, BUFFER_LEN, "Not enough %s", PL_MONEY);
     abort_interp(buffer);   /* defined in money.h */
  }

  p_ref = new_object ();

  DBSTORE(p_ref, flags, TYPE_THING);
  DBSTORE(p_ref, name, dup_string(p_oper1->data.string));
  DBSTORE(p_ref, location, fr->euid);
  DBSTORE(p_ref, owner, OWNER(fr->euid));
  add_ownerlist(p_ref);
  DBSTORE(p_ref, link, fr->euid);
  add_backlinks(p_ref);
  DBSTORE(p_ref, pennies,
    (OBJECT_ENDOWMENT(p_result) > MAX_OBJECT_ENDOWMENT) ?
    MAX_OBJECT_ENDOWMENT :
    OBJECT_ENDOWMENT(p_result));
  DBSTORE(p_ref, exits, NOTHING);

  PUSH(p_ref, DBFETCH(fr->euid)->contents);
  DBDIRTY(p_ref);
  DBDIRTY(fr->euid);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

/****************************************
 * dig ( s d -- d d ) - creates a new ROOM
 ****************************************/
void prims_dig (__P_PROTO)
{
  dbref parent;
  if (!Builder(fr->euid) && !fr->wizard && !(FLAGS(program) & BUILDER))
    abort_interp ("You're not a builder.");
  CHECKOP(2);
  p_oper2 = POP();
  p_oper1 = POP();

  if (p_oper1->type != PROG_STRING) abort_interp("Non-string argument. (1)");
  if (p_oper2->type != PROG_OBJECT) abort_interp("Non-object arguement. (2)");
  if (p_oper2->data.objref == NOTHING)
    p_oper2->data.objref = DBFETCH(DBFETCH(fr->euid)->location)->location;
  if (p_oper2->data.objref == NOTHING)
    p_oper2->data.objref = DBFETCH(fr->euid)->location;

  if (!valid_object(p_oper2)) abort_interp("Invalid object. (2)");

  parent = p_oper2->data.objref;
  if (Typeof(parent) != TYPE_ROOM) abort_interp ("Parent not a room.");
  if (!can_link_to(fr->euid, TYPE_ROOM, parent)) parent = GLOBAL_ENVIRONMENT;
  if ((*(p_oper1->data.string) == '\0') ||
    !ok_name(p_oper1->data.string))
    abort_interp ("That's a silly name for a room!");
  if (!fr->wizard && !payfor(fr->euid, ROOM_COST)) {
     snprintf(buffer, BUFFER_LEN, "Not enough %s", PL_MONEY);
     abort_interp(buffer);   /* defined in money.h */
   }

  p_ref = new_object();
  FLAGS(p_ref) = TYPE_ROOM | (FLAGS(fr->euid) & JUMP_OK);
  DBSTORE(p_ref, name, dup_string(p_oper1->data.string));
  DBSTORE(p_ref, location, parent);
  DBSTORE(p_ref, owner, OWNER(fr->euid));
  add_ownerlist(p_ref);
  DBSTORE(p_ref, exits, NOTHING);
  DBSTORE(p_ref, link, NOTHING);
  PUSH(p_ref, DBFETCH(parent)->contents);
  DBDIRTY(p_ref);
  DBDIRTY(parent);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
  push(arg, top, PROG_OBJECT, MIPSCAST &parent);
}

/****************************************
 * open ( s d -- d d ) - creates a new EXIT
 ****************************************/
void prims_open (__P_PROTO)
{
  if (!Builder(fr->euid) && !fr->wizard && !(FLAGS(program) & BUILDER))
    abort_interp ("You're not a builder.");

  CHECKOP(2);
  p_oper2 = POP();
  p_oper1 = POP();
  if (!valid_object(p_oper2)) abort_interp("Invalid object. (2)");
  if (p_oper1->type != PROG_STRING) abort_interp("Non-string argument. (1)");
  if (!ok_name(p_oper1->data.string)) abort_interp ("Illegal exit name.");
  if (!fr->wizard && (fr->euid != OWNER(p_oper2->data.objref)))
    abort_interp ("Permission denied.");
  if (!fr->wizard && !payfor(fr->euid, EXIT_COST)) {
     snprintf(buffer, BUFFER_LEN, "Not enough %s", PL_MONEY);
     abort_interp(buffer);   /* defined in money.h */
   }

  p_ref = new_object ();
  DBSTORE(p_ref, name, dup_string(p_oper1->data.string));
  DBSTORE(p_ref, location, NOTHING);
  DBSTORE(p_ref, sp.exit.ndest, 0);
  DBSTORE(p_ref, sp.exit.dest, NULL);
  DBSTORE(p_ref, owner, OWNER(fr->euid));
  add_ownerlist(p_ref);
  FLAGS(p_ref) = TYPE_EXIT;
  moveto(p_ref, p_oper2->data.objref);
  DBDIRTY(p_ref);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

/****************************************
 * unlink ( d -- ) unlinks an EXIT
 ****************************************/
void prims_unlink (__P_PROTO)
{
  if (!Builder(fr->euid) && !fr->wizard && !(FLAGS(program) & BUILDER))
    abort_interp ("You're not a builder.");
  CHECKOP(1);
  p_oper1 = POP();

  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  p_ref = p_oper1->data.objref;
  if (!fr->wizard && (fr->euid != OWNER(p_ref)))
    abort_interp("Permission denied.");

  switch (Typeof(p_ref))
  {
    case TYPE_EXIT:
      remove_backlinks(p_ref);
      if(DBFETCH(p_ref)->sp.exit.ndest != 0)
      {
        if (!fr->wizard)
          DBFETCH(OWNER(p_ref))->pennies += LINK_COST;
        DBDIRTY(OWNER(p_ref));
      }
      DBSTORE(p_ref, sp.exit.ndest, 0);
      if (DBFETCH(p_ref)->sp.exit.dest)
      {
        free ((void *)DBFETCH(p_ref)->sp.exit.dest);
        DBSTORE(p_ref, sp.exit.dest, NULL);
      }
      break;
    case TYPE_ROOM:
      remove_backlinks(p_ref);
      DBSTORE(p_ref, link, NOTHING);
      break;
    default:
      abort_interp("Invalid object type");
  }
  CLEAR(p_oper1);
}

/** addlink_check -> checks to make sure there isn't already a link to
    something other than an exit **/
int prims_addlink_check (dbref exit)
{
  int i;
  for (i = 0; i < DBFETCH(exit)->sp.exit.ndest; i++)
  {
    switch (Typeof(DBFETCH(exit)->sp.exit.dest[i]))
    {
      case TYPE_PLAYER:
      case TYPE_ROOM:
      case TYPE_PROGRAM:
      return 1;
    }
  }
  return 0;
}

/** addlink_sub -> checks to see if a link is valid, and links it **/
int prims_addlink_sub (frame *fr, dbref exit, dbref destination)
{
#ifndef TELEPORT_TO_PLAYER
  if (Typeof(destination) == TYPE_PLAYER)
    return 0;
#endif /* TELEPORT_TO_PLAYER */
  if (DBFETCH(exit)->sp.exit.ndest == MAX_LINKS) return 0;
  if (!can_link(fr->euid, exit)) return 0;
  if (!can_link_to(fr->euid, TYPE_EXIT, destination)) return 0;
  switch (Typeof(destination))
  {
    case TYPE_PLAYER:
    case TYPE_ROOM:
    case TYPE_PROGRAM:
      if (prims_addlink_check(exit)) return 0;
      break;
    case TYPE_EXIT:
      if (exit_loop_check(exit, destination)) return 0;
  }

  /* chown it if it isna' linked to anything */
  if (!DBFETCH(exit)->sp.exit.ndest) db_chown(exit, fr->player);

  /* increment the # of destinations */
  DBSTORE(exit, sp.exit.ndest, DBFETCH(exit)->sp.exit.ndest + 1);

  /* realloc if there's more than one dest, malloc new memory if there is
   * no previos destinations.
   */
  if (DBFETCH(exit)->sp.exit.ndest > 1)
  { DBSTORE(exit, sp.exit.dest, (dbref *)realloc(DBFETCH(exit)->sp.exit.dest,
        sizeof(dbref) * DBFETCH(exit)->sp.exit.ndest));
  } else
    DBSTORE(exit, sp.exit.dest, (dbref *)malloc(sizeof(dbref)));

  /* shove the dest in the array. */
  DBSTORE(exit, sp.exit.dest[DBFETCH(exit)->sp.exit.ndest - 1], destination);
  return 1;
}

/****************************************
 * addlink ( d d -- ) - adds a link to an OBJECT
 ****************************************/
void prims_addlink (__P_PROTO)
{
  if (!Builder(fr->euid) && !fr->wizard && !(FLAGS(program) & BUILDER))
    abort_interp ("You're not a builder.");
  CHECKOP(2);
  p_oper2 = POP();
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object. (1)");
  if (!valid_object(p_oper2)) abort_interp("Invalid object. (2)");

  switch (Typeof(p_oper1->data.objref))
  {
    case TYPE_EXIT:
      if (fr->euid != OWNER(p_oper1->data.objref))
        abort_interp("Permission denied.");

      remove_backlinks(p_oper1->data.objref);
      if (!prims_addlink_sub (fr, p_oper1->data.objref, p_oper2->data.objref))
        abort_interp ("Can't link.");
      add_backlinks(p_oper1->data.objref);
      break;
    case TYPE_PLAYER:
    case TYPE_THING:
    case TYPE_PROGRAM:
    case TYPE_ROOM:
      if (!controls(fr->euid, p_oper1->data.objref) ||
        !can_link_to(fr->euid, Typeof(p_oper1->data.objref),
	  p_oper2->data.objref))
        abort_interp("Permission denied.");
      remove_backlinks(p_oper1->data.objref);
      DBSTORE(p_oper1->data.objref, link, p_oper2->data.objref);
      add_backlinks(p_oper1->data.objref);
  }

  CLEAR(p_oper1);
  CLEAR(p_oper2);
}
