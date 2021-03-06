/* ************************************************************************
*   File: act.battle.c                                    EmpireMUD 2.0b3 *
*  Usage: commands and functions related to the Battle skill              *
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
#include "dg_scripts.h"

/**
* Contents:
*   Battle Helpers
*   Battle Commands
*/

// external vars

// external functions
extern bool check_hit_vs_dodge(char_data *attacker, char_data *victim, bool off_hand);	// fight.c
extern bool is_fight_ally(char_data *ch, char_data *frenemy);	// fight.c


 //////////////////////////////////////////////////////////////////////////////
//// BATTLE HELPERS //////////////////////////////////////////////////////////

/**
* @param char_data *ch The person who is rescuing...
* @param char_data *vict The person in need of rescue.
* @param char_data *from The attacker, who will now hit ch.
*/
void perform_rescue(char_data *ch, char_data *vict, char_data *from) {
	send_to_char("Banzai! To the rescue...\r\n", ch);
	act("You are rescued by $N!", FALSE, vict, 0, ch, TO_CHAR);
	act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

	if (FIGHTING(vict) == from)
		stop_fighting(vict);
	if (FIGHTING(from))
		stop_fighting(from);
	if (FIGHTING(ch))
		stop_fighting(ch);

	set_fighting(ch, from, FMODE_MELEE);
	set_fighting(from, ch, FMODE_MELEE);
}


 //////////////////////////////////////////////////////////////////////////////
//// BATTLE COMMANDS /////////////////////////////////////////////////////////


ACMD(do_bash) {
	char_data *vict;
	struct affected_type *af;
	bool success = FALSE;
	int dam, cost = 15;

	one_argument(argument, arg);

	if (!can_use_ability(ch, ABIL_BASH, MOVE, cost, COOLDOWN_BASH)) {
		return;
	}
	if (!GET_EQ(ch, WEAR_WIELD)) {
		send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_DISARM)) {
		msg_to_char(ch, "You can't bash -- your weapon has been disarmed!\r\n");
		return;
	}
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("Bash who?\r\n", ch);
			return;
		}
	}
	if (vict == ch) {
		send_to_char("Aren't we funny today...\r\n", ch);
		return;
	}

	if (FIGHT_MODE(vict) == FMODE_MISSILE || FIGHT_MODE(ch) == FMODE_MISSILE) {
		msg_to_char(ch, "You aren't close enough.\r\n");
		return;
	}

	if (!can_fight(ch, vict)) {
		act("You can't attack $N!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_BASH)) {
		return;
	}
	
	charge_ability_cost(ch, MOVE, cost, COOLDOWN_BASH, 9, WAIT_COMBAT_ABILITY);

	// determine hit
	success = IS_SPECIALTY_ABILITY(ch, ABIL_BASH) || check_hit_vs_dodge(ch, vict, FALSE);

	if (!success) {
		damage(ch, vict, 0, ATTACK_BASH, DAM_PHYSICAL);
	}
	else {
		/*
		 * If we bash a player and they wimp out, they will move to the previous
		 * room before we set them sitting.  If we try to set the victim sitting
		 * first to make sure they don't flee, then we can't bash them!  So now
		 * we only set them sitting if they didn't flee. -gg 9/21/98
		 */

		dam = GET_STRENGTH(ch) + GET_WEAPON_DAMAGE_BONUS(GET_EQ(ch, WEAR_WIELD));
		dam += GET_BONUS_PHYSICAL(ch);
		
		if (IS_CLASS_ABILITY(ch, ABIL_BASH)) {
			dam *= 2;
		}

		if (damage(ch, vict, dam, ATTACK_BASH, DAM_PHYSICAL) > 0) {	/* -1 = dead, 0 = miss */
			if (!AFF_FLAGGED(vict, AFF_IMMUNE_BATTLE | AFF_IMMUNE_STUN)) {
				af = create_flag_aff(ATYPE_BASH, 1, AFF_STUNNED, ch);
				affect_join(vict, af, 0);
		
				// release other saps here
				limit_crowd_control(vict, ATYPE_BASH);
			}			
		}		
	}

	gain_ability_exp(ch, ABIL_BASH, 15);
}


