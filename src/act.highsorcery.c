/* ************************************************************************
*   File: act.highsorcery.c                               EmpireMUD 2.0b3 *
*  Usage: implementation for high sorcery abilities                       *
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
*   Data
*   Helpers
*   Commands
*   Rituals
*   Study Stuff
*/

// external vars

// external funcs
extern bool trigger_counterspell(char_data *ch);	// spells.c
void trigger_distrust_from_hostile(char_data *ch, empire_data *emp);	// fight.c

// locals
void send_ritual_messages(char_data *ch, int rit, int pos);


 //////////////////////////////////////////////////////////////////////////////
//// DATA ////////////////////////////////////////////////////////////////////

// for do_disenchant
// TODO updates for this:
//  - this should be moved to a live config for general disenchant -> possibly global interactions
//  - add the ability to set disenchant results by gear or scaled level
//  - add an obj interaction to allow custom disenchant results
const struct {
	obj_vnum vnum;
	double chance;
} disenchant_data[] = {
	{ o_LIGHTNING_STONE, 5 },
	{ o_BLOODSTONE, 5 },
	{ o_IRIDESCENT_IRIS, 5 },
	{ o_GLOWING_SEASHELL, 5 },

	// this must go last
	{ NOTHING, 0.0 }
};


// for enchantments:
#define ENCHANT_LINE_BREAK  { "\t", 0, NO_ABIL,  APPLY_NONE, APPLY_NONE,  { END_RESOURCE_LIST } }

// enchantment flags
#define ENCH_WEAPON  BIT(0)
#define ENCH_ARMOR  BIT(1)
#define ENCH_SHIELD  BIT(2)
#define ENCH_PACK  BIT(3)
#define ENCH_OFFHAND  BIT(4)


// master enchantment data
// TODO this could be moved to live data; will need a whole enchantment editor suite -pc
struct {
	char *name;
	bitvector_t flags;
	int ability;
	
	int first_bonus;	// APPLY_x
	int second_bonus;	// APPLY_x
		
