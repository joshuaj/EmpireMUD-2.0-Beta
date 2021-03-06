/* ************************************************************************
*   File: act.movement.c                                  EmpireMUD 2.0b3 *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2015                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <math.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "vnums.h"
#include "dg_scripts.h"

/**
* Contents:
*   Helpers
*   Move Validators
*   Move System
*   Commands
*/

// external vars
extern const char *dirs[];
extern const int rev_dir[];
extern const char *from_dir[];

// move vars
#define MOVE_NORMAL		0		/* Normal move message		*/
#define MOVE_LEAD		1		/* Leading message			*/
#define MOVE_FOLLOW		2		/* Follower message			*/
#define MOVE_CART		3		/* Cart message				*/
#define MOVE_EARTHMELD	4
#define MOVE_SWIM		5	// swim skill

#define WATER_SECT(room)		(ROOM_SECT_FLAGGED((room), SECTF_FRESH_WATER | SECTF_OCEAN) || ROOM_BLD_FLAGGED((room), BLD_NEED_BOAT) || RMT_FLAGGED((room), RMT_NEED_BOAT) || (IS_WATER_BUILDING(room) && !IS_COMPLETE(room) && SECT_FLAGGED(ROOM_ORIGINAL_SECT(room), SECTF_FRESH_WATER | SECTF_OCEAN)))
#define DEEP_WATER_SECT(room)	(ROOM_SECT_FLAGGED((room), SECTF_OCEAN))


// door vars
#define NEED_OPEN	BIT(0)
#define NEED_CLOSED	BIT(1)

const char *cmd_door[] = {
	"open",
	"close",
};

const int flags_door[] = {
	NEED_CLOSED,
	NEED_OPEN,
};


 //////////////////////////////////////////////////////////////////////////////
//// HELPERS /////////////////////////////////////////////////////////////////

/**
* @param char_data *ch the person leaving tracks
* @param room_data *room the location of the tracks
* @param byte dir the direction the person went
*/
void add_tracks(char_data *ch, room_data *room, byte dir) {
	struct track_data *track;
	
	if (!IS_IMMORTAL(ch) && !ROOM_SECT_FLAGGED(room, SECTF_FRESH_WATER | SECTF_FRESH_WATER)) {
		if (!IS_NPC(ch) && HAS_ABILITY(ch, ABIL_NO_TRACE) && !COMPLEX_DATA(room) && !IS_ROAD(room)) {
			gain_ability_exp(ch, ABIL_NO_TRACE, 5);
		}
		else if (!IS_NPC(ch) && HAS_ABILITY(ch, ABIL_UNSEEN_PASSING) && (COMPLEX_DATA(room) || IS_ROAD(room))) {
			gain_ability_exp(ch, ABIL_UNSEEN_PASSING, 5);
		}
		else {
			CREATE(track, struct track_data, 1);
		
			track->timestamp = time(0);
			track->dir = dir;
		
			if (IS_NPC(ch)) {
				track->mob_num = GET_MOB_VNUM(ch);
				track->player_id = NOTHING;
			}
			else {
				track->mob_num = NOTHING;
				track->player_id = GET_IDNUM(ch);
			}
			
			track->next = ROOM_TRACKS(room);
			ROOM_TRACKS(room) = track;
		}
	}
}


/**
* Determines if a character is allowed to enter a room via portal or walking.
*
* @param char_data *ch The player trying to enter.
* @parma room_data *room The target location.
*/
bool can_enter_room(char_data *ch, room_data *room) {
	extern bool can_enter_instance(char_data *ch, struct instance_data *inst);
	extern struct instance_data *find_instance_by_room(room_data *room, bool check_homeroom);
	
	struct instance_data *inst;
	
	// always
	if (IS_IMMORTAL(ch) || IS_NPC(ch)) {
		return TRUE;
	}
	
	// player limit
	if (IS_ADVENTURE_ROOM(room) && (inst = find_instance_by_room(room, FALSE))) {
		// only if not already in there
		if (!IS_ADVENTURE_ROOM(IN_ROOM(ch)) || find_instance_by_room(IN_ROOM(ch), FALSE) != inst) {
			if (!can_enter_instance(ch, inst)) {
				return FALSE;
			}
		}
	}
	
	return TRUE;
}


void do_doorcmd(char_data *ch, obj_data *obj, int door, int scmd) {
	char lbuf[MAX_STRING_LENGTH];
	room_data *other_room = NULL;
	struct room_direction_data *ex = !obj ? find_exit(IN_ROOM(ch), door) : NULL, *back = NULL;
	
	#define TOGGLE_DOOR(ex, obj)	((obj) ? \
		(TOGGLE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) : \
		(TOGGLE_BIT((ex)->exit_info, EX_CLOSED)))
	#define OPEN_DOOR(ex, obj)	((obj) ? \
		(REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) : \
		(REMOVE_BIT((ex)->exit_info, EX_CLOSED)))
	#define CLOSE_DOOR(ex, obj)	((obj) ? \
		(SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) : \
		(SET_BIT((ex)->exit_info, EX_CLOSED)))

	if (!door_mtrigger(ch, scmd, door)) {
		return;
	}
	if (!door_wtrigger(ch, scmd, door)) {
		return;
	}

	sprintf(lbuf, "$n %ss ", cmd_door[scmd]);
	
	// attempt to detect the reverse door, if there is one
	if (!obj && COMPLEX_DATA(IN_ROOM(ch)) && ex) {
		other_room = ex->room_ptr;
		if (other_room) {
			back = find_exit(other_room, rev_dir[door]);
			if (back != NULL && back->room_ptr != IN_ROOM(ch)) {
				back = NULL;
			}
		}
	}

	// this room/obj's door
	if (ex || obj) {
		TOGGLE_DOOR(ex, obj);
	}
	
	// toggling the door on the other side sometimes resulted in doors being permanently out-of-sync
	if (ex && back) {
		if (IS_SET(ex->exit_info, EX_CLOSED)) {
			CLOSE_DOOR(back, obj);
		}
		else {
			OPEN_DOOR(back, obj);
		}
	}
	
	send_config_msg(ch, "ok_string");

	/* Notify the room */
	sprintf(lbuf + strlen(lbuf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" : (ex->keyword ? "$F" : "door"));
	if (!obj || IN_ROOM(obj)) {
		act(lbuf, FALSE, ch, obj, obj ? NULL : ex->keyword, TO_ROOM);
	}

	/* Notify the other room */
	if (other_room && back) {
		sprintf(lbuf, "The %s is %s%s from the other side.", (back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd], (scmd == SCMD_CLOSE) ? "d" : "ed");
		if (ROOM_PEOPLE(other_room)) {
			act(buf, FALSE, ROOM_PEOPLE(other_room), 0, 0, TO_ROOM);
			act(buf, FALSE, ROOM_PEOPLE(other_room), 0, 0, TO_CHAR);
		}
	}
}


int find_door(char_data *ch, const char *type, char *dir, const char *cmdname) {
	struct room_direction_data *ex;
	int door;

	if (*dir) {			/* a direction was specified */
		if ((door = parse_direction(ch, dir)) == NO_DIR) {	/* Partial Match */
			send_to_char("That's not a direction.\r\n", ch);
			return (-1);
		}
		if ((ex = find_exit(IN_ROOM(ch), door))) {	/* Braces added according to indent. -gg */
			if (ex->keyword) {
				if (isname((char *) type, ex->keyword))
					return (door);
				else {
					sprintf(buf2, "I see no %s there.\r\n", type);
					send_to_char(buf2, ch);
					return (NO_DIR);
				}
			}
			else
				return (door);
		}
		else {
			sprintf(buf2, "I really don't see how you can %s anything there.\r\n", cmdname);
			send_to_char(buf2, ch);
			return (NO_DIR);
		}
	}
	else {			/* try to locate the keyword */
		if (!*type) {
			sprintf(buf2, "What is it you want to %s?\r\n", cmdname);
			send_to_char(buf2, ch);
			return (NO_DIR);
		}
		if (COMPLEX_DATA(IN_ROOM(ch))) {
			for (ex = COMPLEX_DATA(IN_ROOM(ch))->exits; ex; ex = ex->next) {
				if (ex->keyword) {
					if (isname((char *) type, ex->keyword)) {
						return (ex->dir);
					}
				}
			}
		}

		sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
		send_to_char(buf2, ch);
		return (NO_DIR);
	}
}


// processes the character's north
int get_north_for_char(char_data *ch) {
	if (IS_NPC(ch) || (HAS_ABILITY(ch, ABIL_NAVIGATION) && GET_COND(ch, DRUNK) < 250)) {
		return NORTH;
	}
	else {
		return (int)GET_CONFUSED_DIR(ch);
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// MOVE VALIDATORS /////////////////////////////////////////////////////////

// dir here is a real dir, not a confused dir
int can_move(char_data *ch, int dir, room_data *to_room, int need_specials_check) {
	// water->mountain
	if (!PLR_FLAGGED(ch, PLR_UNRESTRICT) && WATER_SECT(IN_ROOM(ch))) {
		if (ROOM_SECT_FLAGGED(to_room, SECTF_ROUGH) && !EFFECTIVELY_FLYING(ch)) {
			send_to_char("You are unable to scale the cliff.\r\n", ch);
			return 0;
		}
	}
	if (!PLR_FLAGGED(ch, PLR_UNRESTRICT) && IS_MAP_BUILDING(to_room) && !IS_INSIDE(IN_ROOM(ch)) && !IS_ADVENTURE_ROOM(IN_ROOM(ch)) && BUILDING_ENTRANCE(to_room) != dir && ROOM_IS_CLOSED(to_room) && (!ROOM_BLD_FLAGGED(to_room, BLD_TWO_ENTRANCES) || BUILDING_ENTRANCE(to_room) != rev_dir[dir])) {
		if (ROOM_BLD_FLAGGED(to_room, BLD_TWO_ENTRANCES)) {
			msg_to_char(ch, "You can't enter it from this side. The entrances are from %s and %s.\r\n", from_dir[get_direction_for_char(ch, BUILDING_ENTRANCE(to_room))], from_dir[get_direction_for_char(ch, rev_dir[BUILDING_ENTRANCE(to_room)])]);
		}
		else {
			msg_to_char(ch, "You can't enter it from this side. The entrance is from %s.\r\n", from_dir[get_direction_for_char(ch, BUILDING_ENTRANCE(to_room))]);
		}
		return 0;
	}
	if (!IS_IMMORTAL(ch) && !PLR_FLAGGED(ch, PLR_UNRESTRICT) && !IS_INSIDE(IN_ROOM(ch)) && !ROOM_IS_CLOSED(IN_ROOM(ch)) && !IS_ADVENTURE_ROOM(IN_ROOM(ch)) && IS_ANY_BUILDING(to_room) && !can_use_room(ch, to_room, GUESTS_ALLOWED) && ROOM_IS_CLOSED(to_room) && !need_specials_check) {
		msg_to_char(ch, "You can't enter a building without permission.\r\n");
		return 0;
	}
	if (IS_RIDING(ch) && ROOM_SECT_FLAGGED(to_room, SECTF_ROUGH) && !HAS_ABILITY(ch, ABIL_ALL_TERRAIN_RIDING) && !EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "You can't ride on such rough terrain.\r\n");
		return 0;
	}
	if (IS_RIDING(ch) && DEEP_WATER_SECT(to_room) && !MOUNT_FLAGGED(ch, MOUNT_AQUATIC) && !EFFECTIVELY_FLYING(ch)) {
		// ATR does not help ocean
		msg_to_char(ch, "Your mount won't ride into the ocean.\r\n");
		return 0;
	}
	if (IS_RIDING(ch) && !HAS_ABILITY(ch, ABIL_ALL_TERRAIN_RIDING) && WATER_SECT(to_room) && !MOUNT_FLAGGED(ch, MOUNT_AQUATIC) && !EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "Your mount won't ride into the water.\r\n");
		return 0;
	}
	if (IS_RIDING(ch) && MOUNT_FLAGGED(ch, MOUNT_AQUATIC) && !WATER_SECT(to_room) && !IS_WATER_BUILDING(to_room) && !EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "Your mount won't go onto the land.\r\n");
		return 0;
	}
	
	// need a generic for this -- a nearly identical condition is used in do_mount
	if (IS_RIDING(ch) && IS_COMPLETE(to_room) && !BLD_ALLOWS_MOUNTS(to_room)) {
		msg_to_char(ch, "You can't ride indoors.\r\n");
		return 0;
	}
	
	if (MOB_FLAGGED(ch, MOB_AQUATIC) && !WATER_SECT(to_room) && !EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "You can't go on land!\r\n");
		return 0;
	}
	
	if (ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_ROUGH) && ROOM_SECT_FLAGGED(to_room, SECTF_ROUGH) && (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_MOUNTAINWALK)) && !HAS_ABILITY(ch, ABIL_MOUNTAIN_CLIMBING) && !EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "You must buy the Mountain Climbing ability to cross such rough terrain.\r\n");
		return 0;
	}
	
	if (ROOM_AFF_FLAGGED(to_room, ROOM_AFF_NO_FLY) && EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "You can't fly there.\r\n");
		return 0;
	}
	
	return 1;
}