ACMD(do_disarm) {
	struct affected_type *af;
	char_data *victim;
	int cost = 15;
	int disarm_levels[] = { 1, 3, 5 };

	one_argument(argument, arg);

	if (!can_use_ability(ch, ABIL_DISARM, MOVE, cost, COOLDOWN_DISARM)) {
		// nope
	}
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)) && !(victim = FIGHTING(ch)))
		msg_to_char(ch, "Disarm whom?\r\n");
	else if (ch == victim)
		msg_to_char(ch, "You wouldn't want to do that to yourself...\r\n");
	else if (IS_GOD(victim) || IS_IMMORTAL(victim))
		msg_to_char(ch, "You cannot use this ability on so godly a target!\r\n");
	else if (!can_fight(ch, victim))
		act("You can't attack $M!", FALSE, ch, 0, victim, TO_CHAR);
	else if (!IS_NPC(victim) && !GET_EQ(victim, WEAR_WIELD)) {
		msg_to_char(ch, "You can't disarm someone who isn't wielding a weapon.\r\n");
	}
	else if (IS_NPC(victim) && !GET_EQ(victim, WEAR_WIELD) && (MOB_ATTACK_TYPE(victim) == TYPE_BITE || MOB_ATTACK_TYPE(victim) == TYPE_KICK || MOB_ATTACK_TYPE(victim) == TYPE_CLAW || MOB_ATTACK_TYPE(victim) == TYPE_HIT)) {
		act("You can't disarm $M -- $E isn't using a weapon.", FALSE, ch, 0, victim, TO_CHAR);
	}
	else if (AFF_FLAGGED(victim, AFF_DISARM)) {
		act("$E is already disarmed.", FALSE, ch, 0, victim, TO_CHAR);
	}
	else if (ABILITY_TRIGGERS(ch, victim, NULL, ABIL_DISARM)) {
		return;
	}
	else {
		if (SHOULD_APPEAR(ch)) {
			appear(ch);
		}
		
		charge_ability_cost(ch, MOVE, cost, COOLDOWN_DISARM, 30, WAIT_COMBAT_ABILITY);
		
		if (!skill_check(ch, ABIL_DISARM, DIFF_HARD) || AFF_FLAGGED(victim, AFF_IMMUNE_BATTLE)) {
			act("You attempt to disarm $N, but fail.", FALSE, ch, 0, victim, TO_CHAR);
			act("$n attempts to disarm you, but fails.", FALSE, ch, 0, victim, TO_VICT);
			act("$n attempts to disarm $N, but fails.", FALSE, ch, 0, victim, TO_NOTVICT);
		}
		else {
			act("You skillfully disarm $N!", FALSE, ch, 0, victim, TO_CHAR);
			act("$n disarms you! (Your weapon will not work until it wears off.)", FALSE, ch, 0, victim, TO_VICT);
			act("$n skillfully disarms $N!", TRUE, ch, 0, victim, TO_NOTVICT);

			af = create_flag_aff(ATYPE_DISARM, CHOOSE_BY_ABILITY_LEVEL(disarm_levels, ch, ABIL_DISARM), AFF_DISARM, ch);
			affect_join(victim, af, 0);
		}
		
		engage_combat(ch, victim, TRUE);
		gain_ability_exp(ch, ABIL_DISARM, 15);
	}
}