	Resource resources[5];
} enchant_data[] = {

	// weapons
	{ "striking", ENCH_WEAPON, ABIL_ENCHANT_WEAPONS, APPLY_STRENGTH, APPLY_TO_HIT, { { o_BLOODSTONE, 5 }, END_RESOURCE_LIST } },
	{ "parry", ENCH_WEAPON, ABIL_ENCHANT_WEAPONS, APPLY_DEXTERITY, APPLY_NONE, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	{ "wisdom", ENCH_WEAPON, ABIL_ENCHANT_WEAPONS, APPLY_INTELLIGENCE, APPLY_MANA_REGEN, { { o_GLOWING_SEASHELL, 5 }, END_RESOURCE_LIST } },
	{ "accuracy", ENCH_WEAPON, ABIL_ENCHANT_WEAPONS, APPLY_TO_HIT, APPLY_NONE, { { o_BLOODSTONE, 5 }, END_RESOURCE_LIST } },
	{ "awareness", ENCH_WEAPON, ABIL_ENCHANT_WEAPONS, APPLY_WITS, APPLY_TO_HIT, { { o_IRIDESCENT_IRIS, 5 }, END_RESOURCE_LIST } },
	{ "deflection", ENCH_WEAPON, ABIL_ENCHANT_WEAPONS, APPLY_DODGE, APPLY_BLOCK, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },

	
	ENCHANT_LINE_BREAK,
	
	// offhand
	{ "manaspring", ENCH_OFFHAND, ABIL_ENCHANT_WEAPONS, APPLY_MANA_REGEN, APPLY_MANA, { { o_IRIDESCENT_IRIS, 5 }, END_RESOURCE_LIST } },
	{ "healing", ENCH_OFFHAND, ABIL_ENCHANT_WEAPONS, APPLY_BONUS_HEALING, APPLY_MANA_REGEN, { { o_GLOWING_SEASHELL, 5 }, END_RESOURCE_LIST } },
	{ "fury", ENCH_OFFHAND, ABIL_ENCHANT_WEAPONS, APPLY_BONUS_MAGICAL, APPLY_MANA_REGEN, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	{ "venom", ENCH_OFFHAND, ABIL_ENCHANT_WEAPONS, APPLY_BONUS_PHYSICAL, APPLY_MOVE_REGEN, { { o_BLOODSTONE, 5 }, END_RESOURCE_LIST } },
	
	ENCHANT_LINE_BREAK,
	
	// armor
	{ "health", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_HEALTH, APPLY_NONE, { { o_BLOODSTONE, 5 }, END_RESOURCE_LIST } },
	{ "mana", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_MANA, APPLY_NONE, { { o_IRIDESCENT_IRIS, 5 }, END_RESOURCE_LIST } },
	{ "stamina", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_MOVE, APPLY_NONE, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	{ "regeneration", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_HEALTH_REGEN, APPLY_NONE, { { o_IRIDESCENT_IRIS, 5 }, END_RESOURCE_LIST } },
	{ "longrunning", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_MOVE_REGEN, APPLY_NONE, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	{ "energy", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_MANA_REGEN, APPLY_NONE, { { o_IRIDESCENT_IRIS, 5 }, END_RESOURCE_LIST } },
	{ "protection", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_RESIST_PHYSICAL, APPLY_NONE, { { o_GLOWING_SEASHELL, 5 }, END_RESOURCE_LIST } },
	{ "evasion", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_DODGE, APPLY_NONE, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	
	ENCHANT_LINE_BREAK,
	
	{ "rage", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_STRENGTH, APPLY_NONE, { { o_BLOODSTONE, 5 }, END_RESOURCE_LIST } },
	{ "power", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_INTELLIGENCE, APPLY_NONE, { { o_GLOWING_SEASHELL, 5 }, END_RESOURCE_LIST } },
	{ "spellwarding", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_RESIST_MAGICAL, APPLY_NONE, { { o_IRIDESCENT_IRIS, 5 }, END_RESOURCE_LIST } },
	{ "focus", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_TO_HIT, APPLY_NONE, { { o_BLOODSTONE, 5 }, END_RESOURCE_LIST } },
	{ "acrobatics", ENCH_ARMOR, ABIL_ENCHANT_ARMOR, APPLY_DEXTERITY, APPLY_NONE, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	
	ENCHANT_LINE_BREAK,
	
	// shields
	{ "blocking", ENCH_SHIELD, ABIL_ENCHANT_ARMOR, APPLY_BLOCK, APPLY_NONE, { { o_LIGHTNING_STONE, 5 }, END_RESOURCE_LIST } },
	
	ENCHANT_LINE_BREAK,
	
	{ "embiggening", ENCH_PACK, ABIL_ENCHANT_TOOLS, APPLY_INVENTORY, APPLY_NONE, { { o_IRIDESCENT_IRIS, 1 }, { o_GLOWING_SEASHELL, 1 }, { o_BLOODSTONE, 1 }, { o_LIGHTNING_STONE, 1 }, END_RESOURCE_LIST } },
	
	// END
	{ "\n", 0, NO_ABIL,  APPLY_NONE, APPLY_NONE,  { END_RESOURCE_LIST } }
};


/**
* Ritual setup function: for rituals that take arguments or must pre-validate
* anything. If your ritual doesn't require this, use start_simple_ritual as
* your setup function.
*
* You must call start_ritual(ch, ritual) and return TRUE at the end of setup.
* If you need to store data, you can use action vnums 1 and 2; 0 will be the
* ritual number. You can only add action vnum data AFTER calling start_ritual.
*
* @param char_data *ch The person performing the ritual.
* @param char *argument The argument (text after the ritual name).
* @param int ritual The position in the ritual_data[] table.
* @return bool Returns TRUE if the ritual started, or FALSE if there was an error.
*/
#define RITUAL_SETUP_FUNC(name)  bool (name)(char_data *ch, char *argument, int ritual)


/**
* Ritual finish function: This is called when the ritual is complete, and
* should contain the actual functionality of the ritual.
*
* @param char_data *ch The player performing the ritual.
* @param int ritual The position in the ritual_data[] table.
*/
#define RITUAL_FINISH_FUNC(name)  void (name)(char_data *ch, int ritual)


// ritual flags
// TODO these are not currently used but should maybe go in structs.h
#define RIT_FLAG  BIT(0)

// misc
#define NO_MESSAGE	{ "\t", "\t" }	// skip a tick (5 seconds)
#define MESSAGE_END  { "\n", "\n" }  // end of sequence (one final tick to get here)

// ritual prototypes
RITUAL_FINISH_FUNC(perform_devastation_ritual);
RITUAL_FINISH_FUNC(perform_phoenix_rite);
RITUAL_FINISH_FUNC(perform_ritual_of_burdens);
RITUAL_FINISH_FUNC(perform_ritual_of_defense);
RITUAL_FINISH_FUNC(perform_ritual_of_detection);
RITUAL_FINISH_FUNC(perform_ritual_of_teleportation);
RITUAL_FINISH_FUNC(perform_sense_life_ritual);
RITUAL_FINISH_FUNC(perform_siege_ritual);
RITUAL_SETUP_FUNC(start_devastation_ritual);
RITUAL_SETUP_FUNC(start_ritual_of_defense);
RITUAL_SETUP_FUNC(start_ritual_of_detection);
RITUAL_SETUP_FUNC(start_ritual_of_teleportation);
RITUAL_SETUP_FUNC(start_siege_ritual);
RITUAL_SETUP_FUNC(start_simple_ritual);


// Ritual data
// TODO this could all be moved to live data, but would need a lot of support and a custom editor
struct ritual_data_type {
	char *name;
	int cost;
	int ability;
	bitvector_t flags;
	RITUAL_SETUP_FUNC(*begin);	// function pointer
	RITUAL_FINISH_FUNC(*perform);	// function pointer
	struct ritual_strings strings[24];	// one sent per tick, terminate with MESSAGE_END
} ritual_data[] = {
	// 0: ritual of burdens
	{ "burdens", 5, ABIL_RITUAL_OF_BURDENS, 0,
		start_simple_ritual,
		perform_ritual_of_burdens,
		{{ "You whisper your burdens into the air...", "$n whispers $s burdens into the air..." },
		{ "\n", "\n" }
	}},
	
	// 1: ritual of teleportation
	{ "teleportation", 25, ABIL_RITUAL_OF_TELEPORTATION, 0,
		start_ritual_of_teleportation,
		perform_ritual_of_teleportation,
		{{ "You wrap yourself up and begin chanting...", "$n wraps $mself up and begins chanting..." },
		{ "There is a bright blue glow all around you...", "$n begins to glow bright blue, like a flame..." },
		MESSAGE_END
	}},
	
	// 2: phoenix rite
	{ "phoenix", 25, ABIL_PHOENIX_RITE, 0,
		start_simple_ritual,
		perform_phoenix_rite,
		{{ "You light some candles and begin the Phoenix Rite.", "$n lights some candles." },
		{ "You sit and place the candles in a circle around you.", "$n sits and places the candles in a circle around $mself." },
		NO_MESSAGE,
		{ "You focus on the flame of the candles...", "$n moves back and forth as if in a trance." },
		NO_MESSAGE,
		{ "You reach out to the sides and grab the flame of the candles in your hands!", "$n's hands shoot out to the sides and grab the flames of the candles!" },
		MESSAGE_END
	}},
	
	// 3: ritual of defense
	{ "defense", 15, ABIL_RITUAL_OF_DEFENSE, 0,
		start_ritual_of_defense,
		perform_ritual_of_defense,
		{{ "You begin the incantation for the Ritual of Defense.", "\t" },
		{ "You say, 'Empower now this wall of stone'", "$n says, 'Empower now this wall of stone'" },
		{ "You say, 'With wisdom of the sages'", "$n says, 'With wisdom of the sages'" },
		{ "You say, 'Ward against the foes above'", "$n says, 'Ward against the foes above'" },
		{ "You say, 'And strong, withstand the ages'", "$n says, 'And strong, withstand the ages'" },
		MESSAGE_END
	}},
	
	// 4: sense life ritual
	{ "senselife", 15, ABIL_SENSE_LIFE_RITUAL, 0,
		start_simple_ritual,
		perform_sense_life_ritual,
		{{ "You murmur the words to the Sense Life Ritual...", "$n murmurs some arcane words..." },
		MESSAGE_END
	}},
	
	// 5: ritual of detection
	{ "detection", 15, ABIL_RITUAL_OF_DETECTION, 0,
		start_ritual_of_detection,
		perform_ritual_of_detection,
		{{ "You pull a piece of chalk from your pocket and begin drawing circles on the ground.", "$n pulls out a piece of chalk and begins drawing circles on the ground." },
		NO_MESSAGE,
		{ "You make crossed lines through the circles you've drawn.", "$n makes crossed lines through the circles $e has drawn." },
		NO_MESSAGE,
		{ "You begin writing the words 'where', 'find', and 'reveal' on the ground.", "$n begins writing words in the circles on the ground." },
		MESSAGE_END
	}},
	
	// 6: siege ritual
	{ "siege", 30, ABIL_SIEGE_RITUAL, 0,
		start_siege_ritual,
		perform_siege_ritual,
		{{ "You summon the full force of your magic...", "$n begins drawing mana to $mself..." },
		NO_MESSAGE,
		{ "You shoot a fireball at the building...", "$n shoots a fireball at the building..." },
		NO_MESSAGE,
		{ "You draw more mana...", "$n draws more mana..." },
		NO_MESSAGE,
		{ "You shoot a fireball at the building...", "$n shoots a fireball at the building..." },
		NO_MESSAGE,
		MESSAGE_END
	}},
	
	// 7: devastation ritual
	{ "devastation", 15, ABIL_DEVASTATION_RITUAL, 0,
		start_devastation_ritual,
		perform_devastation_ritual,
		{{ "You plant your staff hard into the ground and begin focusing mana...", "$n plants $s staff hard into the ground and begins drawing mana toward $mself..." },
		NO_MESSAGE,
		{ "You send out shockwaves of mana from your staff...", "$n's staff sends out shockwaves of mana..." },
		NO_MESSAGE,
		MESSAGE_END
	}},
	
	{ "\n", 0, NO_ABIL, 0, NULL, NULL, { MESSAGE_END } },
};


 //////////////////////////////////////////////////////////////////////////////
//// HELPERS /////////////////////////////////////////////////////////////////

/**
* @param obj_data *obj
* @param int type enchant_data pos
* @return TRUE if the enchantment could apply to the object
*/
bool can_be_enchanted(obj_data *obj, int type) {
	if (GET_OBJ_CURRENT_SCALE_LEVEL(obj) <= 0) {
		// unscaled items cannot be enchanted
		return FALSE;
	}
	if (IS_SET(enchant_data[type].flags, ENCH_WEAPON) && IS_WEAPON(obj)) {
		return TRUE;
	}
	if (IS_SET(enchant_data[type].flags, ENCH_ARMOR) && IS_ARMOR(obj) && CAN_WEAR(obj, ITEM_WEAR_ARMOR)) {
		return TRUE;
	}
	if (IS_SET(enchant_data[type].flags, ENCH_SHIELD) && IS_SHIELD(obj)) {
		return TRUE;
	}
	if (IS_SET(enchant_data[type].flags, ENCH_PACK) && IS_PACK(obj)) {
		return TRUE;
	}
	if (IS_SET(enchant_data[type].flags, ENCH_OFFHAND) && CAN_WEAR(obj, ITEM_WEAR_HOLD)) {
		return TRUE;
	}
	
	// Nope.
	return FALSE;
}


/**
* @param char_data *ch The player trying to perform a ritual.
* @param int ritual The position in the ritual_data[] table.
*/
bool can_use_ritual(char_data *ch, int ritual) {
	if (IS_NPC(ch)) {
		return FALSE;
	}
	if (ritual_data[ritual].ability != NO_ABIL && !HAS_ABILITY(ch, ritual_data[ritual].ability)) {
		return FALSE;
	}
	
//	if (IS_SET(ritual_data[ritual].flags, RIT_HENGE) && (!IS_COMPLETE(IN_ROOM(ch)) || BUILDING_VNUM(IN_ROOM(ch)) != BUILDING_HENGE)) {
//		return FALSE;
//	}
	
	return TRUE;
}


// for devastation_ritual
INTERACTION_FUNC(devastate_crop) {
	void scale_item_to_level(obj_data *obj, int level);
	
	crop_data *cp = crop_proto(ROOM_CROP_TYPE(inter_room));
	obj_data *newobj;
	int num;

	num = number(1, 4) * interaction->quantity;
	
	msg_to_char(ch, "You devastate the %s and collect %s (x%d)!\r\n", GET_CROP_NAME(cp), get_obj_name_by_proto(interaction->vnum), num);
	sprintf(buf, "$n's powerful ritual devastates the %s crops!", GET_CROP_NAME(cp));
	act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
	
	while (num-- > 0) {
		obj_to_char_or_room((newobj = read_object(interaction->vnum, TRUE)), ch);
		scale_item_to_level(newobj, 1);	// minimum level
		load_otrigger(newobj);
	}
	
	// additional tree if orchard
	if (ROOM_CROP_FLAGGED(inter_room, CROPF_IS_ORCHARD)) {
		obj_to_char_or_room((newobj = read_object(o_TREE, TRUE)), ch);
		scale_item_to_level(newobj, 1);	// minimum level
		load_otrigger(newobj);
	}
	
	return TRUE;
}


/**
* Name matching for enchantments, preferring exact matches first.
*
* @param char_data *ch the player, to check abilities
* @param char *name enchantment name to find
* @return int enchant_data position, or NOTHING for not found
*/
int find_enchant_by_name(char_data *ch, char *name) {
	int iter, exact = NOTHING, partial = NOTHING;
	
	for (iter = 0; *enchant_data[iter].name != '\n' && exact == NOTHING; ++iter) {
		if ((enchant_data[iter].ability == NO_ABIL || HAS_ABILITY(ch, enchant_data[iter].ability))) {
			if (!str_cmp(name, enchant_data[iter].name)) {
				exact = iter;
			}
			else if (is_abbrev(name, enchant_data[iter].name)) {
				partial = iter;
			}
		}
	}
	
	if (exact == NOTHING) {
		exact = partial;
	}
	
	return exact;
}


/**
* This function is called by the action message each update tick, for a person
* who is performing a ritual. That's why it's called that.
*
* @param char_data *ch The person performing the ritual.
*/
void perform_ritual(char_data *ch) {	
	int rit = GET_ACTION_VNUM(ch, 0);
	
	GET_ACTION_TIMER(ch) += 1;
	send_ritual_messages(ch, rit, GET_ACTION_TIMER(ch));
	
	// this triggers the end of the ritual, based on MESSAGE_END
	if (*ritual_data[rit].strings[GET_ACTION_TIMER(ch)].to_char == '\n') {
		GET_ACTION(ch) = ACT_NONE;
		
		if (ritual_data[rit].perform) {
			(ritual_data[rit].perform)(ch, rit);
		}
		else {
			msg_to_char(ch, "You complete the ritual but it doesn't seem to be implemented.\r\n");
		}
	}
}


/**
* Check and send messages for a ritual tick
*
* @param char_data *ch actor
* @param int rit The ritual_data number
* @param int pos Number of ticks into the ritual
*/
void send_ritual_messages(char_data *ch, int rit, int pos) {
	// the very first message (pos==0) must be sent
	int nospam = (pos == 0 ? 0 : TO_SPAMMY);

	if (*ritual_data[rit].strings[pos].to_char != '\t' && *ritual_data[rit].strings[pos].to_char != '\n') {
		act(ritual_data[rit].strings[pos].to_char, FALSE, ch, NULL, NULL, TO_CHAR | nospam);
	}
	if (*ritual_data[rit].strings[pos].to_room != '\t' && *ritual_data[rit].strings[pos].to_room != '\n') {
		act(ritual_data[rit].strings[pos].to_room, FALSE, ch, NULL, NULL, TO_ROOM | nospam);
	}
}


/**
* Sets up the ritual action and sends first message.
*
* @param char_data *ch The player.
* @param int ritual The ritual (position in ritual_data)
*/
void start_ritual(char_data *ch, int ritual) {
	if (!ch || IS_NPC(ch)) {
		return;
	}
	
	// first message
	send_ritual_messages(ch, ritual, 0);
	
	start_action(ch, ACT_RITUAL, 0);
	GET_ACTION_VNUM(ch, 0) = ritual;
}


/**
* Command processing for the "summon materials" ability, called via the
* central do_summon command.
*
* @param char_data *ch The person using the command.
* @param char *argument The typed arg.
*/
void summon_materials(char_data *ch, char *argument) {
	void sort_storage(empire_data *emp);
	void read_vault(empire_data *emp);

	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], *objname;
	struct empire_storage_data *store, *next_store;
	int count = 0, total = 1, number, pos;
	empire_data *emp;
	int cost = 2;	// * number of things to summon
	obj_data *proto;
	bool found = FALSE;

	half_chop(argument, arg1, arg2);
	
	if (!(emp = GET_LOYALTY(ch))) {
		msg_to_char(ch, "You can't summon empire materials if you're not in an empire.\r\n");
		return;
	}
	if (GET_RANK(ch) < EMPIRE_PRIV(emp, PRIV_STORAGE)) {
		msg_to_char(ch, "You aren't high enough rank to retrieve from the empire inventory.\r\n");
		return;
	}
	
	if (GET_ISLAND_ID(IN_ROOM(ch)) == NO_ISLAND) {
		msg_to_char(ch, "You can't summon materials here.\r\n");
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_SUMMON_MATERIALS)) {
		return;
	}
	
	// pull out a number if the first arg is a number
	if (*arg1 && is_number(arg1)) {
		total = atoi(arg1);
		if (total < 1) {
			msg_to_char(ch, "You have to summon at least 1.\r\n");
			return;
			}
		strcpy(arg1, arg2);
	}
	else if (*arg1 && *arg2) {
		// re-combine if it wasn't a number
		sprintf(arg1 + strlen(arg1), " %s", arg2);
	}
	
	// arg1 now holds the desired name
	objname = arg1;
	number = get_number(&objname);

	// multiply cost for total, but don't store it
	if (!can_use_ability(ch, ABIL_SUMMON_MATERIALS, MANA, cost * total, NOTHING)) {
		return;
	}

	if (!*objname) {
		msg_to_char(ch, "What material would you like to summon (use einventory to see what you have)?\r\n");
		return;
	}
	
	msg_to_char(ch, "You open a tiny portal to summon materials...\r\n");
	act("$n opens a tiny portal to summon materials...", FALSE, ch, NULL, NULL, TO_ROOM);
	
	// sort first
	sort_storage(emp);

	pos = 0;
	for (store = EMPIRE_STORAGE(emp); !found && store; store = next_store) {
		next_store = store->next;
		
		// island check
		if (store->island != GET_ISLAND_ID(IN_ROOM(ch))) {
			continue;
		}
		
		proto = obj_proto(store->vnum);
		if (proto && multi_isname(objname, GET_OBJ_KEYWORDS(proto)) && (++pos == number)) {
			found = TRUE;
			
			if (stored_item_requires_withdraw(proto)) {
				msg_to_char(ch, "You can't summon materials out of the vault.\r\n");
				return;
			}
			
			while (count < total && store->amount > 0) {
				++count;
				if (!retrieve_resource(ch, emp, store, FALSE)) {
					break;
				}
			}
		}
	}

	// result messages
	if (!found) {
		msg_to_char(ch, "Nothing like that is stored around here.\r\n");
	}
	else if (count == 0) {
		msg_to_char(ch, "There is nothing stored in your empire nearby.\r\n");
	}
	else {
		// save the empire
		if (found) {
			GET_MANA(ch) -= cost * count;	// charge only the amount retrieved
			save_empire(emp);
			read_vault(emp);
			gain_ability_exp(ch, ABIL_SUMMON_MATERIALS, 1);
		}
	}
	
	command_lag(ch, WAIT_OTHER);
}


 //////////////////////////////////////////////////////////////////////////////
//// COMMANDS ////////////////////////////////////////////////////////////////


ACMD(do_collapse) {
	obj_data *portal, *obj, *reverse = NULL;
	room_data *to_room;
	int cost = 15;
	
	one_argument(argument, arg);

	if (!can_use_ability(ch, ABIL_PORTAL_MASTER, MANA, cost, NOTHING)) {
		return;
	}
	
	if (!*arg) {
		msg_to_char(ch, "Collapse which portal?\r\n");
		return;
	}
	
	if (!(portal = get_obj_in_list_vis(ch, arg, ROOM_CONTENTS(IN_ROOM(ch))))) {
		msg_to_char(ch, "You don't see a %s here.\r\n", arg);
		return;
	}
	
	if (!IS_PORTAL(portal) || GET_OBJ_TIMER(portal) == UNLIMITED) {
		msg_to_char(ch, "You can't collapse that.\r\n");
		return;
	}
	
	if (!(to_room = real_room(GET_PORTAL_TARGET_VNUM(portal)))) {
		msg_to_char(ch, "You can't collapse that.\r\n");
		return;
	}
	
	// find the reverse portal
	for (obj = ROOM_CONTENTS(to_room); obj && !reverse; obj = obj->next_content) {
		if (GET_PORTAL_TARGET_VNUM(obj) == GET_ROOM_VNUM(IN_ROOM(ch))) {
			reverse = obj;
		}
	}

	// do it
	charge_ability_cost(ch, MANA, cost, NOTHING, 0, WAIT_SPELL);
	
	act("You grab $p and draw it shut!", FALSE, ch, portal, NULL, TO_CHAR);
	act("$n grabs $p and draws it shut!", FALSE, ch, portal, NULL, TO_ROOM);
	
	extract_obj(portal);
	
	if (reverse) {
		if (ROOM_PEOPLE(to_room)) {
			act("$p is collapsed from the other side!", FALSE, ROOM_PEOPLE(to_room), reverse, NULL, TO_CHAR | TO_ROOM);
		}
		extract_obj(reverse);
	}
	
	gain_ability_exp(ch, ABIL_PORTAL_MASTER, 20);
}


ACMD(do_colorburst) {
	char_data *vict = NULL;
	struct affected_type *af;
	int amt;
	
	int costs[] = { 15, 30, 45 };
	int levels[] = { -5, -10, -15 };
	
	if (!can_use_ability(ch, ABIL_COLORBURST, MANA, CHOOSE_BY_ABILITY_LEVEL(costs, ch, ABIL_COLORBURST), COOLDOWN_COLORBURST)) {
		return;
	}
	
	// find target
	one_argument(argument, arg);
	if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
		return;
	}
	if (!*arg && !(vict = FIGHTING(ch))) {
		msg_to_char(ch, "Who would you like to cast that at?\r\n");
		return;
	}
	if (ch == vict) {
		msg_to_char(ch, "You wouldn't want to cast that on yourself.\r\n");
		return;
	}
	
	// check validity
	if (!can_fight(ch, vict)) {
		act("You can't attack $M!", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_COLORBURST)) {
		return;
	}
	
	charge_ability_cost(ch, MANA, CHOOSE_BY_ABILITY_LEVEL(costs, ch, ABIL_COLORBURST), COOLDOWN_COLORBURST, 30, WAIT_COMBAT_SPELL);
	
	if (SHOULD_APPEAR(ch)) {
		appear(ch);
	}
	
	// counterspell??
	if (trigger_counterspell(vict) || AFF_FLAGGED(vict, AFF_IMMUNE_HIGH_SORCERY)) {
		act("You fire a burst of color at $N, but $E deflects it!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n fires a burst of color at you, but it's deflected by your counterspell!", FALSE, ch, NULL, vict, TO_VICT);
		act("$n fires a burst of color at $N, but $E deflects it.", FALSE, ch, NULL, vict, TO_NOTVICT);
	}
	else {
		// succeed
	
		act("You whip your hand forward and fire a burst of color at $N!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n whips $s hand forward and fires a burst of color at you!", FALSE, ch, NULL, vict, TO_VICT);
		act("$n whips $s hand forward and fires a burst of color at $N!", FALSE, ch, NULL, vict, TO_NOTVICT);
		
		amt = CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_COLORBURST) - GET_INTELLIGENCE(ch);
	
		af = create_mod_aff(ATYPE_COLORBURST, 6, APPLY_TO_HIT, amt, ch);
		affect_join(vict, af, 0);

		engage_combat(ch, vict, FALSE);
	}

	gain_ability_exp(ch, ABIL_COLORBURST, 15);
}


ACMD(do_disenchant) {
	obj_data *obj, *reward, *proto;
	int iter, prc, rnd;
	obj_vnum vnum = NOTHING;
	int cost = 5;
	
	one_argument(argument, arg);
	
	if (!can_use_ability(ch, ABIL_DISENCHANT, MANA, cost, NOTHING)) {
		// sends its own messages
	}
	else if (!*arg) {
		msg_to_char(ch, "Disenchant what?\r\n");
	}
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		msg_to_char(ch, "You seem to have lost your %s.\r\n", arg);
	}
	else if (ABILITY_TRIGGERS(ch, NULL, obj, ABIL_DISENCHANT)) {
		return;
	}
	else if (!bind_ok(obj, ch)) {
		msg_to_char(ch, "You can't disenchant something that is bound to someone else.\r\n");
	}
	else if (!OBJ_FLAGGED(obj, OBJ_ENCHANTED)) {
		act("$p is not even enchanted.", FALSE, ch, obj, NULL, TO_CHAR);
	}
	else {
		charge_ability_cost(ch, MANA, cost, NOTHING, 0, WAIT_SPELL);
		REMOVE_BIT(GET_OBJ_EXTRA(obj), OBJ_ENCHANTED | OBJ_SUPERIOR);
		proto = obj_proto(GET_OBJ_VNUM(obj));
		
		for (iter = 0; iter < MAX_OBJ_AFFECT; ++iter) {
			if (!proto || OBJ_FLAGGED(proto, OBJ_ENCHANTED)) {
				// if the protype is enchanted, remove them all
				obj->affected[iter].location = 0;
				obj->affected[iter].modifier = 0;
			}
			else {
				// otherwise just match the proto
				obj->affected[iter].location = proto->affected[iter].location;
				obj->affected[iter].modifier = proto->affected[iter].modifier;
			}
		}
		
		act("You hold $p over your head and shout 'KA!' as you release the mana bound to it!\r\nThere is a burst of red light from $p and then it fizzles and is disenchanted.", FALSE, ch, obj, NULL, TO_CHAR);
		act("$n shouts 'KA!' and cracks $p, which blasts out red light, and then fizzles.", FALSE, ch, obj, NULL, TO_ROOM);
		gain_ability_exp(ch, ABIL_DISENCHANT, 33.4);
		
		// obj back?
		if (skill_check(ch, ABIL_DISENCHANT, DIFF_MEDIUM)) {
			rnd = number(1, 10000);
			prc = 0;
			for (iter = 0; disenchant_data[iter].vnum != NOTHING && vnum == NOTHING; ++iter) {
				prc += disenchant_data[iter].chance * 100;
				if (rnd <= prc) {
					vnum = disenchant_data[iter].vnum;
				}
			}
		
			if (vnum != NOTHING) {
				reward = read_object(vnum, TRUE);
				obj_to_char_or_room(reward, ch);
				act("You manage to weave the freed mana into $p!", FALSE, ch, reward, NULL, TO_CHAR);
				act("$n weaves the freed mana into $p!", TRUE, ch, reward, NULL, TO_ROOM);
				load_otrigger(reward);
			}
		}
	}
}


ACMD(do_dispel) {
	struct over_time_effect_type *dot, *next_dot;
	char_data *vict = ch;
	int cost = 30;
	bool found;
	
	one_argument(argument, arg);
	
	
	if (!can_use_ability(ch, ABIL_DISPEL, MANA, cost, COOLDOWN_DISPEL)) {
		return;
	}
	else if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
	}
	else if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_DISPEL)) {
		return;
	}
	else {
		// verify they have dots
		found = FALSE;
		for (dot = vict->over_time_effects; dot && !found; dot = dot->next) {
			if (dot->damage_type == DAM_MAGICAL || dot->damage_type == DAM_FIRE) {
				found = TRUE;
			}
		}
		if (!found) {
			if (ch == vict) {
				msg_to_char(ch, "You aren't afflicted by anything that can be dispelled.\r\n");
			}
			else {
				act("$E isn't afflicted by anything that can be dispelled.", FALSE, ch, NULL, vict, TO_CHAR);
			}
			return;
		}
	
		charge_ability_cost(ch, MANA, cost, COOLDOWN_DISPEL, 9, WAIT_SPELL);
		
		if (ch == vict) {
			msg_to_char(ch, "You shout 'KA!' and dispel your afflictions.\r\n");
			act("$n shouts 'KA!' and dispels $s afflictions.", TRUE, ch, NULL, NULL, TO_ROOM);
		}
		else {
			act("You shout 'KA!' and dispel $N's afflictions.", FALSE, ch, NULL, vict, TO_CHAR);
			act("$n shouts 'KA!' and dispels your afflictions.", FALSE, ch, NULL, vict, TO_VICT);
			act("$n shouts 'KA!' and dispels $N's afflictions.", FALSE, ch, NULL, vict, TO_NOTVICT);
		}

		// remove DoTs
		for (dot = vict->over_time_effects; dot; dot = next_dot) {
			next_dot = dot->next;

			if (dot->damage_type == DAM_MAGICAL || dot->damage_type == DAM_FIRE) {
				dot_remove(vict, dot);
			}
		}
		
		gain_ability_exp(ch, ABIL_DISPEL, 33.4);

		if (FIGHTING(vict) && !FIGHTING(ch)) {
			engage_combat(ch, FIGHTING(vict), FALSE);
		}
	}
}


