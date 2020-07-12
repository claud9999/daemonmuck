#include "prims.h"
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
extern char p_buf[BUFFER_LEN];
extern int p_result;
extern int p_nargs;
extern dbref p_ref;
extern dbref_list *p_drl;
extern int shutdown_flag;
extern void fork_and_dump();

void prims_moveto(__P_PROTO)
{
  dbref victim, destination, matchroom;
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if ((!valid_object(p_oper1) && !is_home(p_oper1)) || !valid_object(p_oper2))
    abort_interp("Non-object argument.");

  victim = p_oper2->data.objref;
  destination = p_oper1->data.objref;
  matchroom = NOTHING;
  if (destination == HOME) destination = DBFETCH(victim)->link;

  if ((Typeof(victim) == TYPE_THING) || (Typeof(victim) == TYPE_PROGRAM))
  {
    if (Typeof(destination) == TYPE_ROOM
      && (DBFETCH(destination)->link != NOTHING)
      && !(FLAGS(destination) & STICKY))
      destination = DBFETCH(destination)->link;
  }
   if(Typeof(victim) == TYPE_ROOM && Typeof(destination) != TYPE_ROOM)
       abort_interp("Permission denied.");

  if (fr->wizard ||
    ((can_link_to(fr->euid, NOTYPE, destination)) ||
    ((FLAGS(destination) & JUMP_OK) &&
    controls(fr->euid, victim))))
    enter_room(victim, destination, DBFETCH(victim)->location);
  else abort_interp("Permission denied.");
}