ACMD(do_firstaid) {
	extern int total_bonus_healing(char_data *ch);

	struct over_time_effect_type *dot, *next_dot;
	bool has_dot = FALSE;
	char_data *vict;
	int cost = 20;
	int levels[] = { 10, 20, 50 };
	
	if (!can_use_ability(ch, ABIL_FIRSTAID, MOVE, cost, NOTHING)) {
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		vict = ch;
	}
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
		return;
	}
	
	if (IS_DEAD(vict)) {
		msg_to_char(ch, "Unfortunately you can't use firstaid on someone who is already dead.\r\n");
		return;
	}
	
	if (FIGHTING(vict)) {
		msg_to_char(ch, "You can't use firstaid on someone in combat.\r\n");
		return;
	}
	
	// check for DoTs
	for (dot = vict->over_time_effects; dot; dot = next_dot) {
		next_dot = dot->next;

		if (dot->damage_type == DAM_PHYSICAL || dot->damage_type == DAM_POISON || dot->damage_type == DAM_FIRE) {
			has_dot = TRUE;
			break;
		}
	}

	if (GET_HEALTH(vict) >= GET_MAX_HEALTH(vict) && !has_dot) {
		msg_to_char(ch, "You can't apply first aid to someone who isn't injured.\r\n");
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_FIRSTAID)) {
		return;
	}
	
	charge_ability_cost(ch, MOVE, cost, NOTHING, 0, WAIT_ABILITY);
	
	if (ch == vict) {
		msg_to_char(ch, "You apply first aid to your wounds.\r\n");
		act("$n applies first aid to $s wounds.", TRUE, ch, NULL, NULL, TO_ROOM);
	}
	else {
		act("You apply first aid to $N's wounds.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n applies first aid to your wounds.", FALSE, ch, NULL, vict, TO_VICT);
		act("$n applies first aid to $N's wounds.", FALSE, ch, NULL, vict, TO_NOTVICT);
	}
	
	// remove DoTs
	for (dot = vict->over_time_effects; dot; dot = next_dot) {
		next_dot = dot->next;

		if (dot->damage_type == DAM_PHYSICAL || dot->damage_type == DAM_POISON || dot->damage_type == DAM_FIRE) {
			dot_remove(vict, dot);
		}
	}
	
	heal(ch, vict, CHOOSE_BY_ABILITY_LEVEL(levels, ch, ABIL_FIRSTAID) + total_bonus_healing(ch));
	gain_ability_exp(ch, ABIL_FIRSTAID, 15);
	GET_WAIT_STATE(ch) += 2 RL_SEC;	// plus normal command_lag
}