ACMD(do_enchant) {
	extern int get_crafting_level(char_data *ch);
	extern const double apply_values[];
	
	char arg2[MAX_INPUT_LENGTH];
	obj_data *obj;
	int iter, type, scale, charlevel;
	bool found, line, first;		
	double points_available, first_points = 0.0, second_points = 0.0;

	double enchant_points_at_100 = config_get_double("enchant_points_at_100");
	
	two_arguments(argument, arg, arg2);
	
	if (IS_NPC(ch)) {
		msg_to_char(ch, "NPCs can't enchant.\r\n");
	}
	else if (!*arg || !*arg2) {
		msg_to_char(ch, "Usage: enchant <item> <type>\r\nYou know how to enchant:\r\n");
		
		line = FALSE;
		for (iter = 0; *enchant_data[iter].name != '\n'; ++iter) {			
			if (*enchant_data[iter].name == '\t') {
				if (line) {
					msg_to_char(ch, "\r\n");
					line = FALSE;
				}
			}
			else if (enchant_data[iter].ability == NO_ABIL || HAS_ABILITY(ch, enchant_data[iter].ability)) {
				msg_to_char(ch, "%s%s", line ? ", " : "  ", enchant_data[iter].name);
				found = line = TRUE;
			}
		}
		
		if (line) {
			msg_to_char(ch, "\r\n");
		}
		if (!found) {
			msg_to_char(ch, "  nothing\r\n");
		}
		// end !*arg
	}
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)) && !(obj = get_obj_in_list_vis(ch, arg, ROOM_CONTENTS(IN_ROOM(ch))))) {
		msg_to_char(ch, "You don't seem to have any %s.\r\n", arg);
	}
	else if ((type = find_enchant_by_name(ch, arg2)) == NOTHING) {
		msg_to_char(ch, "You don't know that enchantment.\r\n");
	}
	else if (enchant_data[type].ability != NO_ABIL && ABILITY_TRIGGERS(ch, NULL, obj, enchant_data[type].ability)) {
		return;
	}
	else if (OBJ_FLAGGED(obj, OBJ_ENCHANTED)) {
		msg_to_char(ch, "You cannot enchant an item that has already been enchanted.\r\n");
	}
	else if (GET_OBJ_CURRENT_SCALE_LEVEL(obj) <= 0) {
		// always forbidden
		msg_to_char(ch, "That item cannot be enchanted.\r\n");
	}
	else if (!can_be_enchanted(obj, type)) {
		msg_to_char(ch, "You can't place that enchantment on that object.\r\n");
	}
	else if (!has_resources(ch, enchant_data[type].resources, FALSE, TRUE)) {
		// sends its own messages
	}
	else {
		extract_resources(ch, enchant_data[type].resources, FALSE);
		
		// enchant scale level is whichever is less: obj scale level, or player crafting level
		charlevel = MAX(get_crafting_level(ch), get_approximate_level(ch));
		scale = MIN(GET_OBJ_CURRENT_SCALE_LEVEL(obj), charlevel);
		points_available = MAX(1.0, scale / 100.0 * enchant_points_at_100);
		
		if (HAS_ABILITY(ch, ABIL_GREATER_ENCHANTMENTS)) {
			points_available *= config_get_double("greater_enchantments_bonus");
		}
		
		// split the points
		if (enchant_data[type].second_bonus != APPLY_NONE) {
			first_points = points_available * 0.6666;
			second_points = points_available * 0.3333;
		}
		else {
			// only one apply
			first_points = points_available;
			second_points = 0.0;
		}

		first = TRUE;
		
		// find free applies to use
		for (iter = 0; iter < MAX_OBJ_AFFECT; ++iter) {
			if (obj->affected[iter].location == APPLY_NONE) {
				if (first) {
					// always get at least 1 from first bonus
					int value = MAX(1, round(first_points * (1.0 / apply_values[enchant_data[type].first_bonus])));
					
					obj->affected[iter].location = enchant_data[type].first_bonus;
					obj->affected[iter].modifier = value;
					first = FALSE;
				}
				else if (enchant_data[type].second_bonus != APPLY_NONE) {
					int value = round(second_points * (1.0 / apply_values[enchant_data[type].second_bonus]));
					
					// only bother if any points to give
					if (value > 0) {
						obj->affected[iter].location = enchant_data[type].second_bonus;
						obj->affected[iter].modifier = value;
					}
					// second apply done
					break;
				}
			}
		}
		
		// set enchanted bit
		SET_BIT(GET_OBJ_EXTRA(obj), OBJ_ENCHANTED);

		sprintf(buf, "You infuse $p with the %s enchantment.", enchant_data[type].name);
		act(buf, FALSE, ch, obj, NULL, TO_CHAR);
		
		sprintf(buf, "$n infuses $p with the %s enchantment.", enchant_data[type].name);
		act(buf, FALSE, ch, obj, NULL, TO_ROOM);
				
		if (enchant_data[type].ability != NO_ABIL) {
			gain_ability_exp(ch, enchant_data[type].ability, 50);
		}
		gain_ability_exp(ch, ABIL_GREATER_ENCHANTMENTS, 50);
		
		command_lag(ch, WAIT_ABILITY);
	}
}