/* simple function to determine if char can walk on water */
bool has_boat(char_data *ch) {
	obj_data *obj;

	if (EFFECTIVELY_FLYING(ch)) {
		return TRUE;
	}
	
	for (obj = ch->carrying; obj; obj = obj->next_content)
		if (GET_OBJ_TYPE(obj) == ITEM_BOAT)
			return TRUE;
	return FALSE;
}


/**
* @param char_data *ch
* @param room_data *from Origin room
* @param room_data *to Destination room
* @param int dir Which direction is being moved
* @param int mode MOVE_NORMAL, etc
* @return int the move cost
*/
int move_cost(char_data *ch, room_data *from, room_data *to, int dir, int mode) {
	double need_movement, cost_from, cost_to;
	
	/* move points needed is avg. move loss for src and destination sect type */	
	
	// open buildings and incomplete non-closed buildings use average of current & original sect's move cost
	if (!ROOM_IS_CLOSED(from)) {
		cost_from = (GET_SECT_MOVE_LOSS(SECT(from)) + GET_SECT_MOVE_LOSS(ROOM_ORIGINAL_SECT(from))) / 2.0;
	}
	else {
		cost_from = GET_SECT_MOVE_LOSS(SECT(from));
	}
	
	// cost for the space moving to
	if (!ROOM_IS_CLOSED(to)) {
		cost_to = (GET_SECT_MOVE_LOSS(SECT(to)) + GET_SECT_MOVE_LOSS(ROOM_ORIGINAL_SECT(to))) / 2.0;
	}
	else {
		cost_to = GET_SECT_MOVE_LOSS(SECT(to));
	}
	
	// calculate movement needed
	need_movement = (cost_from + cost_to) / 2.0;
	
	// higher diagonal cost
	if (dir >= NUM_SIMPLE_DIRS && dir < NUM_2D_DIRS) {
		need_movement *= 1.414;	// sqrt(2)
	}
	
	// cheaper cost if either room is a road
	if (IS_ROAD(from) || IS_ROAD(to)) {
		need_movement /= 2.0;
	}
	
	if (IS_RIDING(ch) || EFFECTIVELY_FLYING(ch) || mode == MOVE_EARTHMELD) {
		need_movement /= 4.0;
	}
	
	// no free moves
	return MAX(1, (int)need_movement);
}


/**
* Actual transport between starting locations.
*
* @param char_data *ch The person to transport.
* @param room_data *to_room Where to send them.
*/
void perform_transport(char_data *ch, room_data *to_room) {
	room_data *was_in = IN_ROOM(ch);
	struct follow_type *k;

	msg_to_char(ch, "You transport to another starting location!\r\n");
	act("$n dematerializes and vanishes!", TRUE, ch, 0, 0, TO_ROOM);

	char_to_room(ch, to_room);
	look_at_room(ch);
	if (!IS_NPC(ch)) {
		GET_LAST_DIR(ch) = NO_DIR;
	}

	act("$n materializes in front of you!", TRUE, ch, 0, 0, TO_ROOM);
	
	enter_wtrigger(IN_ROOM(ch), ch, NO_DIR);
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, NO_DIR);
	greet_memory_mtrigger(ch);

	for (k = ch->followers; k; k = k->next) {
		if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING)) {
			act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
			perform_transport(k->follower, to_room);
		}
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// MOVE SYSTEM /////////////////////////////////////////////////////////////