ACMD(do_heartstop) {
	struct affected_type *af;
	char_data *victim = FIGHTING(ch);
	int cost = 30;

	one_argument(argument, arg);

	if (!can_use_ability(ch, ABIL_HEARTSTOP, MOVE, cost, COOLDOWN_HEARTSTOP)) {
		// nope
	}
	else if (!*arg && !FIGHTING(ch))
		msg_to_char(ch, "Stop whose heart?\r\n");
	else if (*arg && !(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (ch == victim)
		msg_to_char(ch, "You wouldn't want to do that to yourself...\r\n");
	else if (IS_GOD(victim) || IS_IMMORTAL(victim))
		msg_to_char(ch, "You cannot use this power on so godly a target!\r\n");
	else if (!can_fight(ch, victim))
		act("You can't attack $M!", FALSE, ch, 0, victim, TO_CHAR);
	else if (!IS_VAMPIRE(victim)) {
		msg_to_char(ch, "You can only use heartstop on vampires.\r\n");
	}
	else if (ABILITY_TRIGGERS(ch, victim, NULL, ABIL_HEARTSTOP)) {
		return;
	}
	else {
		if (SHOULD_APPEAR(ch)) {
			appear(ch);
		}
		
		charge_ability_cost(ch, MOVE, cost, COOLDOWN_HEARTSTOP, 30, WAIT_COMBAT_ABILITY);

		act("You grab $N and press hard against $S throat...", FALSE, ch, 0, victim, TO_CHAR);
		act("$n grabs you and presses hard against your throat...", FALSE, ch, 0, victim, TO_VICT);
		act("$n grabs $N and presses hard against $S throat...", TRUE, ch, 0, victim, TO_NOTVICT);

		if (!skill_check(ch, ABIL_HEARTSTOP, DIFF_HARD) || AFF_FLAGGED(victim, AFF_IMMUNE_BATTLE)) {
			msg_to_char(ch, "But nothing happens.\r\n");
			hit(victim, ch, GET_EQ(victim, WEAR_WIELD), FALSE);
			return;
		}
		
		gain_ability_exp(ch, ABIL_HEARTSTOP, 15);

		af = create_flag_aff(ATYPE_HEARTSTOP, 4, AFF_CANT_SPEND_BLOOD, ch);
		affect_join(victim, af, ADD_DURATION);

		msg_to_char(victim, "Your blood becomes inert!\r\n");
		
		hit(victim, ch, GET_EQ(victim, WEAR_WIELD), FALSE);
	}
}


ACMD(do_kick) {
	char_data *vict;
	int cost = 10;
	byte dam;
	bool success = FALSE;

	one_argument(argument, arg);
	
	if (!can_use_ability(ch, ABIL_KICK, MOVE, cost, COOLDOWN_KICK)) {
		return;
	}
	
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else {
			send_to_char("Kick whom?\r\n", ch);
			return;
		}
	}
	
	if (vict == ch) {
		send_to_char("Aren't we funny today...\r\n", ch);
		return;
	}
	if (FIGHT_MODE(vict) == FMODE_MISSILE || FIGHT_MODE(ch) == FMODE_MISSILE) {
		msg_to_char(ch, "You aren't close enough.\r\n");
		return;
	}
	if (!can_fight(ch, vict)) {
		act("You can't attack $N!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_KICK)) {
		return;
	}
	
	charge_ability_cost(ch, MOVE, cost, COOLDOWN_KICK, 6, WAIT_COMBAT_ABILITY);
	
	// determine hit
	success = IS_SPECIALTY_ABILITY(ch, ABIL_KICK) || check_hit_vs_dodge(ch, vict, FALSE);

	if (success) {
		if (HAS_ABILITY(ch, ABIL_SHADOW_KICK) && !AFF_FLAGGED(vict, AFF_IMMUNE_BATTLE)) {
			struct affected_type *af;
			int value = round(GET_COMPUTED_LEVEL(ch) / 50);
			af = create_mod_aff(ATYPE_SHADOW_KICK, 2, APPLY_BONUS_PHYSICAL, -value, ch);
			affect_join(vict, af, 0);
			af = create_mod_aff(ATYPE_SHADOW_KICK, 2, APPLY_BONUS_MAGICAL, -value, ch);
			affect_join(vict, af, 0);
		}
	
		dam = GET_STRENGTH(ch) * (IS_CLASS_ABILITY(ch, ABIL_KICK) ? 4 : 2);
		dam += GET_BONUS_PHYSICAL(ch);
		damage(ch, vict, dam, ATTACK_KICK, DAM_PHYSICAL);
	}
	else {
		damage(ch, vict, 0, ATTACK_KICK, DAM_PHYSICAL);
	}

	gain_ability_exp(ch, ABIL_KICK, 15);
}


ACMD(do_outrage) {
	char_data *victim, *next_vict;
	int base_cost = 50, add_cost = 25;
	bool found;
	
	if (!can_use_ability(ch, ABIL_OUTRAGE, MOVE, base_cost, COOLDOWN_OUTRAGE)) {
		return;
	}
	else if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_OUTRAGE)) {
		return;
	}
	else {
		if (SHOULD_APPEAR(ch)) {
			appear(ch);
		}
		
		charge_ability_cost(ch, MOVE, base_cost, COOLDOWN_OUTRAGE, 9, WAIT_COMBAT_ABILITY);
		
		msg_to_char(ch, "You spin wildly with outrage, hitting everything in sight!\r\n");
		act("$n spins wildly with outrage, hitting everything in sight!", FALSE, ch, NULL, NULL, TO_ROOM);
		
		found = FALSE;
		for (victim = ROOM_PEOPLE(IN_ROOM(ch)); victim && (!found || GET_MOVE(ch) >= add_cost); victim = next_vict) {
			next_vict = victim->next_in_room;
			
			if (victim != ch && IS_NPC(victim) && CAN_SEE(ch, victim) && skill_check(ch, ABIL_OUTRAGE, DIFF_MEDIUM) && !is_fight_ally(ch, victim) && !in_same_group(ch, victim) && can_fight(ch, victim)) {
				if (found) {
					GET_MOVE(ch) -= add_cost;
				}
				
				hit(ch, victim, GET_EQ(ch, WEAR_WIELD), FALSE);
				found = TRUE;
				
				// rescue check
				if (HAS_ABILITY(ch, ABIL_RESCUE) && FIGHTING(victim) && !IS_NPC(FIGHTING(victim)) && FIGHTING(victim) != ch && skill_check(ch, ABIL_RESCUE, DIFF_MEDIUM)) {
					perform_rescue(ch, FIGHTING(victim), victim);
					gain_ability_exp(ch, ABIL_RESCUE, 15);
				}
			}
		}
		
		if (found) {
			gain_ability_exp(ch, ABIL_OUTRAGE, 5);
		}
	}
}