ACMD(do_enervate) {
	char_data *vict = NULL;
	struct affected_type *af, *af2;
	int cost = 20;
	
	int levels[] = { 1, 1, 3 };
		
	if (!can_use_ability(ch, ABIL_ENERVATE, MANA, cost, COOLDOWN_ENERVATE)) {
		return;
	}
	
	// find target
	one_argument(argument, arg);
	if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
		return;
	}
	if (!*arg && !(vict = FIGHTING(ch))) {
		msg_to_char(ch, "Who would you like to cast that at?\r\n");
		return;
	}
	if (ch == vict) {
		msg_to_char(ch, "You wouldn't want to cast that on yourself.\r\n");
		return;
	}
	
	// check validity
	if (!can_fight(ch, vict)) {
		act("You can't attack $M!", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_ENERVATE)) {
		return;
	}
	
	charge_ability_cost(ch, MANA, cost, COOLDOWN_ENERVATE, SECS_PER_MUD_HOUR, WAIT_COMBAT_SPELL);
	
	if (SHOULD_APPEAR(ch)) {
		appear(ch);
	}
	
	// counterspell??
	if (trigger_counterspell(vict) || AFF_FLAGGED(vict, AFF_IMMUNE_HIGH_SORCERY)) {
		act("You attempt to hex $N with enervate, but it fails!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n attempts to hex you with enervate, but it's deflected by your counterspell!", FALSE, ch, NULL, vict, TO_VICT);
		act("$n attempts to hex $N with enervate, but it fails!", FALSE, ch, NULL, vict, TO_NOTVICT);
	}
	else {
		// succeed
	
		act("$N starts to glow red as you shout the enervate hex at $M! You feel your own stamina grow as you drain $S.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n shouts somthing at you... The world takes on a reddish hue and you feel your stamina drain.", FALSE, ch, NULL, vict, TO_VICT);
		act("$n shouts some kind of hex at $N, who starts to glow red and seems weakened!", FALSE, ch, NULL, vict, TO_NOTVICT);
	
		af = create_mod_aff(ATYPE_ENERVATE, 1 MUD_HOURS, APPLY_MOVE_REGEN, -1 * GET_INTELLIGENCE(ch) / 2, ch);
		affect_join(vict, af, 0);
		af2 = create_mod_aff(ATYPE_ENERVATE_GAIN, 1 MUD_HOURS, APPLY_MOVE_REGEN, CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_ENERVATE), ch);
		affect_join(ch, af2, 0);

		engage_combat(ch, vict, FALSE);
	}

	gain_ability_exp(ch, ABIL_ENERVATE, 15);
}


ACMD(do_foresight) {
	struct affected_type *af;
	char_data *vict = ch;
	int amt, cost = 30;
	
	int levels[] = { 5, 10, 15 };
	
	one_argument(argument, arg);
	
	if (!can_use_ability(ch, ABIL_FORESIGHT, MANA, cost, NOTHING)) {
		return;
	}
	else if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
	}
	else if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_FORESIGHT)) {
		return;
	}
	else {
		charge_ability_cost(ch, MANA, cost, NOTHING, 0, WAIT_SPELL);
		
		if (ch == vict) {
			msg_to_char(ch, "You pull out a grease pencil and mark each of your eyelids with an X...\r\nYou feel the gift of foresight!\r\n");
			act("$n pulls out a grease pencil and marks each of $s eyelids with an X.", TRUE, ch, NULL, NULL, TO_ROOM);
		}
		else {
			act("You pull out a grease pencil and mark each of $N's eyelids with an X, giving $M the gift of foresight.", FALSE, ch, NULL, vict, TO_CHAR);
			act("$n marks each of your eyelids using a grease pencil...\r\nYou feel the gift of foresight!", FALSE, ch, NULL, vict, TO_VICT);
			act("$n pulls out a grease pencil and marks each of $N's eyelids with an X.", FALSE, ch, NULL, vict, TO_NOTVICT);
		}
		
		amt = CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_FORESIGHT) + GET_INTELLIGENCE(ch);
		
		af = create_mod_aff(ATYPE_FORESIGHT, 12 MUD_HOURS, APPLY_DODGE, amt, ch);
		affect_join(vict, af, 0);
		
		gain_ability_exp(ch, ABIL_FORESIGHT, 15);

		if (FIGHTING(vict) && !FIGHTING(ch)) {
			engage_combat(ch, FIGHTING(vict), FALSE);
		}
	}
}


ACMD(do_manashield) {
	struct affected_type *af1, *af2, *af3;
	int cost = 25;
	int amt;
	
	int levels[] = { 2, 4, 6 };

	if (affected_by_spell(ch, ATYPE_MANASHIELD)) {
		msg_to_char(ch, "You wipe the symbols off your arm and cancel your mana shield.\r\n");
		act("$n wipes the arcane symbols off $s arm.", TRUE, ch, NULL, NULL, TO_ROOM);
		affect_from_char(ch, ATYPE_MANASHIELD);
	}
	else if (!can_use_ability(ch, ABIL_MANASHIELD, MANA, cost, NOTHING)) {
		return;
	}
	else if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_MANASHIELD)) {
		return;
	}
	else {
		charge_ability_cost(ch, MANA, cost, NOTHING, 0, WAIT_SPELL);
		
		msg_to_char(ch, "You pull out a grease pencil and draw a series of arcane symbols down your left arm...\r\nYou feel yourself shielded by your mana!\r\n");
		act("$n pulls out a grease pencil and draws a series of arcane symbols down $s left arm.", TRUE, ch, NULL, NULL, TO_ROOM);
		
		amt = CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_MANASHIELD) + (GET_INTELLIGENCE(ch) / 3);
		
		af1 = create_mod_aff(ATYPE_MANASHIELD, 24 MUD_HOURS, APPLY_MANA, -25, ch);
		af2 = create_mod_aff(ATYPE_MANASHIELD, 24 MUD_HOURS, APPLY_RESIST_PHYSICAL, amt, ch);
		af3 = create_mod_aff(ATYPE_MANASHIELD, 24 MUD_HOURS, APPLY_RESIST_MAGICAL, amt, ch);
		affect_join(ch, af1, 0);
		affect_join(ch, af2, 0);
		affect_join(ch, af3, 0);
		
		// possible to go negative here
		GET_MANA(ch) = MAX(0, GET_MANA(ch));
	}
}