/**
* process sending ch through portal
*
* @param char_data *ch The person to send through.
* @param obj_data *portal The portal object.
* @param bool following TRUE only if this person followed someone else through.
*/
void char_through_portal(char_data *ch, obj_data *portal, bool following) {
	void trigger_distrust_from_stealth(char_data *ch, empire_data *emp);
	
	obj_data *objiter, *use_portal;
	struct follow_type *fol, *next_fol;
	room_data *to_room = real_room(GET_PORTAL_TARGET_VNUM(portal));
	room_data *was_in = IN_ROOM(ch);
	bool found, junk;
	
	// safety
	if (!to_room) {
		return;
	}
	
	act("You enter $p...", FALSE, ch, portal, 0, TO_CHAR);
	act("$n steps into $p!", TRUE, ch, portal, 0, TO_ROOM);
	
	// ch first
	char_from_room(ch);
	char_to_room(ch, to_room);
	if (!IS_NPC(ch)) {
		GET_LAST_DIR(ch) = NO_DIR;
	}
	
	// see if there's a different portal on the other end
	found = FALSE;
	for (objiter = ROOM_CONTENTS(to_room); objiter; objiter = objiter->next_content) {
		if (GET_PORTAL_TARGET_VNUM(objiter) == GET_ROOM_VNUM(was_in)) {
			found = TRUE;
			use_portal = objiter;
			break;
		}
	}
	
	// otherwise just use the original one
	if (!found) {
		use_portal = portal;
	}

	act("$n appears from $p!", TRUE, ch, use_portal, 0, TO_ROOM);
	look_at_room(ch);
	command_lag(ch, WAIT_MOVEMENT);
	
	// portal sickness
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && GET_OBJ_VNUM(portal) == o_PORTAL) {
		if (get_cooldown_time(ch, COOLDOWN_PORTAL_SICKNESS) > 0) {
			if (is_in_city_for_empire(was_in, ROOM_OWNER(was_in), TRUE, &junk) && is_in_city_for_empire(to_room, ROOM_OWNER(to_room), TRUE, &junk)) {
				add_cooldown(ch, COOLDOWN_PORTAL_SICKNESS, 2 * SECS_PER_REAL_MIN);
			}
			else if (HAS_ABILITY(ch, ABIL_PORTAL_MAGIC)) {
				add_cooldown(ch, COOLDOWN_PORTAL_SICKNESS, 4 * SECS_PER_REAL_MIN);
			}
			else {
				add_cooldown(ch, COOLDOWN_PORTAL_SICKNESS, 5 * SECS_PER_REAL_MIN);
			}
		}
		else {
			add_cooldown(ch, COOLDOWN_PORTAL_SICKNESS, SECS_PER_REAL_MIN);
		}
	}
	
	enter_wtrigger(IN_ROOM(ch), ch, NO_DIR);
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, NO_DIR);
	greet_memory_mtrigger(ch);
	
	// now followers
	for (fol = ch->followers; fol; fol = next_fol) {
		next_fol = fol->next;
		if ((IN_ROOM(fol->follower) == was_in) && (GET_POS(fol->follower) >= POS_STANDING) && can_enter_room(fol->follower, to_room)) {
			if (!IS_IMMORTAL(fol->follower) && GET_OBJ_VNUM(portal) == o_PORTAL && get_cooldown_time(fol->follower, COOLDOWN_PORTAL_SICKNESS) > SECS_PER_REAL_MIN) {
				msg_to_char(ch, "You can't enter a portal until your portal sickness cooldown is under one minute.\r\n");
			}
			else {
				act("You follow $N.\r\n", FALSE, fol->follower, 0, ch, TO_CHAR);
				char_through_portal(fol->follower, portal, TRUE);
			}
		}
	}
	
	// trigger distrust?
	if (!following && ROOM_OWNER(was_in) && !IS_IMMORTAL(ch) && !IS_NPC(ch) && !can_use_room(ch, was_in, GUESTS_ALLOWED)) {
		trigger_distrust_from_stealth(ch, ROOM_OWNER(was_in));
	}
}