ACMD(do_rescue) {
	char_data *vict, *tmp_ch;
	int cost = 20;
	bool found;

	one_argument(argument, arg);

	if (!can_use_ability(ch, ABIL_RESCUE, MOVE, cost, COOLDOWN_RESCUE)) {
		return;
	}
	
	if (!*arg) {
		// no-arg: rescue first group member who is getting hit
		found = FALSE;
		for (vict = ROOM_PEOPLE(IN_ROOM(ch)); vict && !found; vict = vict->next_in_room) {
			if (vict == ch || !FIGHTING(vict) || IS_NPC(FIGHTING(vict)) || FIGHTING(vict) == ch || !in_same_group(ch, FIGHTING(vict))) {
				continue;
			}
			if (is_fight_ally(ch, vict) || !can_fight(ch, vict) || FIGHT_MODE(vict) != FMODE_MELEE) {
				continue;
			}
			
			// seems legit
			found = TRUE;
			if (ABILITY_TRIGGERS(ch, vict, NULL, ABIL_RESCUE)) {
				return;
			}

			charge_ability_cost(ch, MOVE, cost, COOLDOWN_RESCUE, 6, WAIT_COMBAT_ABILITY);
			perform_rescue(ch, FIGHTING(vict), vict);
			gain_ability_exp(ch, ABIL_RESCUE, 15);
			break;
		}
		
		if (!found) {
			msg_to_char(ch, "Rescue whom?\r\n");
		}
		return;
	}
	
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char("Whom do you wish to rescue?\r\n", ch);
		return;
	}
	if (vict == ch) {
		send_to_char("What about fleeing instead?\r\n", ch);
		return;
	}
	if (FIGHTING(ch) == vict) {
		send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
		return;
	}
	if (GET_MOVE(ch) < cost) {
		msg_to_char(ch, "You need %d move points to do that.\r\n", cost);
		return;
	}

	for (tmp_ch = ROOM_PEOPLE(IN_ROOM(ch)); tmp_ch && (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

	if (!tmp_ch) {
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (FIGHT_MODE(tmp_ch) != FMODE_MELEE) {
		msg_to_char(ch, "You may only rescue someone engaged in melee combat.\r\n");
		return;
	}

	if (!can_fight(ch, tmp_ch)) {
		act("You can't attack $N!", FALSE, ch, 0, tmp_ch, TO_CHAR);
		return;
	}
	
	if (ABILITY_TRIGGERS(ch, tmp_ch, NULL, ABIL_RESCUE)) {
		return;
	}

	charge_ability_cost(ch, MOVE, cost, COOLDOWN_RESCUE, 6, WAIT_COMBAT_ABILITY);
	perform_rescue(ch, vict, tmp_ch);
	gain_ability_exp(ch, ABIL_RESCUE, 15);
}