ACMD(do_mirrorimage) {
	extern char_data *has_familiar(char_data *ch);
	void scale_mob_as_familiar(char_data *mob, char_data *master);
	
	char_data *mob, *other;
	obj_data *wield;
	int cost = 40, iter;
	mob_vnum vnum = MIRROR_IMAGE_MOB;
	bool found;
	
	if (!can_use_ability(ch, ABIL_MIRRORIMAGE, MANA, cost, COOLDOWN_MIRRORIMAGE)) {
		return;
	}
	
	if (has_familiar(ch)) {
		msg_to_char(ch, "You can't summon a mirror image while you already have a charmed follower.\r\n");
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_MIRRORIMAGE)) {
		return;
	}
	
	charge_ability_cost(ch, MANA, cost, COOLDOWN_MIRRORIMAGE, 5 * SECS_PER_REAL_MIN, WAIT_COMBAT_SPELL);
	mob = read_mobile(vnum, TRUE);
	
	// scale mob to the summoner -- so it won't change its attributes later
	scale_mob_as_familiar(mob, ch);
	
	char_to_room(mob, IN_ROOM(ch));
	
	// restrings
	GET_PC_NAME(mob) = str_dup(PERS(ch, ch, FALSE));
	GET_SHORT_DESC(mob) = str_dup(GET_PC_NAME(mob));
	sprintf(buf, "%s is standing here.\r\n", GET_SHORT_DESC(mob));
	*buf = UPPER(*buf);
	GET_LONG_DESC(mob) = str_dup(buf);

	// stats
	GET_REAL_SEX(mob) = GET_REAL_SEX(ch);
	
	mob->points.max_pools[HEALTH] = MIN(100, GET_HEALTH(ch));	// NOTE lower health
	mob->points.current_pools[HEALTH] = MIN(100, GET_HEALTH(ch));	// NOTE lower health
	mob->points.max_pools[MOVE] = GET_MAX_MOVE(ch);
	mob->points.current_pools[MOVE] = GET_MOVE(ch);
	mob->points.max_pools[MANA] = GET_MAX_MANA(ch);
	mob->points.current_pools[MANA] = GET_MANA(ch);
	mob->points.max_pools[BLOOD] = GET_MAX_BLOOD(ch);
	mob->points.current_pools[BLOOD] = GET_BLOOD(ch);
	
	for (iter = 0; iter < NUM_EXTRA_ATTRIBUTES; ++iter) {
		GET_EXTRA_ATT(mob, iter) = GET_EXTRA_ATT(ch, iter);
	}
	
	wield = GET_EQ(ch, WEAR_WIELD);
	MOB_ATTACK_TYPE(mob) = wield ? GET_WEAPON_TYPE(wield) : TYPE_HIT;
	MOB_DAMAGE(mob) = wield ? GET_WEAPON_DAMAGE_BONUS(wield) : 3;
	
	mob->real_attributes[STRENGTH] = GET_STRENGTH(ch);
	mob->real_attributes[DEXTERITY] = GET_DEXTERITY(ch);
	mob->real_attributes[CHARISMA] = GET_CHARISMA(ch);
	mob->real_attributes[GREATNESS] = GET_GREATNESS(ch);
	mob->real_attributes[INTELLIGENCE] = GET_INTELLIGENCE(ch);
	mob->real_attributes[WITS] = GET_WITS(ch);
	SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
	SET_BIT(MOB_FLAGS(mob), MOB_FAMILIAR);
	affect_total(mob);
	
	act("You create a mirror image to distract your foes!", FALSE, ch, NULL, NULL, TO_CHAR);
	act("$n suddenly splits in two!", FALSE, ch, NULL, NULL, TO_ROOM);
	
	// switch at least 1 thing that's hitting ch
	found = FALSE;
	for (other = ROOM_PEOPLE(IN_ROOM(ch)); other; other = other->next_in_room) {
		if (FIGHTING(other) == ch) {
			if (!found || !number(0, 1)) {
				found = TRUE;
				FIGHTING(other) = mob;
			}
		}
	}
	
	if (FIGHTING(ch)) {
		set_fighting(mob, FIGHTING(ch), FIGHT_MODE(ch));
	}
	
	add_follower(mob, ch, FALSE);
	gain_ability_exp(ch, ABIL_MIRRORIMAGE, 15);
	
	load_mtrigger(mob);
}


ACMD(do_ritual) {
	char arg2[MAX_INPUT_LENGTH];
	int iter, rit = NOTHING;
	bool found, result = FALSE;
	
	half_chop(argument, arg, arg2);
	
	if (IS_NPC(ch)) {
		msg_to_char(ch, "NPCs cannot perform rituals.\r\n");
		return;
	}
	
	// just ending
	if (GET_ACTION(ch) == ACT_RITUAL) {
		msg_to_char(ch, "You stop the ritual.\r\n");
		act("$n stops the ritual.", FALSE, ch, NULL, NULL, TO_ROOM);
		cancel_action(ch);
		return;
	}
	else if (GET_ACTION(ch) != ACT_NONE) {
		msg_to_char(ch, "You are already busy doing something else.\r\n");
		return;
	}
	
	// list rituals
	if (!*arg) {
		msg_to_char(ch, "You can perform the following rituals:");
		
		found = FALSE;
		for (iter = 0; *ritual_data[iter].name != '\n'; ++iter) {
			if (can_use_ritual(ch, iter)) {
				msg_to_char(ch, "%s%s", (found ? ", " : " "), ritual_data[iter].name);
				found = TRUE;
			}
		}
		
		if (found) {
			msg_to_char(ch, "\r\n");
		}
		else {
			msg_to_char(ch, " none\r\n");
		}
		return;
	}
	
	// find ritual
	found = FALSE;
	for (iter = 0; *ritual_data[iter].name != '\n' && rit == NOTHING; ++iter) {
		if (can_use_ritual(ch, iter) && is_abbrev(arg, ritual_data[iter].name)) {
			found = TRUE;
			rit = iter;
		}
	}
	
	if (!found) {
		msg_to_char(ch, "You don't know that ritual.\r\n");
		return;
	}
	
	// triggers?
	if (ritual_data[rit].ability != NO_ABIL && ABILITY_TRIGGERS(ch, NULL, NULL, ritual_data[rit].ability)) {
		return;
	}
	
	if (GET_MANA(ch) < ritual_data[rit].cost) {
		msg_to_char(ch, "You need %d mana to perform that ritual.\r\n", ritual_data[rit].cost);
		return;
	}
	
	if (ritual_data[rit].begin) {
		result = (ritual_data[rit].begin)(ch, arg2, rit);
	}
	else {
		msg_to_char(ch, "That ritual is not implemented.\r\n");
	}
	
	if (result) {
		GET_MANA(ch) -= ritual_data[rit].cost;
	}
}


ACMD(do_siphon) {
	char_data *vict = NULL;
	struct affected_type *af;
	int cost = 10;
	
	int levels[] = { 2, 5, 9 };
		
	if (!can_use_ability(ch, ABIL_SIPHON, MANA, cost, COOLDOWN_SIPHON)) {
		return;
	}
	if (get_cooldown_time(ch, COOLDOWN_SIPHON) > 0) {
		msg_to_char(ch, "Siphon is still on cooldown.\r\n");
		return;
	}
	
	// find target
	one_argument(argument, arg);
	if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
		return;
	}
	if (!*arg && !(vict = FIGHTING(ch))) {
		msg_to_char(ch, "Who would you like to cast that at?\r\n");
		return;
	}
	if (ch == vict) {
		msg_to_char(ch, "You wouldn't want to cast that on yourself.\r\n");
		return;
	}
	
	// check validity
	if (!can_fight(ch, vict)) {
		act("You can't attack $M!", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_SIPHON)) {
		return;
	}
	
	if (GET_MANA(vict) <= 0) {
		msg_to_char(ch, "Your target has no mana to siphon.\r\n");
		return;
	}
	
	charge_ability_cost(ch, MANA, cost, COOLDOWN_SIPHON, 20, WAIT_COMBAT_SPELL);
	
	if (SHOULD_APPEAR(ch)) {
		appear(ch);
	}
	
	// counterspell??
	if (trigger_counterspell(vict) || AFF_FLAGGED(vict, AFF_IMMUNE_HIGH_SORCERY)) {
		act("You try to siphon mana from $N, but are deflected by a counterspell!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n tries to siphon mana from you, but it's deflected by your counterspell!", FALSE, ch, NULL, vict, TO_VICT);
		act("$n tries to siphon mana from $N, but it fails!", FALSE, ch, NULL, vict, TO_NOTVICT);
	}
	else {
		// succeed
	
		act("$N starts to glow violet as you shout the mana siphon hex at $M! You feel your own mana grow as you drain $S.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n shouts something at you... The world takes on a violet glow and you feel your mana siphoned away.", FALSE, ch, NULL, vict, TO_VICT);
		act("$n shouts some kind of hex at $N, who starts to glow violet as mana flows away from $S skin!", FALSE, ch, NULL, vict, TO_NOTVICT);

		af = create_mod_aff(ATYPE_SIPHON, 4, APPLY_MANA_REGEN, CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_SIPHON), ch);
		affect_join(ch, af, 0);
		
		af = create_mod_aff(ATYPE_SIPHON_DRAIN, 4, APPLY_MANA_REGEN, -1 * CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_SIPHON), ch);
		affect_join(vict, af, 0);

		engage_combat(ch, vict, FALSE);
	}

	gain_ability_exp(ch, ABIL_SIPHON, 15);
}