/* do_simple_move assumes
*    1. That there is no master and no followers.
*    2. That the direction exists.
*
* @return bool TRUE on success, FALSE on failure
*/
bool do_simple_move(char_data *ch, int dir, room_data *to_room, int need_specials_check, byte mode) {
	void cancel_action(char_data *ch);
	extern const struct action_data_struct action_data[];
	extern const char *mob_move_types[];
	
	char lbuf[MAX_STRING_LENGTH];
	room_data *was_in = IN_ROOM(ch), *from_room;
	int need_movement, move_type, reverse = (IS_NPC(ch) || GET_LAST_DIR(ch) == NO_DIR) ? NORTH : rev_dir[(int) GET_LAST_DIR(ch)];
	char_data *animal = NULL, *vict;
	obj_data *cart = NULL;


	/* First things first: Are we pulling a cart? */
	if ((cart = GET_PULLING(ch))) {
		if (IN_ROOM(cart) != IN_ROOM(ch)) {
			// don't bother
			cart = NULL;
		}
		else {
			mode = MOVE_CART;
			if (ch == GET_PULLED_BY(cart, 0))
				animal = GET_PULLED_BY(cart, 1);
			else
				animal = GET_PULLED_BY(cart, 0);
			if (animal && IN_ROOM(animal) != IN_ROOM(ch))
				animal = NULL;

			/* Make sure there's enough work animals */
			if (GET_CART_ANIMALS_REQUIRED(cart) > 1) {
				if (!animal) {
					act("You need two animals to move $p.", FALSE, ch, cart, 0, TO_CHAR);
					return FALSE;
				}
		
				if (animal && MOB_FLAGGED(animal, MOB_TIED)) {
					act("The other animal pulling $p is tied up.", FALSE, ch, cart, 0, TO_CHAR);
					return FALSE;
				}
				if (animal && is_fighting(animal)) {
					act("The other animal pulling $p is fighting!", FALSE, ch, cart, 0, TO_CHAR);
					return FALSE;
				}
			}
		}
	}
	
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch) && IS_CARRYING_N(ch) > CAN_CARRY_N(ch)) {
		msg_to_char(ch, "You are overburdened and cannot move.\r\n");
		return FALSE;
	}
	
	if (IS_INJURED(ch, INJ_TIED)) {
		msg_to_char(ch, "You can't seem to move!\r\n");
		return FALSE;
	}
	
	if (MOB_FLAGGED(ch, MOB_TIED)) {
		msg_to_char(ch, "You are tied in place.\r\n");
		return FALSE;
	}

	/* blocked by a leave trigger ? */
	if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in) {
		/* prevent teleport crashes */
		return FALSE;
	}
	if (!leave_wtrigger(IN_ROOM(ch), ch, dir) || IN_ROOM(ch) != was_in) {
		/* prevent teleport crashes */
		return FALSE;
	}
	if (!leave_otrigger(IN_ROOM(ch), ch, dir) || IN_ROOM(ch) != was_in) {
		/* prevent teleport crashes */
		return FALSE;
	}

	/* charmed? */
	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
		send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
		act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
		return FALSE;
	}

	if (GET_FEEDING_FROM(ch) || GET_FED_ON_BY(ch)) {
		msg_to_char(ch, "You can't seem to move!\r\n");
		return FALSE;
	}

	/* if the room we're going to needs a boat, check for one.  You can wade if you're already in one */
	if (WATER_SECT(to_room)) {
		if (!has_boat(ch) && !IS_RIDING(ch)) {
			if (HAS_ABILITY(ch, ABIL_SWIMMING) && (mode == MOVE_NORMAL || mode == MOVE_FOLLOW)) {
				mode = MOVE_SWIM;
			}
			else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_AQUATIC)) {
				mode = MOVE_SWIM;
			}
			else {
				send_to_char("You don't know how to swim.\r\n", ch);
				return FALSE;
			}
		}
		if (cart) {
			msg_to_char(ch, "You can't pull anything into the water.\r\n");
			return FALSE;
		}
	}

	// move into a barrier at all?
	if (mode != MOVE_EARTHMELD && !REAL_NPC(ch) && !can_use_room(ch, to_room, MEMBERS_ONLY) && !PLR_FLAGGED(ch, PLR_UNRESTRICT) && ROOM_BLD_FLAGGED(to_room, BLD_BARRIER) && IS_COMPLETE(to_room) && !EFFECTIVELY_FLYING(ch)) {
		msg_to_char(ch, "There is a barrier in your way.\r\n");
		return FALSE;
	}

	// wall-block: can only go back last-direction
	if (mode != MOVE_EARTHMELD && !REAL_NPC(ch) && !PLR_FLAGGED(ch, PLR_UNRESTRICT) && !EFFECTIVELY_FLYING(ch)) {
		if (ROOM_BLD_FLAGGED(IN_ROOM(ch), BLD_BARRIER) && IS_COMPLETE(IN_ROOM(ch)) && GET_LAST_DIR(ch) != NO_DIR && dir != reverse) {
			from_room = real_shift(IN_ROOM(ch), shift_dir[reverse][0], shift_dir[reverse][1]);
			
			// ok, we know we're on a wall and trying to go the wrong way. But does the next tile also have this issue?
			if (from_room && (!ROOM_BLD_FLAGGED(from_room, BLD_BARRIER) || !IS_COMPLETE(from_room) || !can_use_room(ch, from_room, MEMBERS_ONLY))) {
				msg_to_char(ch, "You are unable to pass. You can only go back %s.\r\n", dirs[get_direction_for_char(ch, reverse)]);
				return FALSE;
			}
		}
	}
	
	// unfinished tunnel
	if ((BUILDING_VNUM(IN_ROOM(ch)) == BUILDING_TUNNEL || BUILDING_VNUM(IN_ROOM(ch)) == RTYPE_TUNNEL) && !IS_COMPLETE(IN_ROOM(ch)) && mode != MOVE_EARTHMELD && !REAL_NPC(ch) && !PLR_FLAGGED(ch, PLR_UNRESTRICT) && dir != rev_dir[(int) GET_LAST_DIR(ch)]) {
		msg_to_char(ch, "The tunnel is incomplete. You can only go back %s.\r\n", dirs[get_direction_for_char(ch, rev_dir[(int) GET_LAST_DIR(ch)])]);
		return FALSE;
	}

	if (ROOM_SECT_FLAGGED(to_room, SECTF_ROUGH) && cart) {
		msg_to_char(ch, "You can't pull anything over such rough terrain!\r\n");
		return FALSE;
	}
	if (ROOM_BLD_FLAGGED(to_room, BLD_BARRIER) && cart) {
		msg_to_char(ch, "You can't pull anything that close to the barrier.\r\n");
		return FALSE;
	}

	if (!REAL_NPC(ch) && mode != MOVE_EARTHMELD && !can_move(ch, dir, to_room, need_specials_check))
		return FALSE;

	/* move points needed is avg. move loss for src and destination sect type */
	need_movement = move_cost(ch, IN_ROOM(ch), to_room, dir, mode);

	if (GET_MOVE(ch) < need_movement && !IS_IMMORTAL(ch) && !IS_NPC(ch)) {
		if (need_specials_check && ch->master)
			send_to_char("You are too exhausted to follow.\r\n", ch);
		else
			send_to_char("You are too exhausted.\r\n", ch);

		return FALSE;
	}

	if ((cart || GET_LED_BY(ch)) && IS_MAP_BUILDING(to_room) && !BLD_ALLOWS_MOUNTS(to_room))
		return FALSE;
		
	if (AFF_FLAGGED(ch, AFF_ENTANGLED)) {
		msg_to_char(ch, "You are entangled and can't move.\r\n");
		return FALSE;
	}

	if (MOB_FLAGGED(ch, MOB_TIED)) {
		msg_to_char(ch, "You're tied up.\r\n");
		return FALSE;
	}
	
	// checks instance limits, etc
	if (!can_enter_room(ch, to_room)) {
		msg_to_char(ch, "You can't seem to go there. Perhaps it's full.\r\n");
		return FALSE;
	}

	/* Now we know we're allowed to go into the room. */
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch))
		GET_MOVE(ch) -= need_movement;

	REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
	
	// determine real move type
	move_type = IS_NPC(ch) ? MOB_MOVE_TYPE(ch) : MOB_MOVE_WALK;
	if (AFF_FLAGGED(ch, AFF_FLY) && !IS_NPC(ch)) {
		move_type = MOB_MOVE_FLY;
	}
	else if (IS_RIDING(ch)) {
		move_type = MOB_MOVE_RIDE;
	}
	else if (move_type == MOB_MOVE_WALK && ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_FRESH_WATER | SECTF_OCEAN) && move_type != MOB_MOVE_FLY) {
		move_type = MOB_MOVE_SWIM;
	}
	else if (move_type == MOB_MOVE_WALK && ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_ROUGH) && move_type != MOB_MOVE_FLY) {
		move_type = MOB_MOVE_CLIMB;
	}
	// tweak for people in boats
	if (!IS_NPC(ch) && has_boat(ch) && move_type == MOB_MOVE_SWIM) {
		move_type = MOB_MOVE_PADDLE;
	}

	// leaving message
	if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
		*buf2 = '\0';
		
		switch (mode) {
			case MOVE_CART:
				// %s handled later
				strcpy(buf2, "$p is pulled %s.");
				break;
			case MOVE_LEAD:
				sprintf(buf2, "%s leads $n with %s.", HSSH(mode == MOVE_LEAD ? GET_LED_BY(ch) : ch->master), HMHR(mode == MOVE_LEAD ? GET_LED_BY(ch) : ch->master));
				break;
			case MOVE_FOLLOW:
				sprintf(buf2, "$n follows %s %%s.", HMHR(mode == MOVE_LEAD ? GET_LED_BY(ch) : ch->master));
				break;
			case MOVE_EARTHMELD:
				break;
			default: {
				// this leaves a %s for later
				sprintf(buf2, "$n %s %%s.", mob_move_types[move_type]);
				break;
			}
		}
		if (*buf2 && (!ROOM_AFF_FLAGGED(IN_ROOM(ch), ROOM_AFF_SILENT) || number(0, 4))) {
			for (vict = ROOM_PEOPLE(IN_ROOM(ch)); vict; vict = vict->next_in_room) {
				if (vict == ch || !vict->desc) {
					continue;
				}
				if (!CAN_SEE(vict, ch) || !WIZHIDE_OK(vict, ch)) {
					continue;
				}

				// adjust direction
				if (strstr(buf2, "%s") != NULL) {
					sprintf(lbuf, buf2, dirs[get_direction_for_char(vict, dir)]);
				}
				else {
					strcpy(lbuf, buf2);
				}
				
				act(lbuf, TRUE, ch, cart, vict, TO_VICT);
			}
		}
	}

	// mark it
	add_tracks(ch, IN_ROOM(ch), dir);

	char_from_room(ch);
	char_to_room(ch, to_room);

	/* move them first, then move them back if they aren't allowed to go. */
	/* see if an entry trigger disallows the move */
	if (!entry_mtrigger(ch) || !enter_wtrigger(IN_ROOM(ch), ch, dir)) {
		char_from_room(ch);
		char_to_room(ch, was_in);
		return FALSE;
	}

	if (!IS_NPC(ch)) {
		GET_LAST_DIR(ch) = dir;
	}

	// cancel some actions on movement
	if (!IS_NPC(ch) && GET_ACTION(ch) != ACT_NONE && !IS_SET(action_data[GET_ACTION(ch)].flags, ACTF_ANYWHERE) && GET_ACTION_ROOM(ch) != GET_ROOM_VNUM(IN_ROOM(ch)) && GET_ACTION_ROOM(ch) != NOWHERE) {
		cancel_action(ch);
	}
	
	command_lag(ch, WAIT_MOVEMENT);

	if (animal) {
		char_from_room(animal);
		char_to_room(animal, to_room);
	}
	if (cart) {
		obj_to_room(cart, to_room);
	}

	// walks-in messages
	if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
		switch (mode) {
			case MOVE_CART:
				for (vict = ROOM_PEOPLE(IN_ROOM(ch)); vict; vict = vict->next_in_room) {
					if (ch != vict && vict->desc) {
						sprintf(buf2, "$p is pulled up from %s.", from_dir[get_direction_for_char(vict, dir)]);
						act(buf2, TRUE, ch, cart, vict, TO_VICT);
					}
				}
				break;
			case MOVE_LEAD:
				act("$E leads $n behind $M.", TRUE, ch, 0, GET_LED_BY(ch), TO_NOTVICT);
				act("You lead $n behind you.", TRUE, ch, 0, GET_LED_BY(ch), TO_VICT);
				break;
			case MOVE_FOLLOW:
				act("$N follows $m.", TRUE, ch->master, 0, ch, TO_NOTVICT);
				act("$n follows you.", TRUE, ch, 0, ch->master, TO_VICT);
				break;
			case MOVE_EARTHMELD:
				break;
			default: {
				// this leaves a %s for later
				sprintf(buf2, "$n %s up from %%s.", mob_move_types[move_type]);

				for (vict = ROOM_PEOPLE(IN_ROOM(ch)); vict; vict = vict->next_in_room) {
					if (vict == ch || !vict->desc) {
						continue;
					}
					if (!CAN_SEE(vict, ch) || !WIZHIDE_OK(vict, ch)) {
						continue;
					}
					
					// adjust direction
					if (strstr(buf2, "%s")) {
						sprintf(lbuf, buf2, from_dir[get_direction_for_char(vict, dir)]);
					}
					else {
						strcpy(lbuf, buf);
					}

					act(lbuf, TRUE, ch, cart, vict, TO_VICT);
				}
				break;
			}
		}
	}

	if (ch->desc != NULL) {
		look_at_room(ch);
	}
	if (animal && animal->desc != NULL) {
		look_at_room(animal);
	}

	// skill gain section
	if (mode == MOVE_SWIM) {
		gain_ability_exp(ch, ABIL_SWIMMING, 1);
	}
	
	if (IS_RIDING(ch)) {
		gain_ability_exp(ch, ABIL_RIDE, 1);
		
		if (ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_FRESH_WATER | SECTF_OCEAN | SECTF_ROUGH)) {
			gain_ability_exp(ch, ABIL_ALL_TERRAIN_RIDING, 5);
		}
	}
	else {	// not riding
		if (ROOM_SECT_FLAGGED(was_in, SECTF_ROUGH) && ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_ROUGH)) {
			gain_ability_exp(ch, ABIL_MOUNTAIN_CLIMBING, 10);
		}
		gain_ability_exp(ch, ABIL_NAVIGATION, 1);

		if (AFF_FLAGGED(ch, AFF_FLY)) {
			gain_ability_exp(ch, ABIL_FLY, 1);
		}
	}

	// trigger section
	entry_memory_mtrigger(ch);
	if (!greet_mtrigger(ch, dir)) {
		char_from_room(ch);
		char_to_room(ch, was_in);
		look_at_room(ch);
	}
	else {
		greet_memory_mtrigger(ch);
	}

	return TRUE;
}