void prims_contents(__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid argument type.");

  p_ref = DBFETCH(p_oper1->data.objref)->contents;

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_exits(__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");

  p_ref = p_oper1->data.objref;
  if (!fr->wizard && !permissions(fr->euid, p_ref))
    abort_interp("Permission denied.");
  p_ref = DBFETCH(p_ref)->exits;

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_next(__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");

  p_ref = DBFETCH(p_oper1->data.objref)->next;

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_match (__P_PROTO)
{
  match_data md;
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type != PROG_STRING) abort_interp("Non-string argument.");
  if (!p_oper1->data.string) abort_interp("Empty string argument.");

  init_match(fr->euid, p_oper1->data.string, NOTYPE, &md);
  match_all_exits(&md);
  match_neighbor(&md);
  match_possession(&md);
  match_me(&md);
  match_here(&md);
  match_home(&md);
  if (fr->wizard)
  {
    match_absolute(&md);
    match_player(&md);
  }
  p_ref = match_result(&md);

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_rmatch (__P_PROTO)
{
  match_data md;
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (p_oper1->type != PROG_STRING) abort_interp("Invalid argument (2)");
  if (p_oper2->type != PROG_OBJECT
    || Typeof(p_oper2->data.objref) == TYPE_PROGRAM
    || Typeof(p_oper2->data.objref) == TYPE_EXIT)
    abort_interp("Invalid argument (1)");

  init_match(fr->euid, DoNullInd(p_oper1->data.string), TYPE_THING, &md);
  match_rmatch(p_oper2->data.objref, &md);
  p_ref = match_result(&md);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_owner (__P_PROTO)
{
  frame *fr_tmp;
  CHECKOP(1);
  p_oper1 = POP();
  if (p_oper1->type == PROG_INTEGER)
  {
    p_ref = (fr_tmp = find_frame(p_oper1->data.number)) ?
      fr_tmp->player :
      NOTHING;
  }
  else
  {
    if (!valid_object(p_oper1)) abort_interp("Invalid object.");
    p_ref = OWNER(p_oper1->data.objref);
    CLEAR(p_oper1);
  }
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_location (__P_PROTO)
{
  dbref loc;
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");

  p_ref = p_oper1->data.objref;
     loc = DBFETCH(p_ref)->location;

  push(arg, top, PROG_OBJECT, MIPSCAST &loc);
}

void prims_getlink (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  if (Typeof(p_oper1->data.objref) == TYPE_PROGRAM)
    abort_interp("Illegal object referenced.");

  switch (Typeof(p_oper1->data.objref))
  {
    case TYPE_EXIT:
      p_ref = (DBFETCH(p_oper1->data.objref)->sp.exit.ndest) ?
      (DBFETCH(p_oper1->data.objref)->sp.exit.dest)[0] : NOTHING;
      break;
    case TYPE_PLAYER:
      p_ref = DBFETCH(p_oper1->data.objref)->link;
      break;
    case TYPE_THING:
      p_ref = DBFETCH(p_oper1->data.objref)->link;
      break;
    case TYPE_ROOM:
      p_ref = DBFETCH(p_oper1->data.objref)->link;
      break;
    default:
      p_ref = NOTHING;
      break;
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}

void prims_online (__P_PROTO)
{
  descriptor_data *d;

  p_result = 0;
  for(d = descriptor_list; d; d = d->next)
  {
    if((d->connected) && (!(FLAGS(d->player) & DARK) || fr->wizard))
    {
      p_ref = d->player;
      if(*top >= STACK_SIZE) abort_interp("Stack Overflow.");
      push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
      p_result++;
    }
  }
  if(*top >= STACK_SIZE) abort_interp("Stack Overflow.");
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_db_top (__P_PROTO)
{
  p_result = db_top;
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_dbtop (__P_PROTO)
{
  p_result = db_top - 1;
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push(arg, top, PROG_OBJECT, MIPSCAST &p_result);
}

void prims_chown (__P_PROTO)
{
  dbref victim, owner;
  CHECKOP(2);
  p_oper1 = POP();
  p_oper2 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid arguement (2)");
  if (!valid_object(p_oper2)) abort_interp("Invalid arguement (1)");
  victim = p_oper2->data.objref;
  owner = p_oper1->data.objref;

  if (!fr->wizard && !(FLAGS(victim) & CHOWN_OK))
    abort_interp("Permission denied.");
  if (Typeof(victim) == TYPE_PLAYER) abort_interp("Can't chown a player.");
  if (Typeof(owner) != TYPE_PLAYER) abort_interp("Object is not a player.");
  if (!fr->wizard && (owner != player))
    abort_interp("Can't chown to anyone but yourself.");

  remove_ownerlist(victim);
  DBSTORE(victim, owner, owner);
  add_ownerlist(victim);

  CLEAR(p_oper1);
  CLEAR(p_oper2);
}

/* linkcount ( d -- i ) */
void prims_linkcount (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  if ((OWNER(player) != OWNER(p_oper1->data.objref)) && !fr->wizard)
    abort_interp("Permission denied.");

  p_ref = p_oper1->data.objref;
  switch (Typeof(p_ref))
  {
    case TYPE_EXIT:
      p_result = DBFETCH(p_oper1->data.objref)->sp.exit.ndest;
      break;
    case TYPE_ROOM:
      p_result = (DBFETCH(p_ref)->link == NOTHING) ? 0 : 1;
      break;
    case TYPE_THING:
      p_result = (DBFETCH(p_ref)->link == NOTHING) ? 0 : 1;
      break;
    case TYPE_PLAYER:
      p_result = 1;
      break;
    case TYPE_PROGRAM:
      p_result = 0;
  }

  CLEAR(p_oper1);
  push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/* getlinks ( d -- d1 ... dN N ) */
void prims_getlinks (__P_PROTO)
{
  int i;

  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  if ((OWNER(player) != OWNER(p_oper1->data.objref)) && !fr->wizard)
    abort_interp ("Permission denied.");

  p_ref = p_oper1->data.objref;
  p_result = 0;
  switch (Typeof(p_ref))
  {
    case TYPE_ROOM:
      if (DBFETCH(p_ref)->link != NOTHING)
      {
        push (arg, top, PROG_OBJECT,
	  MIPSCAST &(DBFETCH(p_ref)->link));
        p_result = 1;
      }
      break;
    case TYPE_THING:
      if (DBFETCH(p_ref)->link != NOTHING)
      {
        push (arg, top, PROG_OBJECT, MIPSCAST &(DBFETCH(p_ref)->link));
        p_result = 1;
      }
      break;
    case TYPE_EXIT:
      p_result = DBFETCH(p_ref)->sp.exit.ndest;
      for (i = 0; i < p_result; i++)
      {
        if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
        push (arg, top, PROG_OBJECT,
          MIPSCAST &((DBFETCH(p_ref)->sp.exit.dest)[i]));
      }
      break;
    case TYPE_PLAYER:
      push (arg, top, PROG_OBJECT, MIPSCAST &(DBFETCH(p_ref)->link));
      p_result = 1;
  }
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push (arg, top, PROG_INTEGER, MIPSCAST &p_result);

  CLEAR(p_oper1);
}

/* prog ( -- d ) */
void prims_prog(__P_PROTO)
{
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push (arg, top, PROG_OBJECT, MIPSCAST &program);
}

/* callers ( -- d1...dN N ) */
void prims_callers (__P_PROTO)
{
  for (p_drl = fr->caller, p_result = 0; p_drl; p_drl = p_drl->next, p_result++)
  {
    if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
    push (arg, top, PROG_OBJECT, MIPSCAST &p_drl->object);
  }
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push (arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

/* caller ( -- d ) */
void prims_caller (__P_PROTO)
{
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push (arg, top, PROG_OBJECT, MIPSCAST &(fr->player));
}

/* trig ( -- d ) */
void prims_trig (__P_PROTO)
{
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push (arg, top, PROG_OBJECT, MIPSCAST &(fr->trigger));
}

/* backlinks ( d -- d1...dN N ) */
void prims_backlinks (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  if ((OWNER(player) != OWNER(p_oper1->data.objref)) && !fr->wizard)
    abort_interp ("Permission denied.");
  for (p_drl = DBFETCH(p_oper1->data.objref)->backlinks, p_result = 0;
    p_drl; p_drl = p_drl->next, p_result++)
  {
    if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
    push (arg, top, PROG_OBJECT, MIPSCAST &p_drl->object);
  }
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
  push (arg, top, PROG_INTEGER, MIPSCAST &p_result);

  CLEAR(p_oper1);
}

/* backlocks ( d -- d1...dN N ) */
void prims_backlocks (__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  if ((OWNER(player) != OWNER(p_oper1->data.objref)) && !fr->wizard)
    abort_interp ("Permission denied.");
  for (p_drl = DBFETCH(p_oper1->data.objref)->backlocks, p_result = 0;
    p_drl; p_drl = p_drl->next, p_result++)
  {
    if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
    push (arg, top, PROG_OBJECT, MIPSCAST &p_drl->object);
  }
  if ((*top) >= STACK_SIZE) abort_interp("Stack overflow.");
    push (arg, top, PROG_INTEGER, MIPSCAST &p_result);
  CLEAR(p_oper1);
}

/* nextowned ( d -- d' ) */
void prims_nextowned(__P_PROTO)
{
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Invalid object.");
  if ((OWNER(player) != OWNER(p_oper1->data.objref)) && !fr->wizard)
    abort_interp ("Permission denied.");
  push (arg, top, PROG_OBJECT,
    MIPSCAST &(DBFETCH(p_oper1->data.objref)->nextowned));
  CLEAR(p_oper1);
}

static boolexp *key;

void prims_lock(__P_PROTO)
{
  dbref thing;
  CHECKOP(2);
  p_oper2 = POP();
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Non-object argument.");
  if (p_oper2->type != PROG_STRING) abort_interp("Non-string argument.");
  if (!fr->wizard && !permissions(fr->euid, p_ref))
    abort_interp("Permission denied.");

  thing = p_oper1->data.objref;
  strncpy(p_buf, p_oper2->data.string, strlen(p_oper2->data.string) + 1);
  key = parse_boolexp(player, p_buf);

  if(key == TRUE_BOOLEXP)
  {
       abort_interp("Invalid key.");
  }
  else
  {
    /* everything ok, do it   Taken directly from do_lock --Howard */
    remove_backlocks_parse(thing, DBFETCH(thing)->key);
    free_boolexp(DBFETCH(thing)->key);
    DBSTORE(thing, key, key);
    add_backlocks_parse(thing, DBFETCH(thing)->key);
  }

  CLEAR(p_oper1);
  CLEAR(p_oper2);
}

void prims_unlock(__P_PROTO)
{
  dbref thing;
  CHECKOP(1);
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Non-object argument.");
  if (!fr->wizard && !permissions(fr->euid, p_ref))
    abort_interp("Permission denied.");
  thing = p_oper1->data.objref;

  remove_backlocks_parse(thing, DBFETCH(thing)->key);
  free_boolexp(DBFETCH(thing)->key);
  DBSTORE(thing, key, TRUE_BOOLEXP);

  CLEAR(p_oper1);
}

void prims_dump(__P_PROTO)
{
  if (!fr->wizard) abort_interp("Permission denied.");

  fork_and_dump();
}

void prims_shutdown(__P_PROTO)
{
  if (!fr->wizard) abort_interp("Permission denied.");

  shutdown_flag = 1;
}

/* toad ( d d -- )
  d - player to toad
  d - recipient of toaded's stuff.
*/
void prims_toad(__P_PROTO)
{
  CHECKOP(2);
  p_oper2 = POP();
  p_oper1 = POP();
  if (!valid_object(p_oper1)) abort_interp("Non-object argument(1).");
  if (!valid_object(p_oper2)) abort_interp("Non-object argument(2).");
  if (Typeof(p_oper1->data.objref) != TYPE_PLAYER)
    abort_interp("Non-player argument(1).");
  if (Typeof(p_oper2->data.objref) != TYPE_PLAYER)
    abort_interp("Non-player argument(2).");

  if (!controls(fr->player, p_oper1->data.objref) ||
    !controls(fr->player, p_oper2->data.objref) ||
    (fr->player == p_oper1->data.objref))
    abort_interp("Permission denied.");
  toad(p_oper1->data.objref, p_oper2->data.objref);
  CLEAR(p_oper1);
  CLEAR(p_oper2);
}

void prims_stats(__P_PROTO)
{
    /* A WhiteFire special. :) */
    CHECKOP(1);
    p_oper1 = POP();

    if (!fr->wizard) abort_interp("Permission denied.");
    if (!valid_player(p_oper1) && (p_oper1->data.objref != NOTHING))
	abort_interp("non-player argument (1)");

    p_ref = p_oper1->data.objref;
    CLEAR(p_oper1);
    {
	dbref   i;
	int     rooms, exits, things, players, programs, garbage;

	/* tmp, ref */
	rooms = exits = things = players = programs = garbage = 0;
	for (i = 0; i < db_top; i++) {
	    if (p_ref == NOTHING || OWNER(i) == p_ref) {
		switch (Typeof(i)) {
		    case TYPE_ROOM:
			rooms++;
			break;
		    case TYPE_EXIT:
			exits++;
			break;
		    case TYPE_THING:
			things++;
			break;
		    case TYPE_PLAYER:
			players++;
			break;
		    case TYPE_PROGRAM:
			programs++;
			break;
		    case TYPE_GARBAGE:
			garbage++;
			break;
		}
	    }
	}
	p_result = rooms + exits + things + players + programs + garbage;
        push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
        push(arg, top, PROG_INTEGER, MIPSCAST &rooms);
        push(arg, top, PROG_INTEGER, MIPSCAST &exits);
        push(arg, top, PROG_INTEGER, MIPSCAST &things);
        push(arg, top, PROG_INTEGER, MIPSCAST &programs);
        push(arg, top, PROG_INTEGER, MIPSCAST &players);
        push(arg, top, PROG_INTEGER, MIPSCAST &garbage);
        if(*top >= STACK_SIZE) abort_interp("Stack Overflow.");
    }
}

void prims_part_pmatch(__P_PROTO)
{
    CHECKOP(1);
    p_oper1 = POP();

    if (p_oper1->type != PROG_STRING) abort_interp("Non-string argument.");
    if (!p_oper1->data.string) abort_interp("Empty string argument.");
    p_ref = partial_pmatch(p_oper1->data.string);
    CLEAR(p_oper1);
    push(arg, top, PROG_OBJECT, MIPSCAST &p_ref);
}