ACMD(do_slow) {
	char_data *vict = NULL;
	struct affected_type *af;
	int cost = 20;
	
	int levels[] = { 0.5 MUD_HOURS, 0.5 MUD_HOURS, 1 MUD_HOURS };
		
	if (!can_use_ability(ch, ABIL_SLOW, MANA, cost, COOLDOWN_SLOW)) {
		return;
	}
	
	// find target
	one_argument(argument, arg);
	if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
		return;
	}
	if (!*arg && !(vict = FIGHTING(ch))) {
		msg_to_char(ch, "Who would you like to cast that at?\r\n");
		return;
	}
	if (ch == vict) {
		msg_to_char(ch, "You wouldn't want to cast that on yourself.\r\n");
		return;
	}
	
	// check validity
	if (!can_fight(ch, vict)) {
		act("You can't attack $M!", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_SLOW)) {
		return;
	}
	
	charge_ability_cost(ch, MANA, cost, COOLDOWN_SLOW, 75, WAIT_COMBAT_SPELL);
	
	if (SHOULD_APPEAR(ch)) {
		appear(ch);
	}
	
	// counterspell??
	if (trigger_counterspell(vict) || AFF_FLAGGED(vict, AFF_IMMUNE_HIGH_SORCERY)) {
		act("You try to use a slow hex on $N, but $E deflects it!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n tries to hex you, but it's deflected by your counterspell!", FALSE, ch, NULL, vict, TO_VICT);
		act("$n tries to hex $N, but $E deflects it.", FALSE, ch, NULL, vict, TO_NOTVICT);
	}
	else {
		// succeed
	
		act("$N grows lethargic and starts to glow gray as you shout the slow hex at $M!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n shouts something at you... The world takes on a gray tone and you more lethargic.", FALSE, ch, NULL, vict, TO_VICT);
		act("$n shouts some kind of hex at $N, who starts to move sluggishly and starts to glow gray!", FALSE, ch, NULL, vict, TO_NOTVICT);
	
		af = create_flag_aff(ATYPE_SLOW, CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_SLOW), AFF_SLOW, ch);
		affect_join(vict, af, 0);

		engage_combat(ch, vict, FALSE);
	}

	gain_ability_exp(ch, ABIL_SLOW, 15);
}


ACMD(do_vigor) {
	struct affected_type *af;
	char_data *vict = ch, *iter;
	int gain;
	bool fighting;
	
	// gain levels:		basic, specialty, class skill
	int costs[] = { 15, 25, 40 };
	int move_gain[] = { 30, 65, 125 };
	int bonus_per_intelligence = 5;
	
	one_argument(argument, arg);
	
	if (!can_use_ability(ch, ABIL_VIGOR, MANA, CHOOSE_BY_ABILITY_LEVEL(costs, ch, ABIL_VIGOR), NOTHING)) {
		return;
	}
	else if (*arg && !(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
	}
	else if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_VIGOR)) {
		return;
	}
	else if (GET_MOVE(vict) >= GET_MAX_MOVE(vict)) {
		if (ch == vict) {
			msg_to_char(ch, "You already have full movement points.\r\n");
		}
		else {
			act("$E already has full movement points.", FALSE, ch, NULL, vict, TO_CHAR);
		}
	}
	else if (affected_by_spell(vict, ATYPE_VIGOR)) {
		if (ch == vict) {
			msg_to_char(ch, "You can't cast vigor on yourself again until the effects of the last one have worn off.\r\n");
		}
		else {
			act("$E has had vigor cast on $M too recently to do it again.", FALSE, ch, NULL, vict, TO_CHAR);
		}
	}
	else {
		charge_ability_cost(ch, MANA, CHOOSE_BY_ABILITY_LEVEL(costs, ch, ABIL_VIGOR), NOTHING, 0, WAIT_SPELL);
		
		if (ch == vict) {
			msg_to_char(ch, "You focus your thoughts and say the word 'maktso', and you feel a burst of vigor!\r\n");
			act("$n closes $s eyes and says the word 'maktso', and $e seems suddenly refreshed.", TRUE, ch, NULL, NULL, TO_ROOM);
		}
		else {
			act("You focus your thoughts and say the word 'maktso', and $N suddenly seems refreshed.", FALSE, ch, NULL, vict, TO_CHAR);
			act("$n closes $s eyes and says the word 'matkso', and you feel a sudden burst of vigor!", FALSE, ch, NULL, vict, TO_VICT);
			act("$n closes $s eyes and says the word 'matkso', and $N suddenly seems refreshed.", FALSE, ch, NULL, vict, TO_NOTVICT);
		}
		
		// check if vict is in combat
		fighting = (FIGHTING(vict) != NULL);
		for (iter = ROOM_PEOPLE(IN_ROOM(ch)); iter && !fighting; iter = iter->next_in_room) {
			if (FIGHTING(iter) == vict) {
				fighting = TRUE;
			}
		}

		gain = CHOOSE_BY_ABILITY_LEVEL(move_gain, ch, ABIL_VIGOR) + (bonus_per_intelligence * GET_INTELLIGENCE(ch));
		
		// bonus if not in combat
		if (!fighting) {
			gain *= 2;
		}
		
		GET_MOVE(vict) = MIN(GET_MAX_MOVE(vict), GET_MOVE(vict) + gain);
		
		// the cast_by on this is vict himself, because it is a penalty and this will block cleanse
		af = create_mod_aff(ATYPE_VIGOR, 1 MUD_HOURS, APPLY_MOVE_REGEN, -5, vict);
		affect_join(vict, af, 0);
		
		gain_ability_exp(ch, ABIL_VIGOR, 33.4);

		if (FIGHTING(vict) && !FIGHTING(ch)) {
			engage_combat(ch, FIGHTING(vict), FALSE);
		}
	}
}


 ///////////////////////////////////////////////////////////////////////////////
//// RITUALS //////////////////////////////////////////////////////////////////


// used for rituals with no start requirements
RITUAL_SETUP_FUNC(start_simple_ritual) {
	start_ritual(ch, ritual);
	return TRUE;
}


RITUAL_FINISH_FUNC(perform_ritual_of_burdens) {
	struct affected_type *af;
	
	int burden_levels[] = { 6, 12, 18 };
	
	msg_to_char(ch, "You feel the weight of the world lift from your shoulders!\r\n");
	act("$n seems uplifted!", FALSE, ch, NULL, NULL, TO_ROOM);
	
	af = create_mod_aff(ATYPE_UNBURDENED, 24 MUD_HOURS, APPLY_INVENTORY, CHOOSE_BY_ABILITY_LEVEL(burden_levels, ch, ABIL_RITUAL_OF_BURDENS), ch);
	affect_join(ch, af, 0);	
	
	gain_ability_exp(ch, ABIL_RITUAL_OF_BURDENS, 25);
}


RITUAL_SETUP_FUNC(start_ritual_of_teleportation) {
	room_data *room, *next_room, *to_room = NULL;
	struct empire_city_data *city;
	int subtype = NOWHERE;
	bool wait;
	
	if (!can_teleport_to(ch, IN_ROOM(ch), TRUE) || RMT_FLAGGED(IN_ROOM(ch), RMT_NO_TELEPORT)) {
		msg_to_char(ch, "You can't teleport out of here.\r\n");
		return FALSE;
	}
	
	if (!*argument) {
		// random!
		subtype = NOWHERE;
	}
	else if (!str_cmp(argument, "home")) {
		HASH_ITER(world_hh, world_table, room, next_room) {
			if (ROOM_PRIVATE_OWNER(room) == GET_IDNUM(ch)) {
				to_room = room;
				break;
			}
		}

		if (!to_room) {
			msg_to_char(ch, "You have no home to which you could teleport.\r\n");
			return FALSE;
		}
		else if (get_cooldown_time(ch, COOLDOWN_TELEPORT_HOME) > 0) {
			msg_to_char(ch, "Your home teleportation is still on cooldown.\r\n");
			return FALSE;
		}
		else if (IS_RIDING(ch)) {
			msg_to_char(ch, "You can't teleport home while riding.\r\n");
			return FALSE;
		}
		else {
			subtype = GET_ROOM_VNUM(to_room);
		}
	}
	else if (HAS_ABILITY(ch, ABIL_CITY_TELEPORTATION) && (city = find_city_by_name(GET_LOYALTY(ch), argument))) {
		subtype = GET_ROOM_VNUM(city->location);
		
		if (get_cooldown_time(ch, COOLDOWN_TELEPORT_CITY) > 0) {
			msg_to_char(ch, "Your city teleportation is still on cooldown.\r\n");
			return FALSE;
		}
		if (!is_in_city_for_empire(city->location, GET_LOYALTY(ch), TRUE, &wait)) {
			msg_to_char(ch, "That city was founded too recently to teleport to it.\r\n");
			return FALSE;
		}
	}
	else {
		msg_to_char(ch, "That's not a valid place to teleport.\r\n");
		return FALSE;
	}
	
	start_ritual(ch, ritual);
	GET_ACTION_VNUM(ch, 1) = subtype;	// vnum 0 is ritual id
	return TRUE;
}


RITUAL_FINISH_FUNC(perform_ritual_of_teleportation) {
	void cancel_adventure_summon(char_data *ch);
	
	room_data *to_room, *rand_room;
	int tries, rand_x, rand_y;
	bool random;
	
	to_room = real_room(GET_ACTION_VNUM(ch, 1));
	random = to_room ? FALSE : TRUE;
	
	// if there's no room, find a where
	tries = 0;
	while (!to_room && tries < 100) {
		// randomport!
		rand_x = number(0, 1) ? number(2, 7) : number(-7, -2);
		rand_y = number(0, 1) ? number(2, 7) : number(-7, -2);
		rand_room = real_shift(HOME_ROOM(IN_ROOM(ch)), rand_x, rand_y);
		
		// found outdoors!
		if (rand_room && !ROOM_IS_CLOSED(rand_room) && can_teleport_to(ch, rand_room, TRUE)) {
			to_room = rand_room;
		}
	}
	
	if (!to_room || !can_teleport_to(ch, to_room, TRUE)) {
		msg_to_char(ch, "Teleportation failed: you couldn't find a safe place to teleport.\r\n");
	}
	else {
		act("$n vanishes in a brilliant flash of light!", FALSE, ch, NULL, NULL, TO_ROOM);
		char_to_room(ch, to_room);
		look_at_room_by_loc(ch, IN_ROOM(ch), NOBITS);
		act("$n appears with a brilliant flash of light!", FALSE, ch, NULL, NULL, TO_ROOM);
	
		// reset this in case they teleport onto a wall.
		GET_LAST_DIR(ch) = NO_DIR;
		
		// any existing adventure summon location is no longer valid after a voluntary teleport
		if (!random) {	// except random teleport
			cancel_adventure_summon(ch);
		}

		// trigger block	
		enter_wtrigger(IN_ROOM(ch), ch, NO_DIR);
		entry_memory_mtrigger(ch);
		greet_mtrigger(ch, NO_DIR);
		greet_memory_mtrigger(ch);
	
		gain_ability_exp(ch, ABIL_RITUAL_OF_TELEPORTATION, 50);
	
		if (IS_CITY_CENTER(IN_ROOM(ch))) {
			gain_ability_exp(ch, ABIL_CITY_TELEPORTATION, 50);
			add_cooldown(ch, COOLDOWN_TELEPORT_CITY, 15 * SECS_PER_REAL_MIN);
		}
		else if (ROOM_PRIVATE_OWNER(IN_ROOM(ch)) == GET_IDNUM(ch)) {
			add_cooldown(ch, COOLDOWN_TELEPORT_HOME, 15 * SECS_PER_REAL_MIN);
		}
	}
}


RITUAL_FINISH_FUNC(perform_phoenix_rite) {
	struct affected_type *af;
	
	msg_to_char(ch, "The flames on the remaining candles shoot toward you and form the crest of the Phoenix!\r\n");
	act("The flames on the remaining candles shoot toward $n and form a huge fiery bird around $m!", FALSE, ch, NULL, NULL, TO_ROOM);

	af = create_mod_aff(ATYPE_PHOENIX_RITE, UNLIMITED, APPLY_NONE, 0, ch);
	affect_join(ch, af, 0);

	gain_ability_exp(ch, ABIL_PHOENIX_RITE, 10);
}