int perform_move(char_data *ch, int dir, int need_specials_check, byte mode) {
	room_data *was_in, *to_room = IN_ROOM(ch);
	struct room_direction_data *ex;
	struct follow_type *k, *next;

	if (ch == NULL)
		return FALSE;

	if ((PLR_FLAGGED(ch, PLR_UNRESTRICT) && !IS_ADVENTURE_ROOM(IN_ROOM(ch)) && !IS_INSIDE(IN_ROOM(ch))) || !ROOM_IS_CLOSED(IN_ROOM(ch))) {
		if (dir >= NUM_2D_DIRS || dir < 0) {
			send_to_char("Alas, you cannot go that way...\r\n", ch);
			return FALSE;
		}
		// may produce a NULL
		to_room = real_shift(IN_ROOM(ch), shift_dir[dir][0], shift_dir[dir][1]);
	}
	else {
		if (!(ex = find_exit(IN_ROOM(ch), dir)) || !ex->room_ptr) {
			msg_to_char(ch, "Alas, you cannot go that way...\r\n");
			return FALSE;
		}
		if (EXIT_FLAGGED(ex, EX_CLOSED) && ex->keyword) {
			msg_to_char(ch, "The %s is closed.\r\n", fname(ex->keyword));
			return FALSE;
		}
		to_room = ex->room_ptr;
	}
	
	// safety (and map bounds)
	if (!to_room) {
		msg_to_char(ch, "Alas, you cannot go that way...\r\n");
		return FALSE;
	}

	/* Store old room */
	was_in = IN_ROOM(ch);

	if (!do_simple_move(ch, dir, to_room, need_specials_check, mode))
		return FALSE;

	if (GET_LEADING(ch) && IN_ROOM(GET_LEADING(ch)) == was_in)
		perform_move(GET_LEADING(ch), dir, 1, MOVE_LEAD);

	for (k = ch->followers; k; k = next) {
		next = k->next;
		if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING)) {
			act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
			perform_move(k->follower, dir, 1, MOVE_FOLLOW);
		}
	}
	return TRUE;
}


 //////////////////////////////////////////////////////////////////////////////
//// COMMANDS ////////////////////////////////////////////////////////////////


ACMD(do_avoid) {
	char_data *vict;
	
	one_argument(argument, arg);
	
	if (!*arg) {
		msg_to_char(ch, "Who would you like to avoid?\r\n");
	}
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
	}
	else if (vict == ch) {
		send_to_char("You can't avoid yourself, no matter how hard you try.\r\n", ch);
	}
	else if (vict->master != ch) {
		act("$E isn't even following you.", FALSE, ch, NULL, vict, TO_CHAR);
	}
	else if (IS_NPC(vict)) {
		msg_to_char(ch, "You can only use 'avoid' on player characters.\r\n");
	}
	else {
		act("You manage to avoid $N.", FALSE, ch, NULL, vict, TO_CHAR);
		act("You were following $n, but $e has managed to avoid you.", FALSE, ch, NULL, vict, TO_VICT);
		act("$n manages to avoid $N, who was following $m.", TRUE, ch, NULL, vict, TO_NOTVICT);
		
		stop_follower(vict);
		GET_WAIT_STATE(vict) = 4 RL_SEC;
	}
}


ACMD(do_circle) {
	// directions to check for blocking walls for 
	const int check_circle_dirs[NUM_SIMPLE_DIRS][2] = {
		{ EAST, WEST },	// n
		{ NORTH, SOUTH }, // e
		{ EAST, WEST }, // s
		{ NORTH, SOUTH } // w
	};
	
	char_data *vict;
	int dir, iter, need_movement, side, blocked[NUM_SIMPLE_DIRS];
	room_data *to_room, *found_room = NULL, *side_room, *was_in = IN_ROOM(ch);
	struct follow_type *fol, *next_fol;
	bool ok = TRUE, found_side_clear;
	
	one_argument(argument, arg);
	
	if (!*arg) {
		msg_to_char(ch, "Circle which direction?\r\n");
		return;
	}
	
	if ((dir = parse_direction(ch, arg)) == NO_DIR) {
		msg_to_char(ch, "Invalid direction.\r\n");
		return;
	}
	
	if (ROOM_IS_CLOSED(IN_ROOM(ch))) {
		msg_to_char(ch, "You need to be outdoors to circle buildings.\r\n");
		return;
	}
	
	if (dir >= NUM_2D_DIRS) {
		msg_to_char(ch, "You can't circle that way.\r\n");
		return;
	}
	
	if (ROOM_BLD_FLAGGED(IN_ROOM(ch), BLD_BARRIER) && IS_COMPLETE(IN_ROOM(ch))) {
		msg_to_char(ch, "You can't circle from here.\r\n");
		return;
	}
	
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch) && IS_CARRYING_N(ch) > CAN_CARRY_N(ch)) {
		msg_to_char(ch, "You are overburdened and cannot move.\r\n");
		return;
	}
	
	// start here
	to_room = IN_ROOM(ch);
	
	// attempt to go up to 4 rooms
	for (iter = 0; iter < 4; ++iter) {
		to_room = real_shift(to_room, shift_dir[dir][0], shift_dir[dir][1]);
		
		// map boundary
		if (!to_room) {
			msg_to_char(ch, "You can't circle that way.\r\n");
			ok = FALSE;
			break;
		}
		
		// check wall directly in the way
		if (ROOM_BLD_FLAGGED(to_room, BLD_BARRIER) && IS_COMPLETE(to_room)) {
			msg_to_char(ch, "There is a barrier in the way.\r\n");
			ok = FALSE;
			break;
		}
		
		// check invalid terrain in the way
		if (ROOM_SECT_FLAGGED(to_room, SECTF_FRESH_WATER | SECTF_OCEAN | SECTF_ROUGH)) {
			msg_to_char(ch, "You can't circle that way.\r\n");
			ok = FALSE;
			break;
		}
		
		// check first room is blocked at all...
		if (iter == 0 && !ROOM_IS_CLOSED(to_room)) {
			msg_to_char(ch, "There's nothing to circle.\r\n");
			ok = FALSE;
			break;
		}
		
		// detect blocked array
		for (side = 0; side < NUM_SIMPLE_DIRS; ++side) {
			side_room = real_shift(to_room, shift_dir[side][0], shift_dir[side][1]);
			
			if (!side_room || !ROOM_BLD_FLAGGED(side_room, BLD_BARRIER) || !IS_COMPLETE(side_room)) {
				blocked[side] = FALSE;
			}
			else {
				blocked[side] = TRUE;
			}
		}
		
		// check if the correct dirs are blocked
		found_side_clear = FALSE;
		
		if (dir < NUM_SIMPLE_DIRS) {
			for (side = 0; side < 2; ++side) {
				if (!blocked[check_circle_dirs[dir][side]]) {
					found_side_clear = TRUE;
				}
			}
		}
		else {
			// if circling at an angle, rules are slightly different
			// first, if N/S or E/W pairs are blocked, any angle is blocked
			if (!(blocked[NORTH] && blocked[SOUTH]) && !(blocked[EAST] && blocked[WEST])) {
				// check L-shaped walls
				if (dir == NORTHEAST || dir == SOUTHWEST) {
					if (!(blocked[NORTH] && blocked[EAST]) && !(blocked[SOUTH] && blocked[WEST])) {
						found_side_clear = TRUE;
					}
				}
				else if (dir == NORTHWEST || dir == SOUTHEAST) {
					if (!(blocked[NORTH] && blocked[WEST]) && !(blocked[SOUTH] && blocked[EAST])) {
						found_side_clear = TRUE;
					}
				}
				else if ((!blocked[NORTH] || !blocked[SOUTH]) && (!blocked[EAST] || !blocked[WEST])) {
					// weird misc case
					found_side_clear = TRUE;
				}
			}
		}
		
		if (!found_side_clear) {
			msg_to_char(ch, "There is a barrier in the way.\r\n");
			ok = FALSE;
			break;
		}
		
		if (!ROOM_IS_CLOSED(to_room)) {
			found_room = to_room;
			break;
		}
	}
	
	if (!ok) {
		return;
	}
	
	if (!found_room) {
		msg_to_char(ch, "You can't circle that way.\r\n");
		return;
	}
	
	// check costs
	need_movement = move_cost(ch, IN_ROOM(ch), found_room, dir, MOVE_NORMAL);
	
	if (GET_MOVE(ch) < need_movement && !IS_IMMORTAL(ch) && !IS_NPC(ch)) {
		msg_to_char(ch, "You're too tired to circle that way.\r\n");
		return;
	}
	
	// message
	msg_to_char(ch, "You circle %s.\r\n", dirs[get_direction_for_char(ch, dir)]);
	if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
		for (vict = ROOM_PEOPLE(IN_ROOM(ch)); vict; vict = vict->next_in_room) {
			if (vict != ch && vict->desc && CAN_SEE(vict, ch)) {
				sprintf(buf, "$n circles %s.", dirs[get_direction_for_char(vict, dir)]);
				act(buf, TRUE, ch, NULL, vict, TO_VICT);
			}
		}
	}

	// work
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch)) {
		GET_MOVE(ch) -= need_movement;
	}
	char_from_room(ch);
	char_to_room(ch, found_room);
	
	if (!IS_NPC(ch)) {
		GET_LAST_DIR(ch) = dir;
	}
	
	if (ch->desc) {
		look_at_room(ch);
	}
	
	command_lag(ch, WAIT_MOVEMENT);
	
	// triggers?
	if (!enter_wtrigger(IN_ROOM(ch), ch, dir) || !greet_mtrigger(ch, dir)) {
		char_from_room(ch);
		char_to_room(ch, was_in);
		if (ch->desc) {
			look_at_room(ch);
		}
		return;
	}
	entry_memory_mtrigger(ch);
	greet_memory_mtrigger(ch);
	
	// message
	if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
		for (vict = ROOM_PEOPLE(IN_ROOM(ch)); vict; vict = vict->next_in_room) {
			if (vict != ch && vict->desc && CAN_SEE(vict, ch)) {
				sprintf(buf, "$n circles in from %s.", from_dir[get_direction_for_char(vict, dir)]);
				act(buf, TRUE, ch, NULL, vict, TO_VICT);
			}
		}
	}

	// followers
	for (fol = ch->followers; fol; fol = next_fol) {
		next_fol = fol->next;
		if ((IN_ROOM(fol->follower) == was_in) && (GET_POS(fol->follower) >= POS_STANDING)) {
			sprintf(buf, "%s", dirs[get_direction_for_char(fol->follower, dir)]);
			do_circle(fol->follower, buf, 0, 0);
		}
	}
}


// enters a portal
ACMD(do_enter) {
	extern bool can_infiltrate(char_data *ch, empire_data *emp);
	
	char_data *tmp_char;
	obj_data *portal;
	room_data *room;
	
	if (AFF_FLAGGED(ch, AFF_ENTANGLED)) {
		msg_to_char(ch, "You are entangled and can't enter anything.\r\n");
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
		msg_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
		act("$n bursts into tears.", FALSE, ch, NULL, NULL, TO_ROOM);
		return;
	}
	
	one_argument(argument, arg);
	
	if (!*arg) {
		msg_to_char(ch, "Enter what?\r\n");
		return;
	}

	generic_find(arg, FIND_OBJ_ROOM, ch, &tmp_char, &portal);
	
	if (!portal) {
		msg_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
		return;
	}
	
	room = real_room(GET_PORTAL_TARGET_VNUM(portal));
	if (!room) {
		act("$p doesn't seem to lead anywhere.", FALSE, ch, portal, 0, TO_CHAR);
		return;
	}
	
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch) && IS_CARRYING_N(ch) > CAN_CARRY_N(ch)) {
		msg_to_char(ch, "You are overburdened and cannot move.\r\n");
		return;
	}
	
	if (!can_enter_room(ch, room)) {
		msg_to_char(ch, "You can't seem to go there. Perhaps it's full.\r\n");
		return;
	}
	
	// portal sickness
	if (!IS_IMMORTAL(ch) && GET_OBJ_VNUM(portal) == o_PORTAL && get_cooldown_time(ch, COOLDOWN_PORTAL_SICKNESS) > SECS_PER_REAL_MIN) {
		msg_to_char(ch, "You can't enter a portal until your portal sickness cooldown is under one minute.\r\n");
		return;
	}
	
	// permissions
	if (ROOM_OWNER(IN_ROOM(ch)) && !IS_IMMORTAL(ch) && !IS_NPC(ch) && (!can_use_room(ch, IN_ROOM(ch), GUESTS_ALLOWED) || !can_use_room(ch, room, GUESTS_ALLOWED))) {
		if (!HAS_ABILITY(ch, ABIL_INFILTRATE)) {
			msg_to_char(ch, "You don't have permission to enter that.\r\n");
			return;
		}
		if (!can_infiltrate(ch, ROOM_OWNER(IN_ROOM(ch)))) {
			// sends own message
			return;
		}
	}
	
	char_through_portal(ch, portal, FALSE);
}


ACMD(do_follow) {
	bool circle_follow(char_data *ch, char_data *victim);
	char_data *leader;

	one_argument(argument, buf);

	if (*buf) {
		if (!(leader = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
			send_config_msg(ch, "no_person");
			return;
		}
	}
	else {
		send_to_char("Whom do you wish to follow?\r\n", ch);
		return;
	}

	if (ch->master == leader) {
		act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
		act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
	else {			/* Not Charmed follow person */
		if (leader == ch) {
			if (!ch->master) {
				send_to_char("You are already following yourself.\r\n", ch);
				return;
			}
			stop_follower(ch);
		}
		else {
			if (circle_follow(ch, leader)) {
				send_to_char("Sorry, but following in loops is not allowed.\r\n", ch);
				return;
			}
			if (ch->master) {
				stop_follower(ch);
			}

			add_follower(ch, leader, TRUE);
		}
	}
}


ACMD(do_gen_door) {
	int door = NO_DIR;
	char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
	obj_data *obj = NULL;
	char_data *victim = NULL;
	struct room_direction_data *ex;
	
	#define DOOR_IS_OPENABLE(ch, obj, ex)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) : \
			(EXIT_FLAGGED((ex), EX_ISDOOR)))
	#define DOOR_IS_OPEN(ch, obj, ex)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) : \
			(!EXIT_FLAGGED((ex), EX_CLOSED)))

	#define DOOR_IS_CLOSED(ch, obj, ex)	(!(DOOR_IS_OPEN(ch, obj, ex)))

	skip_spaces(&argument);
	if (!*argument) {
		sprintf(buf, "%s what?\r\n", cmd_door[subcmd]);
		send_to_char(CAP(buf), ch);
		return;
	}
	two_arguments(argument, type, dir);
	if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
		door = find_door(ch, type, dir, cmd_door[subcmd]);
	
	ex = find_exit(IN_ROOM(ch), door);

	if ((obj) || (ex)) {
		if (!(DOOR_IS_OPENABLE(ch, obj, ex)))
			act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
		else if (!DOOR_IS_OPEN(ch, obj, ex) && IS_SET(flags_door[subcmd], NEED_OPEN))
			send_to_char("But it's already closed!\r\n", ch);
		else if (!DOOR_IS_CLOSED(ch, obj, ex) && IS_SET(flags_door[subcmd], NEED_CLOSED))
			send_to_char("But it's currently open!\r\n", ch);
		else
			do_doorcmd(ch, obj, door, subcmd);
	}
	return;
}


ACMD(do_land) {	
	if (!AFF_FLAGGED(ch, AFF_FLY)) {
		msg_to_char(ch, "You aren't flying.\r\n");
		return;
	}

	affects_from_char_by_aff_flag(ch, AFF_FLY);
	
	if (!AFF_FLAGGED(ch, AFF_FLY)) {
		msg_to_char(ch, "You land.\r\n");
		act("$n lands.", TRUE, ch, NULL, NULL, TO_ROOM);
	}
	else {
		msg_to_char(ch, "You can't seem to land. Perhaps whatever is causing your flight can't be ended.\r\n");
	}
	
	command_lag(ch, WAIT_OTHER);
}