RITUAL_SETUP_FUNC(start_ritual_of_defense) {
	Resource defense_res[3] = { { o_IMPERIUM_SPIKE, 1 }, { o_BLOODSTONE, 1 }, END_RESOURCE_LIST };
	bool found = FALSE;
	
	// valid sects
	if (ROOM_BLD_FLAGGED(IN_ROOM(ch), BLD_BARRIER)) {
		found = TRUE;
	}

	if (!found) {
		msg_to_char(ch, "You can't perform the Ritual of Defense here.\r\n");
		return FALSE;
	}
	
	if (!IS_COMPLETE(IN_ROOM(ch))) {
		msg_to_char(ch, "You need to finish it before you can cast Ritual of Defense.\r\n");
		return FALSE;
	}
	
	if (!has_resources(ch, defense_res, TRUE, TRUE)) {
		// message sent by has_resources
		return FALSE;
	}
	
	// OK: take resources
	extract_resources(ch, defense_res, TRUE);
	start_ritual(ch, ritual);
	return TRUE;
}


RITUAL_FINISH_FUNC(perform_ritual_of_defense) {
	msg_to_char(ch, "You finish the ritual and the walls take on a strange magenta glow!\r\n");
	act("$n finishes the ritual and the walls take on a strange magenta glow!", FALSE, ch, NULL, NULL, TO_ROOM);
	if (!ROOM_AFF_FLAGGED(IN_ROOM(ch), ROOM_AFF_NO_FLY)) {
		gain_ability_exp(ch, ABIL_RITUAL_OF_DEFENSE, 25);
	}
	SET_BIT(ROOM_AFF_FLAGS(IN_ROOM(ch)), ROOM_AFF_NO_FLY);
	SET_BIT(ROOM_BASE_FLAGS(IN_ROOM(ch)), ROOM_AFF_NO_FLY);
}


RITUAL_FINISH_FUNC(perform_sense_life_ritual) {
	char_data *targ;
	bool found;
	
	msg_to_char(ch, "You finish the ritual and your eyes are opened...\r\n");
	act("$n finishes the ritual and $s eyes flash a bright white.", FALSE, ch, NULL, NULL, TO_ROOM);
	
	found = FALSE;
	for (targ = ROOM_PEOPLE(IN_ROOM(ch)); targ; targ = targ->next_in_room) {
		if (ch != targ && AFF_FLAGGED(targ, AFF_HIDE) && !CAN_SEE(ch, targ)) {
			SET_BIT(AFF_FLAGS(ch), AFF_SENSE_HIDE);

			if (CAN_SEE(ch, targ)) {
				act("You sense $N hiding here!", FALSE, ch, 0, targ, TO_CHAR);
				msg_to_char(targ, "You are discovered!\r\n");
				REMOVE_BIT(AFF_FLAGS(targ), AFF_HIDE);
				found = TRUE;
			}

			REMOVE_BIT(AFF_FLAGS(ch), AFF_SENSE_HIDE);
		}
	}
	
	if (!found) {
		msg_to_char(ch, "You sense no unseen presence.\r\n");
	}
	
	gain_ability_exp(ch, ABIL_SENSE_LIFE_RITUAL, 15);
}


RITUAL_SETUP_FUNC(start_ritual_of_detection) {
	bool wait;
	
	if (!GET_LOYALTY(ch)) {
		msg_to_char(ch, "You must be a member of an empire to do this.\r\n");
		return FALSE;
	}
	if (!is_in_city_for_empire(IN_ROOM(ch), GET_LOYALTY(ch), TRUE, &wait)) {
		msg_to_char(ch, "You can only use the Ritual of Detection in one of your own cities%s.\r\n", wait ? " (this city was founded too recently)" : "");
		return FALSE;
	}

	start_ritual(ch, ritual);
	return TRUE;
}


RITUAL_FINISH_FUNC(perform_ritual_of_detection) {
	extern char *get_room_name(room_data *room, bool color);
	
	struct empire_city_data *city;
	descriptor_data *d;
	char_data *targ;
	bool found, wait;
	
	if (!GET_LOYALTY(ch)) {
		msg_to_char(ch, "The ritual fails as you aren't in any empire.\r\n");
	}
	else if (!is_in_city_for_empire(IN_ROOM(ch), GET_LOYALTY(ch), TRUE, &wait) || !(city = find_city(GET_LOYALTY(ch), IN_ROOM(ch)))) {
		msg_to_char(ch, "The ritual fails as you aren't in one of your cities%s.\r\n", wait ? " (this city was founded too recently)" : "");
	}
	else {
		msg_to_char(ch, "You complete the Ritual of Detection...\r\n");
	
		found = FALSE;
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) == CON_PLAYING && (targ = d->character) && targ != ch && !IS_NPC(targ) && !IS_IMMORTAL(targ)) {
				if (find_city(GET_LOYALTY(ch), IN_ROOM(targ)) == city) {
					found = TRUE;
					msg_to_char(ch, "You sense %s at (%d, %d) %s\r\n", PERS(targ, targ, FALSE), X_COORD(IN_ROOM(targ)), Y_COORD(IN_ROOM(targ)), get_room_name(IN_ROOM(targ), FALSE));
				}
			}
		}
		
		if (!found) {
			msg_to_char(ch, "You sense no other players in the city.\r\n");
		}
		gain_ability_exp(ch, ABIL_RITUAL_OF_DETECTION, 15);
	}				
}


RITUAL_SETUP_FUNC(start_siege_ritual) {
	struct empire_political_data *emp_pol = NULL;
	room_data *to_room;
	empire_data *enemy;
	int dir;
	
	if (!*argument) {
		msg_to_char(ch, "Besiege which direction?\r\n");
		return FALSE;
	}
	if ((dir = parse_direction(ch, argument)) == NO_DIR) {
		msg_to_char(ch, "Which direction is that?\r\n");
		return FALSE;
	}
	if (ROOM_IS_CLOSED(IN_ROOM(ch))) {
		msg_to_char(ch, "You must be outdoors to do this.\r\n");
		return FALSE;
	}
	
	to_room = real_shift(IN_ROOM(ch), shift_dir[dir][0], shift_dir[dir][1]);
	if (dir >= NUM_2D_DIRS || !to_room || to_room == IN_ROOM(ch)) {
		msg_to_char(ch, "You can't besiege that direction.\r\n");
		return FALSE;
	}
	if (!ROOM_IS_CLOSED(to_room) && !IS_MAP_BUILDING(to_room)) {
		msg_to_char(ch, "The Siege Ritual can only be used to destroy completed buildings.\r\n");
		return FALSE;
	}
	if (ROOM_BLD_FLAGGED(IN_ROOM(ch), BLD_BARRIER)) {
		msg_to_char(ch, "You can't perform the ritual this close to the barrier.\r\n");
		return FALSE;
	}
	if (IS_CITY_CENTER(to_room)) {
		msg_to_char(ch, "You can't besiege at a city center.\r\n");
		return FALSE;
	}
	if (ROOM_SECT_FLAGGED(to_room, SECTF_START_LOCATION)) {
		msg_to_char(ch, "You can't besiege a starting location.\r\n");
		return FALSE;
	}
	
	if ((enemy = ROOM_OWNER(to_room))) {
		emp_pol = find_relation(GET_LOYALTY(ch), enemy);
	}
	if (enemy && (!emp_pol || !IS_SET(emp_pol->type, DIPL_WAR))) {
		msg_to_char(ch, "You can't besiege that tile!\r\n");
		return FALSE;
	}

	// ready:

	// trigger hostile immediately so they are attackable
	if (enemy && GET_LOYALTY(ch) != enemy) {
		trigger_distrust_from_hostile(ch, enemy);
	}
	
	start_ritual(ch, ritual);
	GET_ACTION_VNUM(ch, 1) = GET_ROOM_VNUM(to_room);	// action 0 is ritual #
	return TRUE;
}


RITUAL_FINISH_FUNC(perform_siege_ritual) {
	void besiege_room(room_data *to_room, int damage);
	
	sector_data *secttype;
	room_data *to_room = real_room(GET_ACTION_VNUM(ch, 1));
	
	if (!to_room) {
		msg_to_char(ch, "You don't seem to have a valid target, and the ritual fails.\r\n");
	}
	else {
		if (SHOULD_APPEAR(ch)) {
			appear(ch);
		}
		
		secttype = SECT(to_room);
		msg_to_char(ch, "You fire one last powerful fireball...\r\n");
		act("$n fires one last powerful fireball...", FALSE, ch, NULL, NULL, TO_ROOM);

		if (ROOM_OWNER(to_room) && GET_LOYALTY(ch) != ROOM_OWNER(to_room)) {
			trigger_distrust_from_hostile(ch, ROOM_OWNER(to_room));
		}
		besiege_room(to_room, 1 + GET_INTELLIGENCE(ch));

		if (SECT(to_room) != secttype) {
			msg_to_char(ch, "It is destroyed!\r\n");
			act("$n's target is destroyed!", FALSE, ch, NULL, NULL, TO_ROOM);
		}

		gain_ability_exp(ch, ABIL_SIEGE_RITUAL, 33.4);
	}
}


RITUAL_SETUP_FUNC(start_devastation_ritual) {
	if (IS_ADVENTURE_ROOM(IN_ROOM(ch)) || !IS_OUTDOORS(ch)) {
		msg_to_char(ch, "You need to be outdoors to do perform the Devastation Ritual.\r\n");
		return FALSE;
	}
	if (!can_use_room(ch, IN_ROOM(ch), MEMBERS_ONLY)) {
		msg_to_char(ch, "You don't have permission to devastate here.\r\n");
		return FALSE;
	}

	start_ritual(ch, ritual);
	return TRUE;
}


RITUAL_FINISH_FUNC(perform_devastation_ritual) {
	extern int change_chop_territory(room_data *room);
	extern const sector_vnum climate_default_sector[NUM_CLIMATES];
	
	room_data *rand_room, *to_room = NULL;
	obj_data *newobj;
	crop_data *cp;
	int dist, iter;
	int x, y, num;
	
	#define CAN_DEVASTATE(room)  ((ROOM_SECT_FLAGGED((room), SECTF_HAS_CROP_DATA) || CAN_CHOP_ROOM(room)) && !ROOM_AFF_FLAGGED((room), ROOM_AFF_HAS_INSTANCE))
	#define DEVASTATE_RANGE  3	// tiles

	// check this room
	if (CAN_DEVASTATE(IN_ROOM(ch)) && can_use_room(ch, IN_ROOM(ch), MEMBERS_ONLY)) {
		to_room = IN_ROOM(ch);
	}
	
	// check surrounding rooms in star patter
	for (dist = 1; !to_room && dist <= DEVASTATE_RANGE; ++dist) {
		for (iter = 0; !to_room && iter < NUM_2D_DIRS; ++iter) {
			rand_room = real_shift(IN_ROOM(ch), shift_dir[iter][0] * dist, shift_dir[iter][1] * dist);
			if (rand_room && CAN_DEVASTATE(rand_room) && compute_distance(IN_ROOM(ch), rand_room) <= DEVASTATE_RANGE && can_use_room(ch, rand_room, MEMBERS_ONLY)) {
				to_room = rand_room;
			}
		}
	}
	
	// check max radius
	for (x = -DEVASTATE_RANGE; !to_room && x <= DEVASTATE_RANGE; ++x) {
		for (y = -DEVASTATE_RANGE; !to_room && y <= DEVASTATE_RANGE; ++y) {
			rand_room = real_shift(IN_ROOM(ch), x, y);
			if (rand_room && CAN_DEVASTATE(rand_room) && compute_distance(IN_ROOM(ch), rand_room) <= DEVASTATE_RANGE && can_use_room(ch, rand_room, MEMBERS_ONLY)) {
				to_room = rand_room;
			}
		}
	}
	
	// SUCCESS: distribute resources
	if (to_room) {
		if (has_evolution_type(SECT(to_room), EVO_CHOPPED_DOWN)) {
			num = change_chop_territory(to_room);
			
			msg_to_char(ch, "You devastate the forest and collect %s tree%s!\r\n", num > 1 ? "some" : "a", num > 1 ? "s" : "");
			act("$n's powerful ritual devastates the forest!", FALSE, ch, NULL, NULL, TO_ROOM);
			
			while (num-- > 0) {
				obj_to_char_or_room((newobj = read_object(o_TREE, TRUE)), ch);
				load_otrigger(newobj);
			}
		}
		else if (ROOM_SECT_FLAGGED(to_room, SECTF_CROP) && (cp = crop_proto(ROOM_CROP_TYPE(to_room))) && has_interaction(GET_CROP_INTERACTIONS(cp), INTERACT_HARVEST)) {
			run_room_interactions(ch, to_room, INTERACT_HARVEST, devastate_crop);
			
			// check for original sect, which may have been stored
			if (ROOM_ORIGINAL_SECT(to_room) != SECT(to_room)) {
				change_terrain(to_room, GET_SECT_VNUM(ROOM_ORIGINAL_SECT(to_room)));
			}
			else {
				// fallback sect
				change_terrain(to_room, climate_default_sector[GET_CROP_CLIMATE(cp)]);
			}
		}
		else if (ROOM_SECT_FLAGGED(to_room, SECTF_HAS_CROP_DATA) && (cp = crop_proto(ROOM_CROP_TYPE(to_room)))) {
			msg_to_char(ch, "You devastate the seeded field!\r\n");
			act("$n's powerful ritual devastates the seeded field!", FALSE, ch, NULL, NULL, TO_ROOM);
			
			// check for original sect, which may have been stored
			if (ROOM_ORIGINAL_SECT(to_room) != SECT(to_room)) {
				change_terrain(to_room, GET_SECT_VNUM(ROOM_ORIGINAL_SECT(to_room)));
			}
			else {
				// fallback sect
				change_terrain(to_room, climate_default_sector[GET_CROP_CLIMATE(cp)]);
			}
		}
		else {
			msg_to_char(ch, "The Devastation Ritual has failed.\r\n");
			return;
		}
	
		gain_ability_exp(ch, ABIL_DEVASTATION_RITUAL, 10);
		
		// auto-repeat
		if (GET_MANA(ch) >= ritual_data[ritual].cost) {
			GET_MANA(ch) -= ritual_data[ritual].cost;
			GET_ACTION(ch) = ACT_RITUAL;
			GET_ACTION_TIMER(ch) = 0;
			// other variables are still set up
		}
		else {
			msg_to_char(ch, "You do not have enough mana to continue the ritual.\r\n");
		}
	}
	else {
		msg_to_char(ch, "The Devastation Ritual is complete.\r\n");
	}
}


 ///////////////////////////////////////////////////////////////////////////////
//// STUDY STUFF //////////////////////////////////////////////////////////////

// TODO ultimately, this should be moved to triggers

const char *study_strings[] = {
	// placeholder
	"",
	
	"   You sit down at a bench and open a worn old book, one that has long since\r\n"
	"lost its gold leaf. The leather cover is faded and worn, and you can barely\r\n"
	"make out the words through a fine layer of dust. As you blow the dust away, a\r\n"
	"white flame flashes across the cover and sears the book's name anew. This is\r\n"
	"The Handril Instructionary: Apprentice to Sorcery, 2nd Edition, Volumes I-II.\r\n",

	"   \"Since the dawn of time, Man has wielded the primal mana which flows through\r\n"
	"the veins of our world. In the first empires, during the Age of Destiny, the\r\n"
	"primitive tribes of men, dwarfs, orcs, and elves drew magic from rare stones\r\n"
	"named for the gods Vinak, Hadash, and Ma'kor. As the race of men conquered the\r\n"
	"land, they came to wield mana directly, and so became the first druids.\"\r\n",

	"   \"For many years, the druids hid themselves, and squirreled their magic away\r\n"
	"from the world. As great cities rose and fell and rose again, and empires sank\r\n"
	"into the mires of history, the art of magic was kept to whispers. But even the\r\n"
	"druids could not keep this secret forever. High Sorcery begins with the druid\r\n"
	"who left his order and founded a school to investigate the true depths of the\r\n"
	"power of mana. For this, he was branded Warlock.\"\r\n",

	"   \"High Sorcery is by nature academic magic. It is pondered and forged\r\n"
	"where the so-called natural magic is raw and untamed. A sorcerer must learn to\r\n"
	"focus his -- or her -- energies. This is done through study, through ritual,\r\n"
	"and through rigorous practice. Each sorcerer is held to the highest standard,\r\n"
	"whether at the academy or in the field. Each sorcerer swears to uphold the core\r\n"
	"ideals of High Sorcery, even while pursuing his or her own ambitions.\"\r\n",

	"   \"A sorcerer swears first to be loyal to his prince. Above all things, we\r\n"
	"serve the empire. A sorcerer swears second to seek knowledge. Above all things,\r\n"
	"we know that wisdom is the true source of power. A sorcerer swears third to\r\n"
	"leave his sigil emblazoned on the pages of history. Above all things, our\r\n"
	"greatness must wave as an unyielding banner over the whole of the world.\"\r\n",

	"   You find the book is missing some pages after the oath, and begin skimming\r\n"
	"through the sections that remain. There's a section about the origins of mana\r\n"
	"and the proper techniques for channeling it through a staff, but that seems to\r\n"
	"be something you could figure out on your own. Pages have been torn from that\r\n"
	"section as well, and you flip through descriptions of some of the great spells\r\n"
	"and rituals contained within the book.\r\n",

	"   \"Once, worldly travel was restricted to horses and canoes. Even modern ships\r\n"
	"of sail can take days or weeks to reach their destination. For this reason, the\r\n"
	"academy of High Sorcery has begun to craft a network of portals across the\r\n"
	"many continents. Through the proper invocations and rituals, a ring of stones\r\n"
	"can anchor reality itself and allow even a layman to bridge two such rings.\"\r\n",

	"   \"The earliest sorcerers learned to bind their magic to physical objects in\r\n"
	"order to enhance the physical prowess of their bearers. This craft, called\r\n"
	"Enchanting, brought the greatest emperors to the doors of the towers of sor-\r\n"
	"cery in search of powerful artifacts. Even today, powerful artificers and\r\n"
	"enchanters can earn a living in the service of kings.\"\r\n",

	"   \"When the druids began to emerge from their ancient hiding places, it was\r\n"
	"only to fight their successor, the academy of High Sorcery. And so ensued the\r\n"
	"great magic wars, pitting tribe against empire, staff against staff. In this\r\n"
	"way, the sorcerers came to train battlemages in the martial arts. Soon, these\r\n"
	"sorcerers became the most feared warriors in the land, capable of laying waste\r\n"
	"to whole armies with a single spell.\"\r\n",

	"   \"A battlemage protects himself first by wielding a staff. This traditional\r\n"
	"method of focus allows for powerful magic without draining the sorcerer's own\r\n"
	"vein of mana. Many sorcerers who have suffered the loss of their staves have\r\n"
	"also suffered the loss of their lives.\"\r\n",

	"   \"A battlemage must learn to anticipate their enemy's actions using a ritual\r\n"
	"known to some as Foresight. The truth, however, is that it does not allow the\r\n"
	"sorcerer to see the future. Instead, this ritual seeds the air with mana to\r\n"
	"show the path down which physical objects travel. The entire swing of a war-\r\n"
	"rior's sword is visible even as his muscles begin to twitch, and the cleverest\r\n"
	"sorcerer is never caught off-guard.\"\r\n",

	"   You close the worn old book and set it down. Perhaps that's enough reading\r\n"
	"for now. It's clear the path of High Sorcery is a long one, and this is only\r\n"
	"the start of the journey. As you reflect upon the book, its illuminated pages\r\n"
	"still fresh in your mind, you begin to wonder what else awaits you on this\r\n"
	"path. Only time will tell.\r\n",

	"\n"
};


// **** for do_study ****
void perform_study(char_data *ch) {
	void set_skill(char_data *ch, int skill, int level);
	int pos;
	
	GET_ACTION_TIMER(ch) += 1;

	if ((GET_ACTION_TIMER(ch) % 4) != 0) {
		// no message to show
		return;
	}
	
	pos = GET_ACTION_TIMER(ch) / 4;
	
	if (*study_strings[pos] == '\n') {
		// DONE!
		if (CAN_GAIN_NEW_SKILLS(ch) && GET_SKILL(ch, SKILL_HIGH_SORCERY) == 0 && !NOSKILL_BLOCKED(ch, SKILL_HIGH_SORCERY)) {
			msg_to_char(ch, "&mYour mind begins to open to the ways of High Sorcery, and you are now an apprentice to this school.&0\r\n");
			set_skill(ch, SKILL_HIGH_SORCERY, 1);
			SAVE_CHAR(ch);
		}
		cancel_action(ch);
	}
	else {
		send_to_char(study_strings[pos], ch);
	}
}


ACMD(do_study) {
	// just ending
	if (GET_ACTION(ch) == ACT_STUDYING) {
		msg_to_char(ch, "You put down the book and stop studying.\r\n");
		act("$n puts down $s book and stops studying.", FALSE, ch, NULL, NULL, TO_ROOM);
		cancel_action(ch);
		return;
	}
	
	if (GET_ACTION(ch) != ACT_NONE) {
		msg_to_char(ch, "You're a little busy right now.\r\n");
		return;
	}
	
	if (BUILDING_VNUM(IN_ROOM(ch)) != RTYPE_SORCERER_TOWER) {
		msg_to_char(ch, "You need to be in the library of a Tower of Sorcery.\r\n");
		return;
	}
	
	if (!can_use_room(ch, IN_ROOM(ch), GUESTS_ALLOWED)) {
		msg_to_char(ch, "You don't have permission to study here.\r\n");
		return;
	}
	
	// do chant
	msg_to_char(ch, "You pick up a book to study.\r\n");
	act("$n picks up a book to study.", FALSE, ch, NULL, NULL, TO_ROOM);
	
	start_action(ch, ACT_STUDYING, 0);
}