ACMD(do_lead) {
	char_data *mob;

	one_argument(argument, arg);

	if (GET_LEADING(ch)) {
		act("You stop leading $N.", FALSE, ch, 0, GET_LEADING(ch), TO_CHAR);
		act("$n stops leading $N.", FALSE, ch, 0, GET_LEADING(ch), TO_ROOM);
		GET_LED_BY(GET_LEADING(ch)) = NULL;
		GET_LEADING(ch) = NULL;
	}
	else if (IS_NPC(ch)) {
		msg_to_char(ch, "Npcs can't lead anything.\r\n");
	}
	else if (!*arg)
		msg_to_char(ch, "Lead whom?\r\n");
	else if (!(mob = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (ch == mob)
		msg_to_char(ch, "You can't lead yourself.\r\n");
	else if (!IS_NPC(mob))
		msg_to_char(ch, "You can't lead other players around.\r\n");
	else if (!MOB_FLAGGED(mob, MOB_MOUNTABLE))
		act("You can't lead $N!", FALSE, ch, 0, mob, TO_CHAR);
	else if (GET_LED_BY(mob))
		act("Someone is already leading $M.", FALSE, ch, 0, mob, TO_CHAR);
	else if (mob->desc)
		act("You can't lead $N!", FALSE, ch, 0, mob, TO_CHAR);
	else {
		act("You begin to lead $N.", FALSE, ch, 0, mob, TO_CHAR);
		act("$n begins to lead $N.", TRUE, ch, 0, mob, TO_ROOM);
		GET_LEADING(ch) = mob;
		GET_LED_BY(mob) = ch;
	}
}


ACMD(do_move) {
	extern int get_north_for_char(char_data *ch);
	extern const int confused_dirs[NUM_SIMPLE_DIRS][2][NUM_OF_DIRS];
	
	// this blocks normal moves but not flee
	if (is_fighting(ch)) {
		msg_to_char(ch, "You can't move while fighting!\r\n");
		return;
	}
	
	perform_move(ch, confused_dirs[get_north_for_char(ch)][0][subcmd], FALSE, MOVE_NORMAL);
}


// mortals have to portal from a certain building, immortals can do it anywhere
ACMD(do_portal) {
	void empire_skillup(empire_data *emp, int ability, double amount);
	extern char *get_room_name(room_data *room, bool color);
	
	bool all_access = (IS_IMMORTAL(ch) || (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM)));
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH * 2], line[MAX_STRING_LENGTH];
	room_data *room, *next_room, *target = NULL;
	obj_data *portal, *end, *obj;
	int bsize, lsize, count, num, dist;
	bool all = FALSE, wait_here = FALSE, wait_there = FALSE, ch_in_city;
	bool there_in_city;
	
	int max_out_of_city_portal = config_get_int("max_out_of_city_portal");
	
	argument = any_one_word(argument, arg);
	
	if (!str_cmp(arg, "-a") || !str_cmp(arg, "-all")) {
		all = TRUE;
	}
	
	// just show portals
	if (all || !*arg) {
		if (IS_NPC(ch) || !ch->desc) {
			msg_to_char(ch, "You can't list portals right now.\r\n");
			return;
		}
		if (!GET_LOYALTY(ch)) {
			msg_to_char(ch, "You can't get a portal list if you're not in an empire.\r\n");
			return;
		}
		
		bsize = snprintf(buf, sizeof(buf), "Known portals:\r\n");
		
		count = 0;
		ch_in_city = (is_in_city_for_empire(IN_ROOM(ch), ROOM_OWNER(IN_ROOM(ch)), TRUE, &wait_here) || (!ROOM_OWNER(IN_ROOM(ch)) && is_in_city_for_empire(IN_ROOM(ch), GET_LOYALTY(ch), TRUE, &wait_here)));
		HASH_ITER(world_hh, world_table, room, next_room) {
			// early exit
			if (bsize >= sizeof(buf) - 1) {
				break;
			}
			
			dist = compute_distance(IN_ROOM(ch), room);
			there_in_city = is_in_city_for_empire(room, ROOM_OWNER(room), TRUE, &wait_there);
			
			if (ROOM_OWNER(room) && ROOM_BLD_FLAGGED(room, BLD_PORTAL) && IS_COMPLETE(room) && can_use_room(ch, room, all ? GUESTS_ALLOWED : MEMBERS_AND_ALLIES) && (!all || (dist <= max_out_of_city_portal || (ch_in_city && there_in_city)))) {
				// only shows owned portals the character can use
				++count;
				*line = '\0';
				lsize = 0;
				
				// sequential numbering: only numbers them if it's not a -all
				if (!all) {
					lsize += snprintf(line + lsize, sizeof(line) - lsize, "%2d. ", count);
				}
				
				// coords: navigation only
				if (HAS_ABILITY(ch, ABIL_NAVIGATION)) {
					lsize += snprintf(line + lsize, sizeof(line) - lsize, "(%*d, %*d) ", X_PRECISION, X_COORD(room), Y_PRECISION, Y_COORD(room));
				}
				
				lsize += snprintf(line + lsize, sizeof(line) - lsize, "%s (%s%s&0)", get_room_name(room, FALSE), EMPIRE_BANNER(ROOM_OWNER(room)), EMPIRE_ADJECTIVE(ROOM_OWNER(room)));
				
				if ((dist > max_out_of_city_portal && (!ch_in_city || !there_in_city)) || (!HAS_ABILITY(ch, ABIL_PORTAL_MASTER) && (!GET_LOYALTY(ch) || !EMPIRE_HAS_TECH(GET_LOYALTY(ch), TECH_MASTER_PORTALS)) && GET_ISLAND_ID(IN_ROOM(ch)) != GET_ISLAND_ID(target))) {
					lsize += snprintf(line + lsize, sizeof(line) - lsize, " &r(too far)&0");
				}
				
				bsize += snprintf(buf + bsize, sizeof(buf) - bsize, "%s\r\n", line);
			}
		}
		
		// page it in case it's long
		page_string(ch->desc, buf, TRUE);
		command_lag(ch, WAIT_OTHER);
		return;
	}
	
	// targeting: by list number (only targets member/ally portals
	if (is_number(arg) && (num = atoi(arg)) >= 1 && GET_LOYALTY(ch)) {
		HASH_ITER(world_hh, world_table, room, next_room) {
			if (ROOM_OWNER(room) && ROOM_BLD_FLAGGED(room, BLD_PORTAL) && IS_COMPLETE(room) && can_use_room(ch, room, MEMBERS_AND_ALLIES)) {
				if (--num <= 0) {
					target = room;
					break;
				}
			}
		}
	}
	
	// targeting: if we didn't get a result yet, try standard targeting
	if (!target && !(target = find_target_room(ch, arg))) {
		// sends own message
		return;
	}

	// ok, we have a target...
	if (!all_access && !HAS_ABILITY(ch, ABIL_PORTAL_MAGIC)  && (!GET_LOYALTY(ch) || !EMPIRE_HAS_TECH(GET_LOYALTY(ch), TECH_PORTALS))) {
		msg_to_char(ch, "You can only open portals if there is a portal mage in your empire.\r\n");
		return;
	}
	if (target == IN_ROOM(ch)) {
		msg_to_char(ch, "There's no point in portaling to the same place you're standing.\r\n");
		return;
	}
	if (!all_access && !can_use_room(ch, IN_ROOM(ch), GUESTS_ALLOWED)) {
		msg_to_char(ch, "You don't have permission to open portals here.\r\n");
		return;
	}
	if (!all_access && (!ROOM_BLD_FLAGGED(IN_ROOM(ch), BLD_PORTAL) || !ROOM_BLD_FLAGGED(target, BLD_PORTAL) || !IS_COMPLETE(target) || !IS_COMPLETE(IN_ROOM(ch)))) {
		msg_to_char(ch, "You can only open portals between portal buildings.\r\n");
		return;
	}
	if (!all_access && !can_use_room(ch, target, GUESTS_ALLOWED)) {
		msg_to_char(ch, "You don't have permission to open a portal to that location.\r\n");
		return;
	}
	if (!HAS_ABILITY(ch, ABIL_PORTAL_MASTER) && (!GET_LOYALTY(ch) || !EMPIRE_HAS_TECH(GET_LOYALTY(ch), TECH_MASTER_PORTALS)) && GET_ISLAND_ID(IN_ROOM(ch)) != GET_ISLAND_ID(target)) {
		msg_to_char(ch, "You can't open a portal to another land without a portal master in your empire.\r\n");
		return;
	}

	// check there's not already a portal to there from here
	for (obj = ROOM_CONTENTS(IN_ROOM(ch)); obj; obj = obj->next_content) {
		if (GET_PORTAL_TARGET_VNUM(obj) == GET_ROOM_VNUM(target)) {
			msg_to_char(ch, "There is already a portal to that location open here.\r\n");
			return;
		}
	}
	
	// check distance
	if (!all_access) {
		if (!is_in_city_for_empire(IN_ROOM(ch), ROOM_OWNER(IN_ROOM(ch)), TRUE, &wait_here) || !is_in_city_for_empire(target, ROOM_OWNER(target), TRUE, &wait_there)) {
			dist = compute_distance(IN_ROOM(ch), target);
			
			if (dist > max_out_of_city_portal) {
				msg_to_char(ch, "You can't open a portal further away than %d tile%s unless both ends are in a city%s.\r\n", max_out_of_city_portal, PLURAL(max_out_of_city_portal), wait_here ? " (this city was founded too recently)" : (wait_there ? " (that city was founded too recently)" : ""));
				return;
			}
		}
	}
	
	// portal this side
	portal = read_object(o_PORTAL, TRUE);
	GET_OBJ_VAL(portal, VAL_PORTAL_TARGET_VNUM) = GET_ROOM_VNUM(target);
	GET_OBJ_TIMER(portal) = 5;
	obj_to_room(portal, IN_ROOM(ch));
	
	if (all_access) {
		act("You wave your hand and create $p!", FALSE, ch, portal, 0, TO_CHAR);
		act("$n waves $s hand and creates $p!", FALSE, ch, portal, 0, TO_ROOM);
	}
	else {
		act("You chant a few mystic words and $p whirls open!", FALSE, ch, portal, 0, TO_CHAR);
		act("$n chants a few mystic words and $p whirls open!", FALSE, ch, portal, 0, TO_ROOM);
	}
		
	load_otrigger(portal);
	
	// portal other side
	end = read_object(o_PORTAL, TRUE);
	GET_OBJ_VAL(end, VAL_PORTAL_TARGET_VNUM) = GET_ROOM_VNUM(IN_ROOM(ch));
	GET_OBJ_TIMER(end) = 5;
	obj_to_room(end, target);
	if (GET_ROOM_VNUM(IN_ROOM(end))) {
		act("$p spins open!", TRUE, ROOM_PEOPLE(IN_ROOM(end)), end, 0, TO_ROOM | TO_CHAR);
	}
	load_otrigger(end);
	
	if (GET_LOYALTY(ch)) {
		empire_skillup(GET_LOYALTY(ch), ABIL_PORTAL_MAGIC, 15);
		empire_skillup(GET_LOYALTY(ch), ABIL_PORTAL_MASTER, 15);
	}
	
	command_lag(ch, WAIT_OTHER);
}


ACMD(do_rest) {
	switch (GET_POS(ch)) {
		case POS_STANDING:
			if (IS_RIDING(ch)) {
				msg_to_char(ch, "You climb down from your mount.\r\n");
				perform_dismount(ch);
			}
			send_to_char("You sit down and rest your tired bones.\r\n", ch);
			act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_RESTING;
			break;
		case POS_SITTING:
			char_from_chair(ch);
			send_to_char("You rest your tired bones on the ground.\r\n", ch);
			act("$n rests on the ground.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_RESTING;
			break;
		case POS_RESTING:
			send_to_char("You are already resting.\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("You have to wake up first.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Rest while fighting?  Are you MAD?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and stop to rest your tired bones.\r\n", ch);
			act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
	}
}


ACMD(do_sit) {
	obj_data *chair;

	one_argument(argument, arg);

	switch (GET_POS(ch)) {
		case POS_STANDING:
			if (IS_RIDING(ch))
				msg_to_char(ch, "You're already sitting!\r\n");
			else if (!*arg) {
				send_to_char("You sit down.\r\n", ch);
				act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
				GET_POS(ch) = POS_SITTING;
				break;
			}
			else if (!(chair = get_obj_in_list_vis(ch, arg, ROOM_CONTENTS(IN_ROOM(ch)))))
				send_to_char("You don't see anything like that here.\r\n", ch);
			else if (!OBJ_FLAGGED(chair, OBJ_CHAIR))
				send_to_char("You can't sit on that!\r\n", ch);
			else if (IN_CHAIR(chair))
				send_to_char("There's already someone sitting there.\r\n", ch);
			else {
				act("You sit on $p.", FALSE, ch, chair, 0, TO_CHAR);
				act("$n sits on $p.", FALSE, ch, chair, 0, TO_ROOM);
				char_to_chair(ch, chair);
				GET_POS(ch) = POS_SITTING;
				break;
			}
			break;
		case POS_SITTING:
			send_to_char("You're sitting already.\r\n", ch);
			break;
		case POS_RESTING:
			if (*arg) {
				send_to_char("You need to stand up before you can sit on something.\r\n", ch);
				return;
			}
			send_to_char("You stop resting, and sit up.\r\n", ch);
			act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SLEEPING:
			send_to_char("You have to wake up first.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Sit down while fighting? Are you MAD?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and sit down.\r\n", ch);
			act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
	}
}


ACMD(do_sleep) {
	switch (GET_POS(ch)) {
		case POS_STANDING:
		case POS_SITTING:
			char_from_chair(ch);
		case POS_RESTING:
			if (IS_RIDING(ch)) {
				msg_to_char(ch, "You climb down from your mount.\r\n");
				perform_dismount(ch);
			}
			send_to_char("You lay down and go to sleep.\r\n", ch);
			act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SLEEPING;
			break;
		case POS_SLEEPING:
			send_to_char("You are already sound asleep.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and lie down to sleep.\r\n", ch);
			act("$n stops floating around, and lie down to sleep.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SLEEPING;
			break;
	}
}


ACMD(do_stand) {
	switch (GET_POS(ch)) {
		case POS_STANDING:
			send_to_char("You are already standing.\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("You stand up.\r\n", ch);
			act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
			char_from_chair(ch);
			/* Will be sitting after a successful bash and may still be fighting. */
			GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
			break;
		case POS_RESTING:
			send_to_char("You stop resting, and stand up.\r\n", ch);
			act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
			char_from_chair(ch);
			GET_POS(ch) = POS_STANDING;
			break;
		case POS_SLEEPING:
			send_to_char("You have to wake up first!\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Do you not consider fighting as standing?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and put your feet on the ground.\r\n", ch);
			act("$n stops floating around, and puts $s feet on the ground.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_STANDING;
			break;
	}
}


ACMD(do_transport) {
	extern room_data *find_starting_location();
	
	char arg[MAX_INPUT_LENGTH];
	room_data *target = NULL;
	
	one_word(argument, arg);
	
	if (!ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_START_LOCATION)) {
		msg_to_char(ch, "You need to be at a starting location to transport!\r\n");
	}
	else if (*arg && !(target = find_target_room(ch, arg))) {
		skip_spaces(&argument);
		msg_to_char(ch, "You can't transport to '%s'\r\n", argument);
	}
	else if (target == IN_ROOM(ch)) {
		msg_to_char(ch, "It seems you've arrived before you even left!\r\n");
	}
	else if (target && !ROOM_SECT_FLAGGED(target, SECTF_START_LOCATION)) {
		msg_to_char(ch, "You can only transport to other starting locations.\r\n");
	}
	else if (target && !can_use_room(ch, target, GUESTS_ALLOWED)) {
		msg_to_char(ch, "You don't have permission to transport there.\r\n");
	}
	else {
		perform_transport(ch, target ? target : find_starting_location());
	}
}


ACMD(do_wake) {
	char_data *vict;
	int self = 0;

	one_argument(argument, arg);
	if (*arg) {
		if (GET_POS(ch) == POS_SLEEPING)
			send_to_char("Maybe you should wake yourself up first.\r\n", ch);
		else if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)) == NULL)
			send_config_msg(ch, "no_person");
		else if (vict == ch)
			self = 1;
		else if (AWAKE(vict))
			act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
		else if (GET_POS(vict) < POS_SLEEPING)
			act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
		else {
			act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
			act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
			GET_POS(vict) = POS_SITTING;
		}
		if (!self)
			return;
	}
	if (GET_POS(ch) > POS_SLEEPING)
		send_to_char("You are already awake...\r\n", ch);
	else {
		send_to_char("You awaken, and sit up.\r\n", ch);
		act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
	}
}


ACMD(do_worm) {
	int dir;
	room_data *to_room;

	one_argument(argument, arg);

	if (IS_NPC(ch)) {
		msg_to_char(ch, "NPCs cannot worm.\r\n");
	}
	else if (!can_use_ability(ch, ABIL_WORM, NOTHING, 0, NOTHING)) {
		return;
	}
	else if (!AFF_FLAGGED(ch, AFF_EARTHMELD))
		msg_to_char(ch, "You aren't even earthmelded!\r\n");
	else if (IS_INSIDE(IN_ROOM(ch)) || IS_ADVENTURE_ROOM(IN_ROOM(ch))) {
		// map only
		msg_to_char(ch, "You can't do that here.\r\n");
	}
	else if (!*arg)
		msg_to_char(ch, "Which way would you like to move?\r\n");
	else if (is_abbrev(arg, "look"))
		look_at_room(ch);
	else if ((dir = parse_direction(ch, arg)) == NO_DIR)
		msg_to_char(ch, "That's not a direction!\r\n");
	else if (dir >= NUM_2D_DIRS) {
		msg_to_char(ch, "You can't go that way from here!\r\n");
	}
	else if (!(to_room = real_shift(IN_ROOM(ch), shift_dir[dir][0], shift_dir[dir][1])))
		msg_to_char(ch, "You can't go that way!\r\n");
	else if (ROOM_SECT_FLAGGED(to_room, SECTF_FRESH_WATER | SECTF_OCEAN | SECTF_SHALLOW_WATER) || SECT_FLAGGED(ROOM_ORIGINAL_SECT(to_room), SECTF_FRESH_WATER | SECTF_OCEAN | SECTF_SHALLOW_WATER))
		msg_to_char(ch, "You can't pass through the water!\r\n");
	else if (GET_MOVE(ch) < 1)
		msg_to_char(ch, "You don't have enough energy left to do that.\r\n");
	else if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_WORM)) {
		return;
	}
	else {
		do_simple_move(ch, dir, to_room, TRUE, MOVE_EARTHMELD);
		gain_ability_exp(ch, ABIL_WORM, 1);
		
		// on top of any wait from the move itself
		GET_WAIT_STATE(ch) += 1 RL_SEC;
	}
}

