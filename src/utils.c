/* ************************************************************************
*   File: utils.c                                         EmpireMUD 2.0b3 *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2015                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <math.h>
#include <sys/time.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "skills.h"
#include "vnums.h"
#include "dg_scripts.h"

/**
* Contents:
*   Basic Utils
*   Empire Utils
*   Empire Diplomacy Utils
*   Empire Permissions Utils
*   File Utils
*   Logging Utils
*   Adventure Utils
*   Interpreter Utils
*   Mobile Utils
*   Object Utils
*   Player Utils
*   Resource Utils
*   Sector Utils
*   String Utils
*   Type Utils
*   World Utils
*   Misc Utils
*/

// external vars
extern const struct mine_data_type mine_data[];

// external funcs
void scale_item_to_level(obj_data *obj, int level);

// locals
#define WHITESPACE " \t"	// used by some of the string functions
bool emp_can_use_room(empire_data *emp, room_data *room, int mode);
bool is_trading_with(empire_data *emp, empire_data *partner);
void score_empires();


 //////////////////////////////////////////////////////////////////////////////
//// BASIC UTILS /////////////////////////////////////////////////////////////


/**
* This is usually called as GET_AGE() from utils.h
*
* @param char_data *ch The character.
* @return struct time_info_data* A time object indicating the player's age.
*/
struct time_info_data *age(char_data *ch) {
	static struct time_info_data player_age;

	player_age = *mud_time_passed(time(0), ch->player.time.birth);

	player_age.year += 17;	/* All players start at 17 */
	player_age.year -= config_get_int("starting_year");

	return (&player_age);
}


/**
* @param room_data *room The location
* @return bool TRUE if any players are in the room
*/
bool any_players_in_room(room_data *room) {
	char_data *ch;
	bool found = FALSE;
	
	for (ch = ROOM_PEOPLE(room); !found && ch; ch = ch->next_in_room) {
		if (!IS_NPC(ch)) {
			found = TRUE;
		}
	}
	
	return found;
}


/**
* Applies bonus traits whose effects are one-time-only.
*
* @param char_data *ch The player to apply the trait to.
* @param bitvector_t trait Any BONUS_x bit.
* @param bool add If TRUE, we are adding the trait; FALSE means removing it;
*/
void apply_bonus_trait(char_data *ch, bitvector_t trait, bool add) {	
	int amt = (add ? 1 : -1);
	
	if (IS_SET(trait, BONUS_EXTRA_DAILY_SKILLS)) {
		GET_DAILY_BONUS_EXPERIENCE(ch) = MAX(0, GET_DAILY_BONUS_EXPERIENCE(ch) + (config_get_int("num_bonus_trait_daily_skills") * amt));
	}
	if (IS_SET(trait, BONUS_STRENGTH)) {
		ch->real_attributes[STRENGTH] = MAX(1, MIN(att_max(ch), ch->real_attributes[STRENGTH] + amt));
	}
	if (IS_SET(trait, BONUS_DEXTERITY)) {
		ch->real_attributes[DEXTERITY] = MAX(1, MIN(att_max(ch), ch->real_attributes[DEXTERITY] + amt));
	}
	if (IS_SET(trait, BONUS_CHARISMA)) {
		ch->real_attributes[CHARISMA] = MAX(1, MIN(att_max(ch), ch->real_attributes[CHARISMA] + amt));
	}
	if (IS_SET(trait, BONUS_GREATNESS)) {
		ch->real_attributes[GREATNESS] = MAX(1, MIN(att_max(ch), ch->real_attributes[GREATNESS] + amt));
	}
	if (IS_SET(trait, BONUS_INTELLIGENCE)) {
		ch->real_attributes[INTELLIGENCE] = MAX(1, MIN(att_max(ch), ch->real_attributes[INTELLIGENCE] + amt));
	}
	if (IS_SET(trait, BONUS_WITS)) {
		ch->real_attributes[WITS] = MAX(1, MIN(att_max(ch), ch->real_attributes[WITS] + amt));
	}
	
	if (IN_ROOM(ch)) {
		affect_total(ch);
	}
}


/**
* If no ch is supplied, this uses the max npc value by default.
*
* @param char_data *ch (Optional) The person to check the max for.
* @return int The maximum attribute level.
*/
int att_max(char_data *ch) {	
	int max = 1;

	if (ch && !IS_NPC(ch)) {
		max = config_get_int("max_player_attribute");
	}
	else {
		max = config_get_int("max_npc_attribute");
	}

	return max;
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(char_data *ch, char_data *victim) {
	char_data *k;

	for (k = victim; k; k = k->master) {
		if (k == ch) {
			return (TRUE);
		}
	}

	return (FALSE);
}


/**
* @param bitvector_t bitset The bit flags to count.
* @return int The number of flags active in the set.
*/
int count_bits(bitvector_t bitset) {
	bitvector_t set;
	int count = 0;
	
	for (set = bitset; set; set >>= 1) {
		if (set & 1) {
			++count;
		}
	}
	
	return count;
}


/* simulates dice roll */
int dice(int number, int size) {
	unsigned long empire_random();

	int sum = 0;

	if (size <= 0 || number <= 0)
		return (0);

	while (number-- > 0)
		sum += ((empire_random() % size) + 1);

	return (sum);
}


/**
* Diminishing returns formula from lostsouls.org -- the one change I made is
* that it won't return a number of greater magnitude than the original 'val',
* as I noticed that a 15 on a scale of 20 returned 16. The intention is to
* diminish the value, not raise it.
*
* 'scale' is the level at which val is unchanged -- at higher levels, it is
* greatly reduced.
*
* @param double val The base value.
* @param double scale The level after which val diminishes.
* @return double The diminished value.
*/
double diminishing_returns(double val, double scale) {
	// nonsense!
	if (scale == 0) {
		return 0;
	}
	
    if (val < 0)
        return -diminishing_returns(-val, scale);
    double mult = val / scale;
    double trinum = (sqrt(8.0 * mult + 1.0) - 1.0) / 2.0;
    double final = trinum * scale;
    
    if (final > val) {
    	return val;
    }
    
    return final;
}


/**
* @param char_data *ch
* @return bool TRUE if ch has played at least 1 day (or is an npc)
*/
bool has_one_day_playtime(char_data *ch) {
	struct time_info_data playing_time;

	if (IS_NPC(ch)) {
		return TRUE;
	}
	else {
		playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);

		return playing_time.day >= 1;
	}
}


/**
* @param char_data *ch The player to check.
* @return int The total number of bonus traits a character deserves.
*/
int num_earned_bonus_traits(char_data *ch) {
	struct time_info_data t;
	int count = 1;	// all players deserve 1
	
	if (IS_NPC(ch)) {
		return 0;
	}
	
	// extra point at 2 days playtime
	t = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
	if (t.day >= 2) {
		++count;
	}

	return count;
}


/* creates a random number in interval [from;to] */
int number(int from, int to) {
	unsigned long empire_random();

	// shortcut -paul 12/9/2014
	if (from == to) {
		return from;
	}

	/* error checking in case people call number() incorrectly */
	if (from > to) {
		int tmp = from;
		from = to;
		to = tmp;
	}

	return ((empire_random() % (to - from + 1)) + from);
}


/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1) {
	long secs;
	static struct time_info_data now;

	secs = (long) (t2 - t1);

	now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_MUD_HOUR * now.hours;

	now.day = (secs / SECS_PER_MUD_DAY) % 30;		/* 0..29 days  */
	secs -= SECS_PER_MUD_DAY * now.day;

	now.month = (secs / SECS_PER_MUD_MONTH) % 12;	/* 0..11 months */
	secs -= SECS_PER_MUD_MONTH * now.month;

	now.year = config_get_int("starting_year") + (secs / SECS_PER_MUD_YEAR);		/* 0..XX? years */

	return (&now);
}


/**
* PERS is essentially the $n from act() -- how one character is displayed to
* another.
*
* @param char_data *ch The person being displayed.
* @param char_data *vict The person who is seeing them.
* @param bool real If TRUE, uses real name instead of disguise/morph.
*/
char *PERS(char_data *ch, char_data *vict, bool real) {
	extern char *morph_string(char_data *ch, byte type);
	
	static char output[MAX_INPUT_LENGTH];

	if (!CAN_SEE(vict, ch)) {
		return "someone";
	}

	if (!IS_NPC(ch) && GET_MORPH(ch) && !real) {
		return morph_string(ch, MORPH_STRING_NAME);
	}
	
	if (!real && IS_DISGUISED(ch)) {
		return GET_DISGUISED_NAME(ch);
	}

	strcpy(output, GET_NAME(ch));
	if (!IS_NPC(ch) && GET_LASTNAME(ch)) {
		sprintf(output + strlen(output), " %s", GET_LASTNAME(ch));
	}

	return output;
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1) {
	long secs;
	static struct time_info_data now;

	secs = (long) (t2 - t1);

	now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_REAL_HOUR * now.hours;

	now.day = (secs / SECS_PER_REAL_DAY);	/* 0..29 days  */
	/* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

	now.month = -1;
	now.year = -1;

	return (&now);
}


 //////////////////////////////////////////////////////////////////////////////
//// EMPIRE UTILS ////////////////////////////////////////////////////////////


/**
* @param empire_data *emp
* @return int the number of members online
*/
int count_members_online(empire_data *emp) {
	descriptor_data *d;
	int count = 0;
	
	for (d = descriptor_list; d; d = d->next) {
		if (STATE(d) == CON_PLAYING && d->character && GET_LOYALTY(d->character) == emp) {
			++count;
		}
	}
	
	return count;
}


/**
* @param empire_data *emp An empire
* @return int The number of technologies known by that empire
*/
int count_tech(empire_data *emp) {
	int iter, count = 0;
	
	for (iter = 0; iter < NUM_TECHS; ++iter) {
		if (EMPIRE_HAS_TECH(emp, iter)) {
			++count;
		}
	}
	
	return count;
}


/**
* Gets an empire's total score -- only useful if you've called score_empires()
*
* @param empire_data *emp The empire to check.
* @return int The empire's score.
*/
int get_total_score(empire_data *emp) {
	int total, iter;
	
	if (!emp) {
		return 0;
	}
	
	total = 0;
	for (iter = 0; iter < NUM_SCORES; ++iter) {
		total += EMPIRE_SCORE(emp, iter);
	}
	
	return total;
}


/**
* Runs imports from one empire. By the time this function is called, we only
* know they are active empires and trading partners.
*
* @param empire_data *emp The empire importing.
* @param empire_data *partner The trading partner.
* @param int *limit Pointer to the number imported so far today -- to limit the total.
*/
void process_import_pair(empire_data *emp, empire_data *partner, int *limit) {
	extern int get_main_island(empire_data *emp);
	
	struct empire_trade_data *trade, *p_trade, **trade_list = NULL, **partner_list = NULL;
	int *trade_list_cost = NULL, *trade_list_count = NULL;
	double rate = exchange_rate(emp, partner);
	int my_amt, their_amt, found_island, iter, trade_list_size = 0, owed;
	bool found_any;
	obj_data *orn;
	
	int imports_per_day = config_get_int("imports_per_day");
	
	// could we even afford them?
	if (rate < 0.01) {
		return;
	}
	
	// find items to trade with this empire: construct a list of valid trades
	for (trade = EMPIRE_TRADE(emp); trade; trade = trade->next) {
		if (trade->type != TRADE_IMPORT) {
			continue;
		}
		
		// do we need it?
		my_amt = get_total_stored_count(emp, trade->vnum, TRUE);	// count shipping
		if (my_amt >= trade->limit) {
			continue;
		}
		
		// do THEY have it?
		if (!(p_trade = find_trade_entry(partner, TRADE_EXPORT, trade->vnum))) {
			continue;
		}
		
		// do they have enough?
		their_amt = get_total_stored_count(partner, trade->vnum, FALSE);	// don't count shipping -- it's not tradable
		if (their_amt <= p_trade->limit) {
			continue;
		}
		
		// will we pay that much? (we compare this at *imports_per_day on both sides because the cost-per-one may have misleading rounding
		if ((int) round(p_trade->cost * imports_per_day * (1/rate)) > trade->cost * imports_per_day) {
			continue;
		}
		
		// possible trade! add to list
		if (trade_list_size > 0) {
			RECREATE(trade_list, struct empire_trade_data*, trade_list_size+1);
			RECREATE(partner_list, struct empire_trade_data*, trade_list_size+1);
		}
		else {
			CREATE(trade_list, struct empire_trade_data*, trade_list_size+1);
			CREATE(partner_list, struct empire_trade_data*, trade_list_size+1);
		}
		
		trade_list[trade_list_size] = trade;
		partner_list[trade_list_size] = p_trade;
		
		++trade_list_size;
	}
	
	// set up costs so we know what to log later
	if (trade_list_size > 0) {
		CREATE(trade_list_cost, int, trade_list_size);
		CREATE(trade_list_count, int, trade_list_size);
		for (iter = 0; iter < trade_list_size; ++iter) {
			trade_list_cost[iter] = 0;
			trade_list_count[iter] = 0;
		}
	}
	
	// did we find any?
	owed = 0;
	do {
		found_any = FALSE;
		for (iter = 0; iter < trade_list_size && *limit < imports_per_day; ++iter) {
			// do they still have any?
			their_amt = get_total_stored_count(partner, trade_list[iter]->vnum, FALSE);	// don't count shipping; it's not tradable
			if (their_amt <= partner_list[iter]->limit) {
				continue;
			}

			// do we still it?
			my_amt = get_total_stored_count(emp, trade_list[iter]->vnum, TRUE);	// do count shipping
			if (my_amt >= trade_list[iter]->limit) {
				continue;
			}
			
			// can afford? (comparing total owed because low values may not rate-convert correctly)
			if (EMPIRE_COINS(emp) < (int) round((owed + partner_list[iter]->cost) * (1/rate))) {
				continue;
			}
			
			// trade ok
			owed += partner_list[iter]->cost;
			
			// only store it if we did find an island to store to
			if ((found_island = get_main_island(emp)) != NO_ISLAND) {
				add_to_empire_storage(emp, found_island, trade_list[iter]->vnum, 1);
				charge_stored_resource(partner, ANY_ISLAND, trade_list[iter]->vnum, 1);
			
				trade_list_cost[iter] += partner_list[iter]->cost;
				++trade_list_count[iter];
			
				// one trade done
				*limit += 1;
				found_any = TRUE;
			}
		}
	} while (found_any);
	
	// settle up
	if (owed > 0) {
		decrease_empire_coins(emp, emp, (int)round(owed * (1/rate)));
		increase_empire_coins(partner, partner, owed);
	}
	
	// anything to log?
	for (iter = 0; iter < trade_list_size; ++iter) {
		if (trade_list_count[iter] > 0) {
			orn = obj_proto(trade_list[iter]->vnum);
			log_to_empire(emp, ELOG_TRADE, "Imported %s x%d from %s for %d coins", GET_OBJ_SHORT_DESC(orn), trade_list_count[iter], EMPIRE_NAME(partner), (int)round(trade_list_cost[iter] * (1/rate)));
			log_to_empire(partner, ELOG_TRADE, "Exported %s x%d to %s for %d coins", GET_OBJ_SHORT_DESC(orn), trade_list_count[iter], EMPIRE_NAME(emp), trade_list_cost[iter]);
		}
	}
	
	if (trade_list) {
		free(trade_list);
	}
	if (partner_list) {
		free(partner_list);
	}
	if (trade_list_cost) {
		free(trade_list_cost);
	}
	if (trade_list_count) {
		free(trade_list_count);
	}
}


// runs daily imports
void process_imports(void) {
	void read_vault(empire_data *emp);
	
	empire_data *emp, *next_emp, *partner, *next_partner;
	int limit;
	
	int imports_per_day = config_get_int("imports_per_day");
	int time_to_empire_emptiness = config_get_int("time_to_empire_emptiness") * SECS_PER_REAL_WEEK;
	
	HASH_ITER(hh, empire_table, emp, next_emp) {
		if (EMPIRE_IMM_ONLY(emp)) {
			continue;
		}
		if (!EMPIRE_HAS_TECH(emp, TECH_TRADE_ROUTES)) {
			continue;
		}
		if (EMPIRE_LAST_LOGON(emp) + time_to_empire_emptiness < time(0)) {
			continue;
		}
		
		// for periodic trade limit
		limit = 0;
		
		// find trading partners in order of empire rank
		HASH_ITER(hh, empire_table, partner, next_partner) {
			if (limit >= imports_per_day) {
				break;
			}
			
			if (!is_trading_with(emp, partner)) {
				continue;
			}
			
			process_import_pair(emp, partner, &limit);
		}
		
		read_vault(emp);
	}
}


/**
* Trigger a score update and re-sort of the empires.
*/
void resort_empires(void) {
	extern int sort_empires(empire_data *a, empire_data *b);
	
	static time_t last_sort_time = 0;
	static int last_sort_size = 0;

	// prevent constant re-sorting by limiting this to once per minute
	if (last_sort_size != HASH_COUNT(empire_table) || last_sort_time + (60 * 1) < time(0)) {
		// better score them first
		score_empires();
		HASH_SORT(empire_table, sort_empires);

		last_sort_size = HASH_COUNT(empire_table);
		last_sort_time = time(0);
	}
}


/**
* This function assigns scores to all empires so they can be ranked on the
* empire list.
*/
void score_empires(void) {
	extern double empire_score_average[NUM_SCORES];
	extern const double score_levels[];
	
	int iter, pos, total[NUM_SCORES], max[NUM_SCORES], num_emps = 0;
	struct empire_political_data *pol;
	struct empire_storage_data *store;
	empire_data *emp, *next_emp;
	long long num;
	
	int time_to_empire_emptiness = config_get_int("time_to_empire_emptiness") * SECS_PER_REAL_WEEK;

	// clear data	
	for (iter = 0; iter < NUM_SCORES; ++iter) {
		total[iter] = 0;
		empire_score_average[iter] = 0;
		max[iter] = 0;
	}
	
	// clear scores first
	HASH_ITER(hh, empire_table, emp, next_emp) {
		for (iter = 0; iter < NUM_SCORES; ++iter) {
			EMPIRE_SCORE(emp, iter) = 0;
		}
		EMPIRE_SORT_VALUE(emp) = 0;
	}
	
	#define SCORE_SKIP_EMPIRE(ee)  (EMPIRE_IMM_ONLY(ee) || EMPIRE_LAST_LOGON(ee) + time_to_empire_emptiness < time(0))

	// build data
	HASH_ITER(hh, empire_table, emp, next_emp) {
		if (SCORE_SKIP_EMPIRE(emp)) {
			continue;
		}
		
		// found one
		++num_emps;
		
		// detect scores -- and store later for easy comparison
		total[SCORE_WEALTH] += GET_TOTAL_WEALTH(emp);
		max[SCORE_WEALTH] = MAX(GET_TOTAL_WEALTH(emp), max[SCORE_WEALTH]);
		EMPIRE_SCORE(emp, SCORE_WEALTH) = GET_TOTAL_WEALTH(emp);
		
		total[SCORE_TERRITORY] += (num = land_can_claim(emp, FALSE));
		max[SCORE_TERRITORY] = MAX(num, max[SCORE_TERRITORY]);
		EMPIRE_SCORE(emp, SCORE_TERRITORY) = num;
		
		total[SCORE_MEMBERS] += EMPIRE_MEMBERS(emp);
		max[SCORE_MEMBERS] = MAX(EMPIRE_MEMBERS(emp), max[SCORE_MEMBERS]);
		EMPIRE_SCORE(emp, SCORE_MEMBERS) = EMPIRE_MEMBERS(emp);
		
		total[SCORE_TECHS] += (num = count_tech(emp));
		max[SCORE_TECHS] = MAX(num, max[SCORE_TECHS]);
		EMPIRE_SCORE(emp, SCORE_TECHS) = num;
		
		num = 0;
		for (store = EMPIRE_STORAGE(emp); store; store = store->next) {
			num += store->amount;
		}
		num /= 1000;	// for sanity of number size
		total[SCORE_EINV] += num;
		max[SCORE_EINV] = MAX(num, max[SCORE_EINV]);
		EMPIRE_SCORE(emp, SCORE_EINV) = num;
		
		total[SCORE_GREATNESS] += EMPIRE_GREATNESS(emp);
		max[SCORE_GREATNESS] = MAX(EMPIRE_GREATNESS(emp), max[SCORE_GREATNESS]);
		EMPIRE_SCORE(emp, SCORE_GREATNESS) = EMPIRE_GREATNESS(emp);
		
		num = 0;
		for (pol = EMPIRE_DIPLOMACY(emp); pol; pol = pol->next) {
			if (IS_SET(pol->type, DIPL_TRADE)) {
				++num;
			}
		}
		total[SCORE_DIPLOMACY] += num;
		max[SCORE_DIPLOMACY] = MAX(num, max[SCORE_DIPLOMACY]);
		EMPIRE_SCORE(emp, SCORE_DIPLOMACY) = num;
		
		total[SCORE_FAME] += EMPIRE_FAME(emp);
		max[SCORE_FAME] = MAX(EMPIRE_FAME(emp), max[SCORE_FAME]);
		EMPIRE_SCORE(emp, SCORE_FAME) = EMPIRE_FAME(emp);
		
		total[SCORE_MILITARY] += EMPIRE_MILITARY(emp);
		max[SCORE_MILITARY] = MAX(EMPIRE_MILITARY(emp), max[SCORE_MILITARY]);
		EMPIRE_SCORE(emp, SCORE_MILITARY) = EMPIRE_MILITARY(emp);
		
		total[SCORE_PLAYTIME] += EMPIRE_TOTAL_PLAYTIME(emp);
		max[SCORE_PLAYTIME] = MAX(EMPIRE_TOTAL_PLAYTIME(emp), max[SCORE_PLAYTIME]);
		EMPIRE_SCORE(emp, SCORE_PLAYTIME) = EMPIRE_TOTAL_PLAYTIME(emp);
	}
	
	// determine avgs
	for (iter = 0; iter < NUM_SCORES; ++iter) {
		empire_score_average[iter] = ((double) total[iter]) / MAX(1, num_emps);
	}
	
	// apply scores to empires based on how they fared
	HASH_ITER(hh, empire_table, emp, next_emp) {
		if (SCORE_SKIP_EMPIRE(emp)) {
			continue;
		}
			
		for (iter = 0; iter < NUM_SCORES; ++iter) {
			num = 0;
			
			// score_levels[] terminates with a -1 but I don't trust doubles enough to do anything other than >= 0 here
			for (pos = 0; score_levels[pos] >= 0; ++pos) {
				if (EMPIRE_SCORE(emp, iter) >= (empire_score_average[iter] * score_levels[pos])) {
					++num;
				}
			}
			
			// assign permanent score
			EMPIRE_SORT_VALUE(emp) += EMPIRE_SCORE(emp, iter);
			EMPIRE_SCORE(emp, iter) = num;
		}
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// EMPIRE DIPLOMACY UTILS //////////////////////////////////////////////////

/**
* This function prevents a player from building/claiming a location while
* at war with a person whose city it would be within.
*
* @param char_data *ch The person trying to build/claim.
* @param room_data *loc The map location.
* @return bool TRUE if it's okay to build/claim; FALSE if not.
*/
bool can_build_or_claim_at_war(char_data *ch, room_data *loc) {
	struct empire_political_data *pol;
	empire_data *enemy;
	bool junk;
	
	// if they aren't at war, this doesn't apply
	if (!ch || !is_at_war(GET_LOYALTY(ch))) {
		return TRUE;
	}
	
	// if they already own it, it's ok
	if (GET_LOYALTY(ch) && ROOM_OWNER(IN_ROOM(ch)) == GET_LOYALTY(ch)) {
		return TRUE;
	}
	
	// if it's in one of their OWN cities, it's ok
	if (GET_LOYALTY(ch) && is_in_city_for_empire(loc, GET_LOYALTY(ch), TRUE, &junk)) {
		return TRUE;
	}
	
	// find who they are at war with
	for (pol = EMPIRE_DIPLOMACY(GET_LOYALTY(ch)); pol; pol = pol->next) {
		if (IS_SET(pol->type, DIPL_WAR)) {
			// not good if they are trying to build in a location owned by the other player while at war
			if ((enemy = real_empire(pol->id)) && is_in_city_for_empire(loc, enemy, TRUE, &junk)) {
				return FALSE;
			}
		}
	}
	
	return TRUE;
}


/**
* Similar to has_relationship except it takes two characters, and attempts to
* check their player masters first. It also considers npc affinities, if there
* is no player master.
*
* @param char_data *ch_a One character.
* @param char_data *ch_b Another character.
* @param bitvector_t dipl_bits Any DIPL_x flags.
* @return bool TRUE if the characters have the requested diplomacy.
*/
bool char_has_relationship(char_data *ch_a, char_data *ch_b, bitvector_t dipl_bits) {
	char_data *top_a, *top_b;
	empire_data *emp_a, *emp_b;
	
	if (!ch_a || !ch_b || ch_a == ch_b) {
		return FALSE;
	}
	
	// find a player for each
	top_a = ch_a;
	while (IS_NPC(top_a) && top_a->master) {
		top_a = top_a->master;
	}
	top_b = ch_b;
	while (IS_NPC(top_b) && top_b->master) {
		top_b = top_b->master;
	}
	
	// look up 
	emp_a = GET_LOYALTY(top_a);
	emp_b = GET_LOYALTY(top_b);
	
	if (!emp_a || !emp_b) {
		return FALSE;
	}
	
	return has_relationship(emp_a, emp_b, dipl_bits);
}


/**
* @param empire_data *emp empire A
* @param empire_data *enemy empire B
* @return bool TRUE if emp is nonaggression/ally/trade/peace
*/
bool empire_is_friendly(empire_data *emp, empire_data *enemy) {
	struct empire_political_data *pol;
	
	if (emp && enemy) {
		if ((pol = find_relation(emp, enemy))) {
			if (IS_SET(pol->type, DIPL_ALLIED | DIPL_NONAGGR | DIPL_TRADE | DIPL_PEACE)) {
				return TRUE;
			}
		}
	}

	return FALSE;
}


/**
* @param empire_data *emp empire A
* @param empire_data *enemy empire B
* @param room_data *loc The location to check (optional; pass NULL to ignore)
* @return bool TRUE if emp is at war/distrust
*/
bool empire_is_hostile(empire_data *emp, empire_data *enemy, room_data *loc) {
	struct empire_political_data *pol;
	bool distrustful = FALSE;
	
	if (emp == enemy || !emp) {
		return FALSE;
	}
	
	if ((!loc && IS_SET(EMPIRE_FRONTIER_TRAITS(emp), ETRAIT_DISTRUSTFUL)) || has_empire_trait(emp, loc, ETRAIT_DISTRUSTFUL)) {
		distrustful = TRUE;
	}
	
	if (emp && enemy) {
		if ((pol = find_relation(emp, enemy))) {
			if (IS_SET(pol->type, DIPL_WAR | DIPL_DISTRUST) || (distrustful && !empire_is_friendly(emp, enemy))) {
				return TRUE;
			}
		}
	}

	return (distrustful && !empire_is_friendly(emp, enemy));
}


/**
* Determines if an empire has a trait set at a certain location. It auto-
* matically detects if the location is covered by a city's traits or the
* empire's frontier traits.
*
* @param empire_data *emp The empire to check.
* @param room_data *loc The location to check (optional; pass NULL to ignore).
* @param bitvector_t ETRAIT_x flag(s).
* @return bool TRUE if the empire has that (those) trait(s) at loc.
*/
bool has_empire_trait(empire_data *emp, room_data *loc, bitvector_t trait) {
	struct empire_city_data *city;
	bitvector_t set = NOBITS;
	
	// short-circuit
	if (!emp) {
		return FALSE;
	}
	
	// determine which location to use
	if (loc && ((city = find_closest_city(emp, loc)) && compute_distance(loc, city->location) < config_get_int("city_trait_radius"))) {
		set = city->traits;
	}
	else {
		set = EMPIRE_FRONTIER_TRAITS(emp);
	}
	
	return (IS_SET(set, trait) ? TRUE : FALSE);
}


/**
* @param empire_data *emp one empire
* @param empire_data *fremp other empire
* @param bitvector_t diplomacy DIPL_x
* @return TRUE if emp and fremp have the given diplomacy with each other
*/
bool has_relationship(empire_data *emp, empire_data *fremp, bitvector_t diplomacy) {
	struct empire_political_data *pol;
	
	if (emp && fremp) {
		if ((pol = find_relation(emp, fremp))) {
			return IS_SET(pol->type, diplomacy) ? TRUE : FALSE;
		}
	}
	
	return FALSE;
}


/**
* Determines if an empire is currently participating in any wars.
*
* @param empire_data *emp The empire to check.
* @return bool TRUE if the empire is in at least one war.
*/
bool is_at_war(empire_data *emp) {
	struct empire_political_data *pol;
	
	if (!emp) {
		return FALSE;
	}
	
	for (pol = EMPIRE_DIPLOMACY(emp); pol; pol = pol->next) {
		if (IS_SET(pol->type, DIPL_WAR)) {
			return TRUE;
		}
	}
	
	return FALSE;
}


/**
* Determine if 2 empires are capable of trading with each other. This returns
* false if either is unable to trade for any reason.
*
* @param empire_data *emp The first empire.
* @param empire_data *partner The other empire. (argument order does not matter)
* @return bool TRUE if the two empires are trading and able to trade.
*/
bool is_trading_with(empire_data *emp, empire_data *partner) {
	int time_to_empire_emptiness = config_get_int("time_to_empire_emptiness") * SECS_PER_REAL_WEEK;
	
	// no self-trades or invalid empires
	if (emp == partner || !emp || !partner) {
		return FALSE;
	}
	// neither can be imm-only
	if (EMPIRE_IMM_ONLY(emp) || EMPIRE_IMM_ONLY(partner)) {
		return FALSE;
	}
	// both must have trade routes
	if (!EMPIRE_HAS_TECH(emp, TECH_TRADE_ROUTES) || !EMPIRE_HAS_TECH(partner, TECH_TRADE_ROUTES)) {
		return FALSE;
	}
	// neither can have hit the emptiness point
	if (EMPIRE_LAST_LOGON(emp) + time_to_empire_emptiness < time(0) || EMPIRE_LAST_LOGON(partner) + time_to_empire_emptiness < time(0)) {
		return FALSE;
	}
	// they need to have trade relations
	if (!has_relationship(emp, partner, DIPL_TRADE)) {
		return FALSE;
	}
	
	// ok!
	return TRUE;
}


 //////////////////////////////////////////////////////////////////////////////
//// EMPIRE PERMISSIONS UTILS ////////////////////////////////////////////////

/**
* Determines if a character can legally claim land.
*
* @param char_data *ch The player.
* @return bool TRUE if the player is allowed to claim.
*/
bool can_claim(char_data *ch) {
	empire_data *e;

	if (IS_NPC(ch))
		return FALSE;
	if (!(e = GET_LOYALTY(ch)))
		return TRUE;
	if (EMPIRE_CITY_TERRITORY(e) + EMPIRE_OUTSIDE_TERRITORY(e) >= land_can_claim(e, FALSE))
		return FALSE;
	if (GET_RANK(ch) < EMPIRE_PRIV(e, PRIV_CLAIM))
		return FALSE;
	return TRUE;
}


/**
* Determines if a character an use a room (chop, withdraw, etc).
* Unclaimable rooms are ok for GUESTS_ALLOWED.
*
* @param char_data *ch The character.
* @param room_data *room The location.
* @param int mode -- GUESTS_ALLOWED, MEMBERS_AND_ALLIES, MEMBERS_ONLY
* @return bool TRUE if ch can use room, FALSE otherwise
*/
bool can_use_room(char_data *ch, room_data *room, int mode) {
	room_data *homeroom = HOME_ROOM(room);

	// no owner?
	if (!ROOM_OWNER(homeroom)) {
		return TRUE;
	}
	// empire ownership
	if (ROOM_OWNER(homeroom) == GET_LOYALTY(ch)) {
		// private room?
		if (ROOM_PRIVATE_OWNER(homeroom) == NOBODY || ROOM_PRIVATE_OWNER(homeroom) == GET_IDNUM(ch) || GET_RANK(ch) == EMPIRE_NUM_RANKS(ROOM_OWNER(homeroom))) {
			return TRUE;
		}
	}
	
	// otherwise it's just whether ch's empire can use it
	return emp_can_use_room(GET_LOYALTY(ch), room, mode);
}


/**
* Determines if an empire can use a room (e.g. send a ship through it).
* Unclaimable rooms are ok for GUESTS_ALLOWED.
*
* @param empire_data *emp The empire trying to use it.
* @param room_data *room The location.
* @param int mode -- GUESTS_ALLOWED, MEMBERS_AND_ALLIES, MEMBERS_ONLY
* @return bool TRUE if emp can use room, FALSE otherwise
*/
bool emp_can_use_room(empire_data *emp, room_data *room, int mode) {
	room_data *homeroom = HOME_ROOM(room);

	// unclaimable always denies MEMBERS_x
	if (mode != GUESTS_ALLOWED && ROOM_AFF_FLAGGED(room, ROOM_AFF_UNCLAIMABLE)) {
		return FALSE;
	}
	// no owner?
	if (!ROOM_OWNER(homeroom)) {
		return TRUE;
	}
	// public + guests
	if (ROOM_AFF_FLAGGED(homeroom, ROOM_AFF_PUBLIC) && mode == GUESTS_ALLOWED) {
		return TRUE;
	}
	// empire ownership
	if (ROOM_OWNER(homeroom) == emp) {
		return TRUE;
	}
	// check allies if not a private room
	if (mode != MEMBERS_ONLY && ROOM_PRIVATE_OWNER(homeroom) == NOBODY && has_relationship(ROOM_OWNER(homeroom), emp, DIPL_ALLIED)) {
		return TRUE;
	}
	
	// newp
	return FALSE;
}


/**
* Checks the room to see if ch has permission.
*
* @param char_data *ch
* @param int type PRIV_x
* @return bool TRUE if it's ok
*/
bool has_permission(char_data *ch, int type) {
	empire_data *emp = ROOM_OWNER(HOME_ROOM(IN_ROOM(ch)));
	
	if (!can_use_room(ch, IN_ROOM(ch), MEMBERS_AND_ALLIES)) {
		return FALSE;
	}
	else if (emp && GET_LOYALTY(ch) == emp && GET_RANK(ch) < EMPIRE_PRIV(emp, type)) {
		// for empire members only
		return FALSE;
	}
	else if (emp && GET_LOYALTY(ch) != emp && EMPIRE_PRIV(emp, type) > 1) {
		// allies can't use things that are above rank 1 in the owner's empire
		return FALSE;
	}
	
	return TRUE;
}


/**
* This determines if the character's current location has the tech available.
* It takes the location into account, not just the tech flags.
*
* @param char_data *ch acting character, although this is location-based
* @param int tech TECH_x id
* @return TRUE if successful, FALSE if not (and sends its own error message to ch)
*/
bool has_tech_available(char_data *ch, int tech) {
	extern const char *techs[];

	if (!ROOM_OWNER(IN_ROOM(ch))) {
		msg_to_char(ch, "In order to do that you need to be in the territory of an empire with %s.\r\n", techs[tech]);
		return FALSE;
	}
	else if (!has_tech_available_room(IN_ROOM(ch), tech)) {
		msg_to_char(ch, "In order to do that you need to be in the territory of an empire with %s.\r\n", techs[tech]);
		return FALSE;
	}
	else {
		return TRUE;
	}
}


/**
* This determines if the given location has the tech available.
* It takes the location into account, not just the tech flags.
*
* @param room_data *room The location to check.
* @param int tech TECH_x id
* @return TRUE if successful, FALSE if not (and sends its own error message to ch)
*/
bool has_tech_available_room(room_data *room, int tech) {
	extern const int techs_requiring_same_island[];
	
	empire_data *emp = ROOM_OWNER(room);
	bool requires_island = FALSE;
	int iter;
	
	// see if it requires island
	for (iter = 0; techs_requiring_same_island[iter] != NOTHING; ++iter) {
		if (tech == techs_requiring_same_island[iter]) {
			requires_island = TRUE;
			break;
		}
	}

	if (!emp) {
		return FALSE;
	}
	else if (!(requires_island ? EMPIRE_HAS_TECH_ON_ISLAND(emp, GET_ISLAND_ID(room), tech) : EMPIRE_HAS_TECH(emp, tech))) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}


/**
* Calculates the total claimable land for an empire.
*
* @param empire_data *emp An empire number.
* @param bool outside_only If TRUE, only the amount of territory that can be outside cities.
* @return int The total claimable land.
*/
int land_can_claim(empire_data *emp, bool outside_only) {
	int from_wealth, total = 0;
	
	if (emp) {
		total += EMPIRE_GREATNESS(emp) * config_get_int("land_per_greatness");
		total += count_tech(emp) * config_get_int("land_per_tech");
		
		if (EMPIRE_HAS_TECH(emp, TECH_COMMERCE)) {
			// diminishes by an amount equal to non-wealth territory
			from_wealth = diminishing_returns((int) (GET_TOTAL_WEALTH(emp) * config_get_double("land_per_wealth")), total);
			
			// limited to 3x non-wealth territory
			from_wealth = MIN(from_wealth, total * 3);
			
			// for a total of 4x
			total += from_wealth;
		}
	}
	
	if (outside_only) {
		total *= config_get_double("land_outside_city_modifier");
	}
	
	return total;
}


 //////////////////////////////////////////////////////////////////////////////
//// FILE UTILS //////////////////////////////////////////////////////////////

/**
* This initiates the autowiz program, which writes fresh wizlist files.
*
* TODO like many player-reading systems, this could be done internally
* instead of with an external call.
*
* @param char_data *ch The player to check. Only runs autowiz if the player is a god.
*/
void check_autowiz(char_data *ch) {
	if (GET_ACCESS_LEVEL(ch) >= LVL_GOD && config_get_bool("use_autowiz")) {
		char buf[128];

		sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", LVL_START_IMM, WIZLIST_FILE, LVL_GOD, GODLIST_FILE, (int) getpid());

		syslog(SYS_INFO, 0, TRUE, "Initiating autowiz.");
		system(buf);
	}
}


/**
* Gets the filename/path for various name-divided file directories.
*
* @param char *orig_name The player name.
* @param char *filename A variable to write the filename to.
* @param int mode CRASH_FILE, ALIAS_FILE, etc.
* @return int 1=success, 0=fail
*/
int get_filename(char *orig_name, char *filename, int mode) {
	const char *prefix, *middle, *suffix;
	char name[64], *ptr;

	if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
		log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.", orig_name, filename);
		return (0);
	}

	switch (mode) {
		case CRASH_FILE:
			prefix = LIB_PLROBJS;
			suffix = SUF_OBJS;
			break;
		case ALIAS_FILE:
			prefix = LIB_PLRALIAS;
			suffix = SUF_ALIAS;
			break;
		case ETEXT_FILE:
			prefix = LIB_PLRTEXT;
			suffix = SUF_TEXT;
			break;
		case LORE_FILE:
			prefix = LIB_PLRLORE;
			suffix = SUF_LORE;
			break;
		case SCRIPT_VARS_FILE:
			prefix = LIB_PLRVARS;
			suffix = SUF_MEM;
			break;
		default:
			return (0);
	}

	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++)
		*ptr = LOWER(*ptr);

	if (LOWER(*name) <= 'e')
		middle = "A-E";
	else if (LOWER(*name) <= 'j')
		middle = "F-J";
	else if (LOWER(*name) <= 'o')
		middle = "K-O";
	else if (LOWER(*name) <= 't')
		middle = "P-T";
	else if (LOWER(*name) <= 'z')
		middle = "U-Z";
	else
		middle = "ZZZ";

	/* If your compiler gives you shit about <= '', use this switch:
		switch (LOWER(*name)) {
			case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
				middle = "A-E";
				break;
			case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
				middle = "F-J";
				break;
			case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
				middle = "K-O";
				break;
			case 'p':  case 'q':  case 'r':  case 's':  case 't':
				middle = "P-T";
				break;
			case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
				middle = "U-Z";
				break;
			default:
				middle = "ZZZ";
				break;
		}
	*/

	sprintf(filename, "%s%s/%s.%s", prefix, middle, name, suffix);
	return (1);
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE *fl, char *buf) {
	char temp[256];
	int lines = 0;

	do {
		fgets(temp, 256, fl);
		if (feof(fl)) {
			return (0);
		}
		lines++;
	} while (*temp == '*' || *temp == '\n');

	temp[strlen(temp) - 1] = '\0';
	strcpy(buf, temp);
	return (lines);
}


/* the "touch" command, essentially. */
int touch(const char *path) {
	FILE *fl;

	if (!(fl = fopen(path, "a"))) {
		log("SYSERR: %s: %s", path, strerror(errno));
		return (-1);
	}
	else {
		fclose(fl);
		return (0);
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// LOGGING UTILS ///////////////////////////////////////////////////////////


/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_log(const char *format, ...) {
	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	if (logfile == NULL) {
		puts("SYSERR: Using log() before stream was initialized!");
	}
	if (format == NULL) {
		format = "SYSERR: log() received a NULL format.";
	}

	time_s[strlen(time_s) - 1] = '\0';

	fprintf(logfile, "%-20.20s :: ", time_s + 4);

	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);

	fprintf(logfile, "\n");
	fflush(logfile);
}


/**
* Shows a log to players in-game and also stores it to file, except for the
* ELOG_NONE/ELOG_LOGINS types, which are displayed but not stored.
*
* @param empire_data *emp Which empire to log to.
* @param int type ELOG_x
* @param const char *str The va-arg format ...
*/
void log_to_empire(empire_data *emp, int type, const char *str, ...) {
	extern const bool show_empire_log_type[];
	
	struct empire_log_data *elog, *temp;
	char output[MAX_STRING_LENGTH];
	descriptor_data *i;
	va_list tArgList;

	if (!str || !*str) {
		return;
	}

	va_start(tArgList, str);
	vsprintf(output, str, tArgList);
	
	// save to file?
	if (type != ELOG_NONE && type != ELOG_LOGINS) {
		CREATE(elog, struct empire_log_data, 1);
		elog->type = type;
		elog->timestamp = time(0);
		elog->string = str_dup(output);
		elog->next = NULL;
		
		// append to end
		if ((temp = EMPIRE_LOGS(emp))) {
			while (temp->next) {
				temp = temp->next;
			}
			temp->next = elog;
		}
		else {
			EMPIRE_LOGS(emp) = elog;
		}
		
		save_empire(emp);
	}
	
	// show to players
	if (show_empire_log_type[type] == TRUE) {
		for (i = descriptor_list; i; i = i->next) {
			if (STATE(i) != CON_PLAYING || IS_NPC(i->character))
				continue;
			if (PLR_FLAGGED(i->character, PLR_WRITING))
				continue;
			if (GET_LOYALTY(i->character) != emp)
				continue;

			msg_to_char(i->character, "%s[ %s ]&0\r\n", EMPIRE_BANNER(emp), output);
		}
	}
	
	va_end(tArgList);
}


void mortlog(const char *str, ...) {
	char output[MAX_STRING_LENGTH];
	descriptor_data *i;
	va_list tArgList;

	if (!str) {
		return;
	}

	va_start(tArgList, str);
	vsprintf(output, str, tArgList);

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING && i->character && !PLR_FLAGGED(i->character, PLR_WRITING) && PRF_FLAGGED(i->character, PRF_MORTLOG)) {
			msg_to_char(i->character, "&c[ %s ]&0\r\n", output);
		}
	}
	va_end(tArgList);
}


/**
* This creates a consistent room identifier for logs.
*
* @param room_data *room The location to log.
* @return char* The compiled string.
*/
char *room_log_identifier(room_data *room) {
	extern char *get_room_name(room_data *room, bool color);
	
	static char val[MAX_STRING_LENGTH];
	
	sprintf(val, "[%d] %s (%d, %d)", GET_ROOM_VNUM(room), get_room_name(room, FALSE), X_COORD(room), Y_COORD(room));
	return val;
}


/**
* This is the main syslog function (mudlog in CircleMUD). It logs to all
* immortals who are listening.
*
* @param bitvector_t type Any SYS_x type
* @param int level The minimum level to see the log.
* @param bool file If TRUE, also outputs to the mud's log file.
* @param const char *str The log string.
* @param ... printf-style args for str.
*/
void syslog(bitvector_t type, int level, bool file, const char *str, ...) {
	char output[MAX_STRING_LENGTH];
	descriptor_data *i;
	va_list tArgList;

	if (!str) {
		return;
	}

	va_start(tArgList, str);
	vsprintf(output, str, tArgList);

	if (file) {
		log(output);
	}
	
	level = MAX(level, LVL_START_IMM);

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING && i->character && !IS_NPC(i->character) && !PLR_FLAGGED(i->character, PLR_WRITING) && GET_ACCESS_LEVEL(i->character) >= level) {
			if (IS_SET(SYSLOG_FLAGS(REAL_CHAR(i->character)), type)) {
				if (level > LVL_START_IMM) {
					msg_to_char(i->character, "&g[ (i%d) %s ]&0\r\n", level, output);
				}
				else {
					msg_to_char(i->character, "&g[ %s ]&0\r\n", output);
				}
			}
		}
	}
	
	va_end(tArgList);
}


 //////////////////////////////////////////////////////////////////////////////
//// ADVENTURE UTILS /////////////////////////////////////////////////////////

/**
* Looks for the adventure that contains this in its vnum range.
* 
* @param rmt_vnum vnum The vnum to look up.
* @return adv_data* An adventure, or NULL if no match.
*/
adv_data *get_adventure_for_vnum(rmt_vnum vnum) {
	adv_data *iter, *next_iter;
	
	HASH_ITER(hh, adventure_table, iter, next_iter) {
		if (vnum >= GET_ADV_START_VNUM(iter) && vnum <= GET_ADV_END_VNUM(iter)) {
			return iter;
		}
	}
	
	return NULL;
}


 //////////////////////////////////////////////////////////////////////////////
//// INTERPRETER UTILS ///////////////////////////////////////////////////////


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg) {
	skip_spaces(&argument);

	while (*argument && !isspace(*argument)) {
		*(first_arg++) = *argument;
		argument++;
	}

	*first_arg = '\0';

	return (argument);
}


/* same as any_one_arg except that it takes "quoted strings" and (coord, pairs) as one arg */
char *any_one_word(char *argument, char *first_arg) {
	skip_spaces(&argument);

	if (*argument == '\"') {
		++argument;
		do {
			*(first_arg++) = *argument;
			++argument;
		} while (*argument && *argument != '\"');
		if (*argument) {
			++argument;
		}
	}
	else if (*argument == '(') {
		++argument;
		do {
			*(first_arg++) = *argument;
			++argument;
		} while (*argument && *argument != ')');
		if (*argument) {
			++argument;
		}
	
	}
	else {
		// copy to first space
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = *argument;
			++argument;
		}
	}

	*first_arg = '\0';

	return (argument);
}


/**
* Function to separate a string into two comma-separated args. It is otherwise
* similar to half_chop().
*
* @param char *string The input string, e.g. "15, 2"
* @param char *arg1 A buffer to save the first argument to, or "" if there is none
* @param char *arg2 A buffer to save the second argument to, or ""
*/
void comma_args(char *string, char *arg1, char *arg2) {
	char *comma;
	int len;
	
	skip_spaces(&string);

	if ((comma = strchr(string, ',')) != NULL) {
		// trailing whitespace
		while (comma > string && isspace(*(comma-1))) {
			--comma;
		}
		len = comma - string;
		
		// arg1
		strncpy(arg1, string, len);
		arg1[len] = '\0';
		
		// arg2
		while (isspace(*comma) || *comma == ',') {
			++comma;
		}
		strcpy(arg2, comma);
	}
	else {
		// no comma
		strcpy(arg1, string);
		*arg2 = '\0';
	}
}


/**
* @param char *argument The argument to test.
* @return int TRUE if the argument is in the fill_words[] list.
*/
int fill_word(char *argument) {
	extern const char *fill_words[];
	return (search_block(argument, fill_words, TRUE) != NOTHING);
}


/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2) {
	char *temp;

	temp = any_one_arg(string, arg1);
	skip_spaces(&temp);
	strcpy(arg2, temp);
}


/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returnss 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2) {
	if (!*arg1)
		return (0);

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return (0);

	if (!*arg1)
		return (1);
	else
		return (0);
}


/**
* similar to is_abbrev() but compares abbreviations of multiple words, e.g.
* "cl br" would be a multi-abbrev of "clay bricks".
*
* @param const char *arg player input
* @param const char *phrase phrase to match
* @return bool TRUE if words in arg are an abbreviation of phrase, FALSE otherwise
*/
bool is_multiword_abbrev(const char *arg, const char *phrase) {
	char *argptr, *phraseptr;
	char argword[256], phraseword[256];
	char argcpy[MAX_INPUT_LENGTH], phrasecpy[MAX_INPUT_LENGTH];
	bool ok;

	if (is_abbrev(arg, phrase)) {
		return TRUE;
	}
	else {
		strcpy(phrasecpy, phrase);
		strcpy(argcpy, arg);
		
		argptr = any_one_arg(argcpy, argword);
		phraseptr = any_one_arg(phrasecpy, phraseword);
		
		ok = TRUE;
		while (*argword && *phraseword && ok) {
			if (!is_abbrev(argword, phraseword)) {
				ok = FALSE;
			}
			argptr = any_one_arg(argptr, argword);
			phraseptr = any_one_arg(phraseptr, phraseword);
		}
		
		// if anything is left in argword, there was another word that didn't match
		return (ok && !*argword);
	}
}
 

/**
* @param const char *str The string to check.
* @return int TRUE if str is a number, FALSE if it contains anything else.
*/
int is_number(const char *str) {
	// allow leading negative
	if (*str == '-') {
		str++;
	}
	
	while (*str)
		if (!isdigit(*(str++)))
			return (0);

	return (1);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg) {
	char *begin = first_arg;

	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (NULL);
	}

	do {
		skip_spaces(&argument);

		first_arg = begin;
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = *argument;
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") or parens
 * () are considered one word.
 */
char *one_word(char *argument, char *first_arg) {
	char *begin = first_arg;

	do {
		skip_spaces(&argument);

		first_arg = begin;

		if (*argument == '\"') {
			argument++;
			while (*argument && *argument != '\"') {
				*(first_arg++) = *argument;
				argument++;
			}
			argument++;
		}
		else if (*argument == '(') {
			argument++;
			while (*argument && *argument != ')') {
				*(first_arg++) = *argument;
				argument++;
			}
			argument++;
		}
		else {
			while (*argument && !isspace(*argument)) {
				*(first_arg++) = *argument;
				argument++;
			}
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return (argument);
}


/**
* @param char *argument The argument to test.
* @return int TRUE if the argument is in the reserved_words[] list.
*/
int reserved_word(char *argument) {
	extern const char *reserved_words[];
	return (search_block(argument, reserved_words, TRUE) != NOTHING);
}


/**
* searches an array of strings for a target string.  "exact" can be
* 0 or non-0, depending on whether or not the match must be exact for
* it to be returned.  Returns NOTHING if not found; 0..n otherwise.  Array
* must be terminated with a '\n' so it knows to stop searching.
*
* @param char *arg The input.
* @param const char **list A "\n"-terminated name list.
* @param int exact 0 = abbrevs, 1 = full match
*/
int search_block(char *arg, const char **list, int exact) {
	register int i, l;

	l = strlen(arg);

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!str_cmp(arg, *(list + i))) {
				return (i);
			}
		}
	}
	else {
		if (!l) {
			l = 1;			/* Avoid "" to match the first available string */
		}
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!strn_cmp(arg, *(list + i), l)) {
				return (i);
			}
		}
	}

	return (NOTHING);
}


/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string) {
	for (; **string && isspace(**string); (*string)++);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg) {
	return (one_argument(one_argument(argument, first_arg), second_arg));
}


 //////////////////////////////////////////////////////////////////////////////
//// MOBILE UTILS ////////////////////////////////////////////////////////////

/**
* Despawns all familiars and charmies a player has. This is usually called upon
* player death.
*
* @param char_data *ch The person whose followers to despawn.
*/
void despawn_charmies(char_data *ch) {
	char_data *iter, *next_iter;
	
	for (iter = character_list; iter; iter = next_iter) {
		next_iter = iter->next;
		
		if (IS_NPC(iter) && iter->master == ch) {
			if (MOB_FLAGGED(iter, MOB_FAMILIAR) || AFF_FLAGGED(iter, AFF_CHARM)) {
				act("$n leaves.", TRUE, iter, NULL, NULL, TO_ROOM);
				extract_char(iter);
			}
		}
	}
}


/**
* Quick way to turn a vnum into a name, safely.
*
* @param mob_vnum vnum The vnum to look up.
* @return char* A name for the vnum, or "UNKNOWN".
*/
char *get_mob_name_by_proto(mob_vnum vnum) {
	char_data *proto = mob_proto(vnum);
	char *unk = "UNKNOWN";
	
	return proto ? GET_SHORT_DESC(proto) : unk;
}


 //////////////////////////////////////////////////////////////////////////////
//// OBJECT UTILS ////////////////////////////////////////////////////////////

/**
* Quick way to turn a vnum into a name, safely.
*
* @param obj_vnum vnum The vnum to look up.
* @return char* A name for the vnum, or "UNKNOWN".
*/
char *get_obj_name_by_proto(obj_vnum vnum) {
	obj_data *proto = obj_proto(vnum);
	char *unk = "UNKNOWN";
	
	return proto ? GET_OBJ_SHORT_DESC(proto) : unk;
}


/**
* @param obj_data *obj Any object
* @return obj_data *the "top object" -- the outer-most container (or obj if it wasn't in one)
*/
obj_data *get_top_object(obj_data *obj) {
	obj_data *top = obj;
	
	while (top->in_obj) {
		top = top->in_obj;
	}
	
	return top;
}


/**
* Determines a rudimentary comparative value for an item
*
* @param obj_data *obj the item to compare
* @return double a score
*/
double rate_item(obj_data *obj) {
	extern double get_base_dps(obj_data *weapon);
	extern const double apply_values[];
	
	double score = 0;
	int iter;
	
	// basic apply score
	for (iter = 0; iter < MAX_OBJ_AFFECT; ++iter) {
		score += obj->affected[iter].modifier * apply_values[(int) obj->affected[iter].location];
	}
	
	// score based on type
	switch (GET_OBJ_TYPE(obj)) {
		case ITEM_WEAPON: {
			score += get_base_dps(obj);
			break;
		}
		case ITEM_DRINKCON: {
			score += GET_DRINK_CONTAINER_CAPACITY(obj) / config_get_double("scale_drink_capacity");
			break;
		}
		case ITEM_FOOD: {
			score += GET_FOOD_HOURS_OF_FULLNESS(obj) / config_get_double("scale_food_fullness");
			break;
		}
		case ITEM_COINS: {
			score += GET_COINS_AMOUNT(obj) / config_get_double("scale_coin_amount");
			break;
		}
		case ITEM_MISSILE_WEAPON: {
			score += get_base_dps(obj);
			break;
		}
		case ITEM_ARROW: {
			score += GET_ARROW_DAMAGE_BONUS(obj);
			break;
		}
		case ITEM_PACK: {
			score += GET_PACK_CAPACITY(obj) / config_get_double("scale_pack_size");
			break;
		}	
	}

	return score;
}


 //////////////////////////////////////////////////////////////////////////////
//// PLAYER UTILS ////////////////////////////////////////////////////////////

/**
* Gives a character the appropriate amount of command lag (wait time).
*
* @param char_data *ch The person to give command lag to.
* @param int wait_type A WAIT_x const to help determine wait time.
*/
void command_lag(char_data *ch, int wait_type) {
	extern const int universal_wait;
	
	double val;
	int wait;
	
	// short-circuit
	if (wait_type == WAIT_NONE) {
		return;
	}
	
	// base
	wait = universal_wait;
	
	switch (wait_type) {
		case WAIT_SPELL: {	// spells (but not combat spells)
			if (HAS_ABILITY(ch, ABIL_FASTCASTING)) {
				val = 0.3333 * GET_WITS(ch);
				wait -= MAX(0, val) RL_SEC;
				
				// ensure minimum of 0.5 seconds
				wait = MAX(wait, 0.5 RL_SEC);
			}
			break;
		}
		case WAIT_MOVEMENT: {	// normal movement (special handling)
			if (AFF_FLAGGED(ch, AFF_SLOW)) {
				wait = 1 RL_SEC;
			}
			else if (IS_RIDING(ch) || IS_ROAD(IN_ROOM(ch))) {
				wait = 0;	// no wait on riding/road
			}
			else if (IS_OUTDOORS(ch)) {
				if (HAS_BONUS_TRAIT(ch, BONUS_FASTER)) {
					wait = 0.25 RL_SEC;
				}
				else {
					wait = 0.5 RL_SEC;
				}
			}
			else {
				wait = 0;	// indoors
			}
			break;
		}
	}
	
	if (GET_WAIT_STATE(ch) < wait) {
		GET_WAIT_STATE(ch) = wait;
	}
}


/**
* Calculates a player's gear level, based on what they have equipped.
*
* @param char_data *ch The player to set gear level for.
*/
void determine_gear_level(char_data *ch) {
	extern const struct wear_data_type wear_data[NUM_WEARS];

	double total, slots;
	int avg, level, pos;
	
	// sanity check: we really have no work to do for mobs
	if (IS_NPC(ch)) {
		return;
	}
	
	level = total = slots = 0;	// init
	
	for (pos = 0; pos < NUM_WEARS; ++pos) {
		// some items count as more or less than 1 slot
		slots += wear_data[pos].gear_level_mod;
		
		if (GET_EQ(ch, pos) && wear_data[pos].gear_level_mod > 0) {
			total += GET_OBJ_CURRENT_SCALE_LEVEL(GET_EQ(ch, pos)) * wear_data[pos].gear_level_mod;
			
			// bonuses
			if (OBJ_FLAGGED(GET_EQ(ch, pos), OBJ_SUPERIOR)) {
				total += 10 * wear_data[pos].gear_level_mod;
			}
			if (OBJ_FLAGGED(GET_EQ(ch, pos), OBJ_HARD_DROP)) {
				total += 10 * wear_data[pos].gear_level_mod;
			}
			if (OBJ_FLAGGED(GET_EQ(ch, pos), OBJ_GROUP_DROP)) {
				total += 10 * wear_data[pos].gear_level_mod;
			}
		}
	}
	
	// determine average gear level of the player's slots
	avg = round(total / slots);
	
	// player's gear level (which will be added to skill level) is:
	level = avg + 50 - 100;	// 50 higher than the average scaled level of their gear, -100 to compensate for skill level
	
	GET_GEAR_LEVEL(ch) = MAX(level, 0);
}


/**
* Raises a person from sleeping+ to standing (or fighting) if possible.
* 
* @param char_data *ch The person to try to wake/stand.
* @return bool TRUE if the character ended up standing (>= fighting), FALSE if not.
*/
bool wake_and_stand(char_data *ch) {
	char buf[MAX_STRING_LENGTH];
	bool was_sleeping = FALSE;
	
	switch (GET_POS(ch)) {
		case POS_SLEEPING: {
			was_sleeping = TRUE;
			// no break -- drop through
		}
		case POS_RESTING:
		case POS_SITTING: {
			GET_POS(ch) = POS_STANDING;
			msg_to_char(ch, "You %sget up.\r\n", (was_sleeping ? "awaken and " : ""));
			snprintf(buf, sizeof(buf), "$n %sgets up.", (was_sleeping ? "awakens and " : ""));
			act(buf, TRUE, ch, NULL, NULL, TO_ROOM);
			// no break -- drop through
		}
		case POS_FIGHTING:
		case POS_STANDING: {
			// at this point definitely standing, or close enough
			return TRUE;
		}
		default: {
			// can't do anything with any other pos
			return FALSE;
		}
	}
	
	return FALSE;
}


 //////////////////////////////////////////////////////////////////////////////
//// RESOURCE UTILS //////////////////////////////////////////////////////////

/**
* Extract resources from the list, hopefully having checked has_resources, as
* this function does not error if it runs out -- it just keeps extracting
* until it's out of items, or hits its required limit.
*
* This function always takes from inventory first, and ground second.
*
* @param char_data *ch The person whose resources to take.
* @param Resource list[] Any resource list.
* @param bool ground If TRUE, will also take resources off the ground.
*/
void extract_resources(char_data *ch, Resource list[], bool ground) {
	obj_data *obj, *next_obj;
	int i, remaining;

	for (i = 0; list[i].vnum != NOTHING; i++) {
		remaining = list[i].amount;

		for (obj = ch->carrying; obj && remaining > 0; obj = next_obj) {
			next_obj = obj->next_content;

			if (GET_OBJ_VNUM(obj) == list[i].vnum) {
				--remaining;
				extract_obj(obj);
			}
		}
		if (ground) {
			for (obj = ROOM_CONTENTS(IN_ROOM(ch)); obj && remaining > 0; obj = next_obj) {
				next_obj = obj->next_content;

				if (GET_OBJ_VNUM(obj) == list[i].vnum) {
					--remaining;
					extract_obj(obj);
				}
			}
		}
	}
}


/**
* Give resources from a resource list to a player.
*
* @param char_data *ch The target player.
* @param Resource list[] Any resource list.
* @param bool split If TRUE, gives back only half.
*/
void give_resources(char_data *ch, Resource list[], bool split) {
	obj_data *obj;
	int i, j, remaining;

	for (i = 0; list[i].vnum != NOTHING; i++) {
		remaining = list[i].amount;

		if (split) {
			remaining /= 2;
		}

		for (j = 0; j < remaining; j++) {
			if (obj_proto(list[i].vnum)) {
				obj = read_object(list[i].vnum, TRUE);
				
				// scale item to minimum level
				scale_item_to_level(obj, 0);
				
				obj_to_char_or_room(obj, ch);
				load_otrigger(obj);
			}
		}
	}
}


/**
* Find out if a person has resources available.
*
* @param char_data *ch The person whose resources to check.
* @param Resource list[] Any resource list.
* @param bool ground If TRUE, will also count resources on the ground.
* @param bool send_msgs If TRUE, will alert the character as to what they need. FALSE runs silently.
*/
bool has_resources(char_data *ch, Resource list[], bool ground, bool send_msgs) {
	obj_data *obj, *proto;
	int i, total = 0;
	bool ok = TRUE;

	for (i = 0; list[i].vnum != NOTHING; i++, total = 0) {
		for (obj = ch->carrying; obj; obj = obj->next_content) {
			if (GET_OBJ_VNUM(obj) == list[i].vnum) {
				// check full of water for drink containers
				if (!IS_DRINK_CONTAINER(obj) || (GET_DRINK_CONTAINER_CONTENTS(obj) >= GET_DRINK_CONTAINER_CAPACITY(obj) && GET_DRINK_CONTAINER_TYPE(obj) == LIQ_WATER)) {
					++total;
				}
			}
		}
		if (ground) {
			for (obj = ROOM_CONTENTS(IN_ROOM(ch)); obj; obj = obj->next_content) {
				if (GET_OBJ_VNUM(obj) == list[i].vnum) {
					// check full of water for drink containers
					if (!IS_DRINK_CONTAINER(obj) || (GET_DRINK_CONTAINER_CONTENTS(obj) >= GET_DRINK_CONTAINER_CAPACITY(obj) && GET_DRINK_CONTAINER_TYPE(obj) == LIQ_WATER)) {
						++total;
					}
				}
			}
		}

		if (total < list[i].amount) {
			if (send_msgs && (proto = obj_proto(list[i].vnum))) {
				msg_to_char(ch, "%s %d more of %s%s", (ok ? "You need" : ","), list[i].amount - total, skip_filler(GET_OBJ_SHORT_DESC(proto)), IS_DRINK_CONTAINER(proto) ? " (full of water)" : "");
			}
			ok = FALSE;
		}
	}
	
	if (!ok && send_msgs) {
		send_to_char(".\r\n", ch);
	}

	return ok;
}


 //////////////////////////////////////////////////////////////////////////////
//// SECTOR UTILS ////////////////////////////////////////////////////////////

/**
* This function finds a sector that has (or doesn't have) certain flags, and
* returns the first matching sector.
*
* @param bitvector_t with_flags Find a sect that has these flags.
* @param bitvector_t without_flags Find a sect that doesn't have these flags.
* @return sector_data* A sector, or NULL if none matches.
*/
sector_data *find_first_matching_sector(bitvector_t with_flags, bitvector_t without_flags) {
	sector_data *sect, *next_sect;
	
	HASH_ITER(hh, sector_table, sect, next_sect) {
		if ((with_flags == NOBITS || SECT_FLAGGED(sect, with_flags)) && (without_flags == NOBITS || !SECT_FLAGGED(sect, without_flags))) {
			return sect;
		}
	}
	
	return NULL;
}


 //////////////////////////////////////////////////////////////////////////////
//// STRING UTILS ////////////////////////////////////////////////////////////

/**
* This converts data file entries into bitvectors, where they may be written
* as "abdo" in the file, or as a number.
*
* @param char *flag The input string.
* @return bitvector_t The bitvector.
*/
bitvector_t asciiflag_conv(char *flag) {
	bitvector_t flags = 0;
	int is_number = 1;
	register char *p;

	for (p = flag; *p; p++) {
		if (islower(*p))
			flags |= BIT(*p - 'a');
		else if (isupper(*p))
			flags |= BIT(26 + (*p - 'A'));

		if (!isdigit(*p))
			is_number = 0;
	}

	if (is_number)
		flags = strtoull(flag, NULL, 10); //atol(flag);

	return (flags);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string) {
	char *read, *write;

	/* If the string has no dollar signs, return immediately */
	if ((write = strchr(string, '$')) == NULL)
		return (string);

	/* Start from the location of the first dollar sign */
	read = write;

	while (*read)   /* Until we reach the end of the string... */
		if ((*(write++) = *(read++)) == '$') /* copy one char */
			if (*read == '$')
				read++; /* skip if we saw 2 $'s in a row */

	*write = '\0';

	return (string);
}


/**
* This converts bitvector flags into the human-readable sequence used in db
* files, e.g. "adoO", where each letter represents a bit starting with a=1.
* If there are no bits, it returns the string "0".
*
* @param bitvector_t flags The bitmask to convert to alpha.
* @return char* The resulting string.
*/
char *bitv_to_alpha(bitvector_t flags) {
	static char output[65];
	int iter, pos;
	
	pos = 0;
	for (iter = 0; flags && iter <= 64; ++iter) {
		if (IS_SET(flags, BIT(iter))) {
			output[pos++] = (iter < 26) ? ('a' + iter) : ('A' + iter - 26);
		}
		
		// remove so we exhaust flags
		REMOVE_BIT(flags, BIT(iter));
	}
	
	// empty string?
	if (pos == 0) {
		output[pos++] = '0';
	}
	
	// terminate it like Ahnold
	output[pos] = '\0';
	
	return output;
}


char *CAP(char *txt) {
	*txt = UPPER(*txt);
	return (txt);
}


/**
* @param char *string String to count color codes in
* @return int the number of &0-style color codes in the string
*/
int count_color_codes(char *string) {
	int iter, count = 0, len = strlen(string);
	for (iter = 0; iter < len - 1; ++iter) {
		if (string[iter] == '\t' && string[iter+1] == '\t') {
			++iter;	// advance past the \t\t (not a color code)
		}
		else if (string[iter] == '\t' && string[iter+1] == '&') {
			++iter;	// advance past the \t& (not a color code)
		}
		else if (string[iter] == '&' && string[iter+1] == '&') {
			++iter;	// advance past the && (not a color code)
		}
		else if (string[iter] == '&' || string[iter] == '\t') {
			++count;
			++iter;	// advance past the color code
		}
	}
	
	return count;
}


/**
* @param char *string The string to count && in.
* @return int The number of occurrances of && in the string.
*/
int count_double_ampersands(char *string) {
	int iter, count = 0, len = strlen(string);
	for (iter = 0; iter < len - 1; ++iter) {
		if (string[iter] == '&' && string[iter+1] == '&') {
			++count;
			++iter;	// advance past the second &
		}
		else if (string[iter] == '\t' && string[iter+1] == '&') {
			++count;
			++iter;	// advance past the & in \t&
		}
		else if (string[iter] == '\t' && string[iter+1] == '\t') {
			++count;
			++iter;	// advance past the second \t in \t\t (similar to an &&)
		}
	}
	
	return count;
}


/**
* Counts the number of icon codes (@) in a string.
*
* @param char *string The input string.
* @return int the number of @ codes.
*/
int count_icon_codes(char *string) {
	int iter, count = 0, len = strlen(string);
	for (iter = 0; iter < len - 1; ++iter) {
		if (string[iter] == '@' && string[iter+1] != '@') {
			++count;
			++iter;	// advance past the color code
		}
	}
	
	return count;
}


/**
* @param const char *string A string that might contain stray % signs.
* @return const char* A string with % signs doubled.
*/
const char *double_percents(const char *string) {
	static char output[MAX_STRING_LENGTH*2];
	int iter, pos;
	
	// no work
	if (!strchr(string, '%')) {
		return string;
	}
	
	for (iter = 0, pos = 0; string[iter] && pos < (MAX_STRING_LENGTH*2); ++iter) {
		output[pos++] = string[iter];
		
		// double %
		if (string[iter] == '%' && pos < (MAX_STRING_LENGTH*2)) {
			output[pos++] = '%';
		}
	}
	
	// terminate/ensure terminator
	output[MIN(pos++, (MAX_STRING_LENGTH*2)-1)] = '\0';
	return (const char*)output;
}


/**
* @param const char *namelist A keyword list, e.g. "bear white huge"
* @return char* The first name in the list, e.g. "bear"
*/
char *fname(const char *namelist) {
	static char holder[30];
	register char *point;

	for (point = holder; isalpha(*namelist); namelist++, point++) {
		*point = *namelist;
	}

	*point = '\0';

	return (holder);
}


/**
* Determines if str is an abbrev for any keyword in namelist.
*
* @param const char *str The search argument, e.g. "be"
* @param const char *namelist The keyword list, e.g. "bear white huge"
* @return bool TRUE if str is an abbrev for any keyword in namelist
*/
bool isname(const char *str, const char *namelist) {
	char *newlist, *curtok;
	bool found = FALSE;

	/* the easy way */
	if (!str_cmp(str, namelist)) {
		return TRUE;
	}

	newlist = strdup(namelist);

	for (curtok = strtok(newlist, WHITESPACE); curtok && !found; curtok = strtok(NULL, WHITESPACE)) {
		if (curtok && is_abbrev(str, curtok)) {
			found = TRUE;
		}
	}

	free(newlist);
	return found;
}


/**
* Makes a level range look more elegant. You can omit current.
*
* @param int min The low end of the range (0 for none).
* @param int max The high end of the range (0 for no max).
* @param int current If it's already scaled, one level (0 for none).
* @return char* The string.
*/
char *level_range_string(int min, int max, int current) {
	static char output[65];
	
	if (current > 0) {
		snprintf(output, sizeof(output), "%d", current);
	}
	else if (min > 0 && max > 0) {
		snprintf(output, sizeof(output), "%d-%d", min, max);
	}
	else if (min > 0) {
		snprintf(output, sizeof(output), "%d+", min);
	}
	else if (max > 0) {
		snprintf(output, sizeof(output), "1-%d", max);
	}
	else {
		snprintf(output, sizeof(output), "0");
	}
	
	return output;
}


/**
* This works like isname but checks to see if EVERY word in arg is an abbrev
* of some word in namelist. E.g. "cl br" is an abbrev of "bricks pile clay",
* the namelist from "a pile of clay bricks".
*
* @param const char *arg The user input
* @param const char *namelist The list of names to check
* @return bool TRUE if every word in arg is a valid name from namelist, otherwise FALSE
*/
bool multi_isname(const char *arg, const char *namelist) {
	char argcpy[MAX_INPUT_LENGTH], argword[256];
	char *ptr;
	bool ok;

	/* the easy way */
	if (!str_cmp(arg, namelist)) {
		return TRUE;
	}
	
	strcpy(argcpy, arg);
	ptr = any_one_arg(argcpy, argword);
	
	ok = TRUE;
	while (*argword && ok) {
		if (!isname(argword, namelist)) {
			ok = FALSE;
		}
		
		ptr = any_one_arg(ptr, argword);
	}
	
	return ok;
}


/**
* This does the same thing as sprintbit but looks nicer for players.
*/
void prettier_sprintbit(bitvector_t bitvector, const char *names[], char *result) {
	long nr;
	bool found = FALSE;

	*result = '\0';

	for (nr = 0; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			if (*names[nr] != '\n') {
				sprintf(result + strlen(result), "%s%s", (found ? ", " : ""), names[nr]);
			}
			else {
				sprintf(result + strlen(result), "%sUNDEFINED", (found ? ", " : ""));
			}
			
			found = TRUE;
		}
		if (*names[nr] != '\n') {
			nr++;
		}
	}

	if (!*result) {
		strcpy(result, "none");
	}
}


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt) {
	int i = strlen(txt) - 1;

	while (txt[i] == '\n' || txt[i] == '\r') {
		txt[i--] = '\0';
	}
}


/**
* Replaces a &? with the replacement color of your choice.
*
* @param char *input The incoming string containing 0 or more '&?'.
* @param char *color The string to replace '&?' with.
* @param char *output A buffer to store the result to.
*/
void replace_question_color(char *input, char *color, char *output) {
	int ipos, opos;
	*output = '\0';
	
	for (ipos = 0, opos = 0; ipos < strlen(input); ++ipos) {
		if (input[ipos] == '&' && input[ipos+1] == '?') {
			// copy replacement color instead
			strcpy(output + opos, color);
			opos += strlen(color);
			
			// skip past &?
			++ipos;
		}
		else {
			output[opos++] = input[ipos];
		}
	}
	
	output[opos++] = '\0';
}


/**
* Finds the last occurrence of needle in haystack.
*
* @param const char *haystack The string to search.
* @param const char *needle The thing to search for.
* @return char* The last occurrence of needle in haystack, or NULL if it does not occur.
*/
char *reverse_strstr(char *haystack, char *needle) {
	char *found = NULL, *next = haystack;
	
	if (!haystack || !needle || !*haystack) {
		return NULL;
	}
	
	while ((next = strstr(next, needle))) {
		found = next;
		++next;
	}
	
	return found;
}


/**
* Doubles the & in a string so that color codes are displayed to the user.
*
* @param char *string The input string.
* @return char* The string with color codes shown.
*/
char *show_color_codes(char *string) {
	static char value[MAX_STRING_LENGTH];
	char *ptr;
	
	ptr = str_replace("&", "&&", string);
	strncpy(value, ptr, MAX_STRING_LENGTH);
	value[MAX_STRING_LENGTH-1] = '\0';	// safety
	free(ptr);
	
	return value;
}


/**
* This function gets just the portion of a string after any fill words or
* reserved words -- used especially to simplify "an object" to "object".
*
* If you want to save the result somewhere, you should str_dup() it
*
* @param char *string The original string.
*/
const char *skip_filler(char *string) {	
	static char remainder[MAX_STRING_LENGTH];
	char temp[MAX_STRING_LENGTH];
	char *ptr;
	int pos = 0;
	
	*remainder = '\0';
	
	if (!string) {
		log("SYSERR: skip_filler received a NULL pointer.");
		return (const char *)remainder;
	}
	
	do {
		string += pos;
		skip_spaces(&string);
		
		if ((ptr = strchr(string, ' '))) {
			pos = ptr - string;
		}
		else {
			pos = 0;
		}
		
		if (pos > 0) {
			strncpy(temp, string, pos);
		}
		temp[pos] = '\0';
	} while (fill_word(temp) || reserved_word(temp));
	
	// when we get here, string is a pointer to the start of the real name
	strcpy(remainder, string);
	return (const char *)remainder;
}


/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
void sprintbit(bitvector_t bitvector, const char *names[], char *result, byte space) {
	long nr;

	*result = '\0';

	for (nr = 0; bitvector; bitvector >>= 1) {
		if (IS_SET(bitvector, 1)) {
			if (*names[nr] != '\n') {
				strcat(result, names[nr]);
				if (space && *names[nr]) {
					strcat(result, " ");
				}
			}
			else {
				strcat(result, "UNDEFINED ");
			}
		}
		if (*names[nr] != '\n') {
			nr++;
		}
	}

	if (!*result && space) {
		strcpy(result, "NOBITS ");
	}
}


void sprinttype(int type, const char *names[], char *result) {
	int nr = 0;

	while (type && *names[nr] != '\n') {
		type--;
		nr++;
	}

	if (*names[nr] != '\n') {
		strcpy(result, names[nr]);
	}
	else {
		strcpy(result, "UNDEFINED");
	}
}


/**
* Similar to strchr except the 2nd argument is a string.
*
* @param const char *haystack The string to search.
* @param const char *needles A set of characters to search for.
* @return bool TRUE if any character from needles exists in haystack.
*/
bool strchrstr(const char *haystack, const char *needles) {
	int iter;
	
	for (iter = 0; needles[iter] != '\0'; ++iter) {
		if (strchr(haystack, needles[iter])) {
			return TRUE;
		}
	}
	
	return FALSE;
}


/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2) {
	int chk, i;

	if (arg1 == NULL || arg2 == NULL) {
		log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; arg1[i] || arg2[i]; i++) {
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0) {
			return (chk);	/* not equal */
		}
	}

	return (0);
}


/* Create a duplicate of a string */
char *str_dup(const char *source) {
	char *new_z;

	CREATE(new_z, char, strlen(source) + 1);
	return (strcpy(new_z, source));
}


/**
* Generic string replacement function: returns a memory-allocated char* with
* the resulting string.
*
* @param char *search The search string ('foo').
* @param char *replace The replacement string ('bar').
* @param char *subject The string to alter ('foodefoo').
* @return char* A pointer to a new string with the replacements ('bardebar').
*/
char *str_replace(char *search, char *replace, char *subject) {
	static char output[MAX_STRING_LENGTH];
	int spos, opos, slen, rlen;
	
	// nothing / don't bother
	if (!*subject || !*search || !strstr(subject, search)) {
		return str_dup(subject);
	}
	
	slen = strlen(search);
	rlen = strlen(replace);
	
	for (spos = 0, opos = 0; spos < strlen(subject) && opos < MAX_STRING_LENGTH; ) {
		if (!strncmp(subject + spos, search, slen)) {
			// found a search match at this position
			strncpy(output + opos, replace, MAX_STRING_LENGTH - opos);
			opos += rlen;
			spos += slen;
		}
		else {
			output[opos++] = subject[spos++];
		}
	}
	
	// safety terminations:
	if (opos < MAX_STRING_LENGTH) {
		output[opos++] = '\0';
	}
	else {
		output[MAX_STRING_LENGTH-1] = '\0';
	}
	
	// allocate enough memory to return
	return str_dup(output);
}


/**
* Strips out the color codes and returns the string.
*
* @param char *input
* @return char *result (no memory allocated, so str_dup if you want to keep it)
*/
char *strip_color(char *input) {
	static char lbuf[MAX_STRING_LENGTH];
	char *ptr;
	int iter, pos;

	for (iter = 0, pos = 0; pos < (MAX_STRING_LENGTH-1) && iter < strlen(input); ++iter) {
		if (input[iter] == '&' && input[iter+1] != '&') {
			++iter;
		}
		else if (input[iter] == '\t' && input[iter+1] != '\t') {
			++iter;
		}
		else {
			lbuf[pos++] = input[iter];
		}
	}
	
	lbuf[pos++] = '\0';
	ptr = lbuf;
	
	return ptr;
}


/* Removes the \r from \r\n to prevent Windows clients from seeing extra linebreaks in descs */
void strip_crlf(char *buffer) {
	register char *ptr, *str;

	ptr = buffer;
	str = ptr;

	while ((*str = *ptr)) {
		str++;
		ptr++;
		if (*ptr == '\r') {
			ptr++;
		}
	}
}


/* strips \r's from line */
char *stripcr(char *dest, const char *src) {
	int i, length;
	char *temp;

	if (!dest || !src) return NULL;
	temp = &dest[0];
	length = strlen(src);
	for (i = 0; *src && (i < length); i++, src++)
		if (*src != '\r') *(temp++) = *src;
	*temp = '\0';
	return dest;
}


/**
* Converts a string to all-lowercase. The orignal string is modified by this,
* and a reference to it is returned.
*
* @param char *str The string to lower.
* @return char* A pointer to the string.
*/
char *strtolower(char *str) {
	char *ptr;
	
	for (ptr = str; *ptr; ++ptr) {
		*ptr = LOWER(*ptr);
	}
	
	return str;
}


/**
* Converts a string to all-uppercase. The orignal string is modified by this,
* and a reference to it is returned.
*
* @param char *str The string to uppercase.
* @return char* A pointer to the string.
*/
char *strtoupper(char *str) {
	char *ptr;
	
	for (ptr = str; *ptr; ++ptr) {
		*ptr = UPPER(*ptr);
	}
	
	return str;
}


/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n) {
	int chk, i;

	if (arg1 == NULL || arg2 == NULL) {
		log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}

	for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--) {
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0) {
			return (chk);	/* not equal */
		}
	}

	return (0);
}


/*
* Version of str_str from dg_scripts -- better than the base CircleMUD version.
*
* @param char *cs The string to search.
* @param char *ct The string to search for...
* @return char* A pointer to the substring within cs, or NULL if not found.
*/
char *str_str(char *cs, char *ct) {
	char *s, *t;

	if (!cs || !ct || !*ct)
		return NULL;

	while (*cs) {
		t = ct;

		while (*cs && (LOWER(*cs) != LOWER(*t)))
			cs++;

		s = cs;

		while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
			t++;
			cs++;
		}

		if (!*t)
			return s;
	}

	return NULL;
}


 //////////////////////////////////////////////////////////////////////////////
//// TYPE UTILS //////////////////////////////////////////////////////////////

/**
* @param char *name Building to find by name.
* @param bool room_only If TRUE, only matches designatable rooms.
* @return bld_data *The matching building entry (BUILDING_x const) or NULL.
*/
bld_data *get_building_by_name(char *name, bool room_only) {
	bld_data *iter, *next_iter;
	
	HASH_ITER(hh, building_table, iter, next_iter) {
		if ((!room_only || IS_SET(GET_BLD_FLAGS(iter), BLD_ROOM)) && is_abbrev(name, GET_BLD_NAME(iter))) {
			return iter;
		}
	}
	
	return NULL;
}


/**
* Find a crop by name; prefers exact match over abbrev.
*
* @param char *name Crop to find by name.
* @return crop_data* The matching crop prototype or NULL.
*/
crop_data *get_crop_by_name(char *name) {
	crop_data *iter, *next_iter, *abbrev_match = NULL;
	
	HASH_ITER(hh, crop_table, iter, next_iter) {
		if (!str_cmp(name, GET_CROP_NAME(iter))) {
			return iter;
		}
		if (!abbrev_match && is_abbrev(name, GET_CROP_NAME(iter))) {
			abbrev_match = iter;
		}
	}
	
	return abbrev_match;	// if any
}


/**
* Find a sector by name; prefers exact match over abbrev.
*
* @param char *name Sector to find by name.
* @return sector_data* The matching sector or NULL.
*/
sector_data *get_sect_by_name(char *name) {
	sector_data *sect, *next_sect, *abbrev_match = NULL;

	HASH_ITER(hh, sector_table, sect, next_sect) {
		if (!str_cmp(name, GET_SECT_NAME(sect))) {
			// exact match
			return sect;
		}
		if (!abbrev_match && is_abbrev(name, GET_SECT_NAME(sect))) {
			abbrev_match = sect;
		}
	}
	
	return abbrev_match;	// if any
}


 //////////////////////////////////////////////////////////////////////////////
//// WORLD UTILS /////////////////////////////////////////////////////////////


/**
* @param room_data *room Room to check.
* @return bool TRUE if it's sunny there.
*/
bool check_sunny(room_data *room) {
	bool sun_sect = !ROOM_IS_CLOSED(room);
	return (sun_sect && weather_info.sunlight != SUN_DARK && !ROOM_AFF_FLAGGED(room, ROOM_AFF_DARK));
}


/**
* Gets linear distance between two rooms.
*
* @param room_data *from Origin room
* @param room_data *to Target room
* @return int distance
*/
int compute_distance(room_data *from, room_data *to) {
	int x1 = X_COORD(from), y1 = Y_COORD(from);
	int x2 = X_COORD(to), y2 = Y_COORD(to);
	int dx = x1 - x2;
	int dy = y1 - y2;
	int dist;
	
	// short circuit on same-room
	if (from == to || HOME_ROOM(from) == HOME_ROOM(to)) {
		return 0;
	}
	
	// infinite distance if they are in an unknown location
	if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0) {
		return MAP_SIZE;	// maximum distance
	}

	// adjust for edges
	if (WRAP_X) {
		if (dx < (-1 * MAP_WIDTH / 2)) {
			dx += MAP_WIDTH;
		}
		if (dx > (MAP_WIDTH / 2)) {
			dx -= MAP_WIDTH;
		}
	}
	if (WRAP_Y) {
		if (dy < (-1 * MAP_HEIGHT / 2)) {
			dy += MAP_HEIGHT;
		}
		if (dy > (MAP_HEIGHT / 2)) {
			dy -= MAP_HEIGHT;
		}
	}
	
	dist = (dx * dx + dy * dy);
	dist = (int) sqrt(dist);
	
	return dist;
}


/**
* Counts how many adjacent tiles have the given sector type...
*
* @param room_data *room The location to check.
* @param sector_vnum The sector vnum to find.
* @param bool count_original_sect If TRUE, also checks ROOM_ORIGINAL_SECT
* @return int The number of matching adjacent tiles.
*/
int count_adjacent_sectors(room_data *room, sector_vnum sect, bool count_original_sect) {
	sector_data *rl_sect = sector_proto(sect);
	int iter, count = 0;
	room_data *to_room;
	
	// we only care about its map room
	room = HOME_ROOM(room);
	
	for (iter = 0; iter < NUM_2D_DIRS; ++iter) {
		to_room = real_shift(room, shift_dir[iter][0], shift_dir[iter][1]);
		
		if (to_room && (SECT(to_room) == rl_sect || (count_original_sect && ROOM_ORIGINAL_SECT(to_room) == rl_sect))) {
			++count;
		}
	}
	
	return count;
}


/**
* Counts how many times a sector appears between two locations, NOT counting
* the start/end locations.
*
* @param bitvector_t sectf_bits SECTF_x flag(s) to look for.
* @param room_data *start Tile to start at (not inclusive).
* @param room_data *end Tile to end at (not inclusive).
* @param bool check_base_sect If TRUE, also looks for those flags on the base sector.
* @return int The number of matching sectors.
*/
int count_flagged_sect_between(bitvector_t sectf_bits, room_data *start, room_data *end, bool check_base_sect) {
	room_data *room;
	int dist, iter, count = 0;
	
	// no work
	if (!start || !end || start == end || !sectf_bits) {
		return count;
	}
	
	dist = compute_distance(start, end);	// for safety-checking
	
	for (iter = 1, room = straight_line(start, end, iter); iter <= dist && room && room != end; ++iter, room = straight_line(start, end, iter)) {
		if (SECT_FLAGGED(SECT(room), sectf_bits) || (check_base_sect && SECT_FLAGGED(ROOM_ORIGINAL_SECT(room), sectf_bits))) {
			++count;
		}
	}
	
	return count;
}


/**
* Computes the distance to the nearest connected player.
*
* @param room_data *room Origin spot
* @return int Distance to the nearest player (> MAP_SIZE if no players)
*/
int distance_to_nearest_player(room_data *room) {
	descriptor_data *d_iter;
	char_data *vict;
	int dist, best = MAP_SIZE * 2;
	
	for (d_iter = descriptor_list; d_iter; d_iter = d_iter->next) {
		if ((vict = d_iter->character) && IN_ROOM(vict)) {
			dist = compute_distance(room, IN_ROOM(vict));
			if (dist < best) {
				best = dist;
			}
		}
	}
	
	return best;
}


/**
* This finds the ultimate map point for a given room, resolving any number of
* layers of boats and home rooms.
*
* @param room_data *room Any room in the game.
* @return room_data* A location on the map, or NULL if there is no map location.
*/
room_data *get_map_location_for(room_data *room) {
	room_data *working = room, *last;
	obj_data *boat;
	
	if (!room) {
		return NULL;
	}
	else if (GET_ROOM_VNUM(room) < MAP_SIZE) {
		// shortcut
		return room;
	}
	else if (GET_ROOM_VNUM(HOME_ROOM(room)) >= MAP_SIZE && BOAT_ROOM(room) == room && !IS_ADVENTURE_ROOM(HOME_ROOM(room))) {
		// no home room on the map and not in a boat?
		return NULL;
	}
	else if (GET_BOAT(room) && GET_ROOM_VNUM(BOAT_ROOM(room)) >= MAP_SIZE) {
		// in a boat but it's not on the map?
		return NULL;
	}
	
	do {
		last = working;
		
		// instance resolution: find instance home
		do {
			last = working;
			if (COMPLEX_DATA(working) && COMPLEX_DATA(working)->instance && COMPLEX_DATA(working)->instance->location) {
				working = COMPLEX_DATA(working)->instance->location;
			}
		} while (last != working);
		
		// boat resolution: find top boat->in_room: this is similar to GET_BOAT()/BOAT_ROOM()
		do {
			last = working;
			boat = (COMPLEX_DATA(working) ? COMPLEX_DATA(working)->boat : NULL);
		
			if (boat && IN_ROOM(boat)) {
				working = IN_ROOM(boat);
			}
		} while (last != working);
		
		// home resolution: similar to HOME_ROOM()
		if (COMPLEX_DATA(working) && COMPLEX_DATA(working)->home_room) {
			working = COMPLEX_DATA(working)->home_room;
		}
	} while (COMPLEX_DATA(working) && GET_ROOM_VNUM(working) >= MAP_SIZE && last != working);
	
	return working;
}


/**
* Find an optimal place to start upon new login or death.
*
* @param char_data *ch The player to find a loadroom for.
* @return room_data* The location of a place to start.
*/
room_data *find_load_room(char_data *ch) {
	extern room_data *find_starting_location();
	
	struct empire_territory_data *ter;
	room_data *rl, *rl_last_room, *found = NULL;
	int num_found = 0;
	sh_int island;
	
	// preferred graveyard?
	if (!IS_NPC(ch) && (rl = real_room(GET_TOMB_ROOM(ch)))) {
		// does not require last room but if there is one, it must be the same island
		rl_last_room = real_room(GET_LAST_ROOM(ch));
		if (ROOM_BLD_FLAGGED(rl, BLD_TOMB) && (!rl_last_room || GET_ISLAND_ID(rl) == GET_ISLAND_ID(rl_last_room)) && can_use_room(ch, rl, GUESTS_ALLOWED)) {
			return rl;
		}
	}
	
	// first: look for graveyard
	if (!IS_NPC(ch) && (rl = real_room(GET_LAST_ROOM(ch))) && GET_LOYALTY(ch)) {
		island = GET_ISLAND_ID(rl);
		for (ter = EMPIRE_TERRITORY_LIST(GET_LOYALTY(ch)); ter; ter = ter->next) {
			if (ROOM_BLD_FLAGGED(ter->room, BLD_TOMB) && IS_COMPLETE(ter->room) && GET_ISLAND_ID(ter->room) == island) {
				// pick at random if more than 1
				if (!number(0, num_found++) || !found) {
					found = ter->room;
				}
			}
		}
		
		if (found) {
			return found;
		}
	}
	
	// still here?
	return find_starting_location();
}


/**
* @param int type MINE_x
* @return int position in mine_data[] or NOTHING
*/
int find_mine_type(int type) {
	int iter, found = NOTHING;
	
	for (iter = 0; mine_data[iter].type != NOTHING && found == NOTHING; ++iter) {
		if (mine_data[iter].type == type) {
			found = iter;
		}
	}
	
	return found;
}


/**
* @param int type a mine type
* @return obj_vnum mine production vnum
*/
obj_vnum find_mine_vnum_by_type(int type) {
	int t = find_mine_type(type);
	obj_vnum vnum = (t != NOTHING ? mine_data[t].vnum : o_IRON_ORE);
	return vnum;
}


/**
* This determines if ch is close enough to a sect with certain flags.
*
* @param char_data *ch
* @param bitvector_t with_flags The bits that the sect MUST have (returns true if any 1 is set).
* @param bitvector_t without_flags The bits that must NOT be set (returns true if NONE are set).
* @param int distance how far away to check
* @return bool TRUE if the sect is found
*/
bool find_flagged_sect_within_distance_from_char(char_data *ch, bitvector_t with_flags, bitvector_t without_flags, int distance) {
	bool found = FALSE;
	
	if (!IS_ADVENTURE_ROOM(IN_ROOM(ch)) && IS_OUTDOORS(ch)) {
		found = find_flagged_sect_within_distance_from_room(IN_ROOM(ch), with_flags, without_flags, distance);
	}
	
	return found;
}


/**
* This determines if room is close enough to a sect with certain flags.
*
* @param room_data *room The location to check.
* @param bitvector_t with_flags The bits that the sect MUST have (returns true if any 1 is set).
* @param bitvector_t without_flags The bits that must NOT be set (returns true if NONE are set).
* @param int distance how far away to check
* @return bool TRUE if the sect is found
*/
bool find_flagged_sect_within_distance_from_room(room_data *room, bitvector_t with_flags, bitvector_t without_flags, int distance) {
	int x, y;
	room_data *shift, *real = get_map_location_for(room);
	bool found = FALSE;
	
	if (!real) {	// no map location
		return FALSE;
	}
	
	for (x = -1 * distance; x <= distance && !found; ++x) {
		for (y = -1 * distance; y <= distance && !found; ++y) {
			shift = real_shift(real, x, y);
			if (shift && (with_flags == NOBITS || ROOM_SECT_FLAGGED(shift, with_flags)) && (without_flags == NOBITS || !ROOM_SECT_FLAGGED(shift, without_flags))) {
				if (compute_distance(room, shift) <= distance) {
					found = TRUE;
				}
			}
		}
	}
	
	return found;
}


/**
* This determines if ch is close enough to a given sect.
*
* @param char_data *ch
* @param sector_vnum sect Sector vnum
* @param int distance how far away to check
* @return bool TRUE if the sect is found
*/
bool find_sect_within_distance_from_char(char_data *ch, sector_vnum sect, int distance) {
	bool found = FALSE;
	
	if (!IS_ADVENTURE_ROOM(IN_ROOM(ch)) && IS_OUTDOORS(ch)) {
		found = find_sect_within_distance_from_room(IN_ROOM(ch), sect, distance);
	}
	
	return found;
}


/**
* This determines if room is close enough to a given sect.
*
* @param room_data *room
* @param sector_vnum sect Sector vnum
* @param int distance how far away to check
* @return bool TRUE if the sect is found
*/
bool find_sect_within_distance_from_room(room_data *room, sector_vnum sect, int distance) {
	room_data *real = get_map_location_for(room);
	sector_data *find = sector_proto(sect);
	bool found = FALSE;
	room_data *shift;
	int x, y;
	
	if (!real) {	// no map location
		return FALSE;
	}
	
	for (x = -1 * distance; x <= distance && !found; ++x) {
		for (y = -1 * distance; y <= distance && !found; ++y) {
			shift = real_shift(real, x, y);
			if (shift && SECT(shift) == find && compute_distance(real, shift) <= distance) {
				found = TRUE;
			}
		}
	}
	
	return found;
}


/**
* find a random starting location
*
* @return room_data* A random starting location.
*/
room_data *find_starting_location() {
	extern int num_of_start_locs;
	extern int *start_locs;
	
	if (num_of_start_locs < 0) {
		return NULL;
	}

	return (real_room(start_locs[number(0, num_of_start_locs)]));
}


/**
* This function determines the approximate direction between two points on the
* map.
*
* @param room_data *from The origin point.
* @param room_data *to The desitination point.
* @return int Any direction const (NORTHEAST), or NO_DIR for no direction.
*/
int get_direction_to(room_data *from, room_data *to) {
	room_data *origin = HOME_ROOM(from), *dest = HOME_ROOM(to);
	int from_x = X_COORD(origin), from_y = Y_COORD(origin);
	int to_x = X_COORD(dest), to_y = Y_COORD(dest);
	int x_diff = to_x - from_x, y_diff = to_y - from_y;
	int dir = NO_DIR;
	
	// adjust for edges
	if (WRAP_X) {
		if (x_diff < (-1 * MAP_WIDTH / 2)) {
			x_diff += MAP_WIDTH;
		}
		if (x_diff > (MAP_WIDTH / 2)) {
			x_diff -= MAP_WIDTH;
		}
	}
	if (WRAP_Y) {
		if (y_diff < (-1 * MAP_HEIGHT / 2)) {
			y_diff += MAP_HEIGHT;
		}
		if (y_diff > (MAP_HEIGHT / 2)) {
			y_diff -= MAP_HEIGHT;
		}
	}

	// tolerance: 1 e/w per 5 north would still count as "north"
	
	if (x_diff == 0 && y_diff == 0) {
		dir = NO_DIR;
	}
	else if (ABSOLUTE(y_diff/5) >= ABSOLUTE(x_diff)) {
		dir = (y_diff > 0) ? NORTH : SOUTH;
	}
	else if (ABSOLUTE(x_diff/5) >= ABSOLUTE(y_diff)) {
		dir = (x_diff > 0) ? EAST : WEST;
	}
	else if (x_diff > 0 && y_diff > 0) {
		dir = NORTHEAST;
	}
	else if (x_diff > 0 && 0 > y_diff) {
		dir = SOUTHEAST;
	}
	else if (0 > x_diff && y_diff > 0) {
		dir = NORTHWEST;
	}
	else if (0 > x_diff && 0 > y_diff) {
		dir = SOUTHWEST;
	}
	
	return dir;
}


/**
* Fetch the island id based on the map location of the room. This was a macro,
* but get_map_location_for() can return a NULL.
*
* @param room_data *room The room to check.
* @return int The island ID, or NO_ISLAND if none.
*/
int GET_ISLAND_ID(room_data *room) {
	room_data *map = get_map_location_for(room);
	
	if (map) {
		return map->island;
	}
	else {
		return NO_ISLAND;
	}
}


/**
* @param room_data *room A room that has existing mine data
* @return TRUE if the room has a deep mine set up
*/
bool is_deep_mine(room_data *room) {
	return (get_room_extra_data(room, ROOM_EXTRA_MINE_AMOUNT) > mine_data[find_mine_type(get_room_extra_data(room, ROOM_EXTRA_MINE_TYPE))].max_amount);
}


/**
* Locks in a random tile icon by assigning it as a custom icon. This only works
* if the room doesn't already have a custom icon.
*
* @param room_data *room The room to set this on.
* @param struct icon_data *use_icon Optional: Force it to use this icon (may be NULL).
*/
void lock_icon(room_data *room, struct icon_data *use_icon) {
	extern struct icon_data *get_icon_from_set(struct icon_data *set, int type);
	extern int pick_season(room_data *room);
	
	struct icon_data *icon;
	int season;

	// don't do it if a custom icon is set
	if (ROOM_CUSTOM_ICON(room)) {
		return;
	}

	if (!(icon = use_icon)) {	
		season = pick_season(room);
		icon = get_icon_from_set(GET_SECT_ICONS(SECT(room)), season);
	}
	ROOM_CUSTOM_ICON(room) = str_dup(icon->icon);
}


/**
* The main function for finding one map location starting from another location
* that is either on the map, or can be resolved to the map (e.g. a home room).
* This function returns NULL if there is no valid shift.
*
* @param room_data *origin The start location
* @param int x_shift How far to move east/west
* @param int y_shift How far to move north/south
* @return room_data* The new location on the map, or NULL if the location would be off the map
*/
room_data *real_shift(room_data *origin, int x_shift, int y_shift) {
	room_data *map;
	int x_coord, y_coord;
	room_vnum loc;
	
	// sanity?
	if (!origin) {
		return NULL;
	}
	
	map = get_map_location_for(origin);
	
	// are we somehow not on the map? if not, don't shift
	if (!map || (loc = GET_ROOM_VNUM(map)) >= MAP_SIZE) {
		return NULL;
	}
	
	x_coord = FLAT_X_COORD(map);
	y_coord = FLAT_Y_COORD(map);
	
	// check map bounds on x coordinate, and shift positions
	if (x_coord + x_shift < 0) {
		if (WRAP_X) {
			loc += x_shift + MAP_WIDTH;
		}
		else {
			// off the left side
			return NULL;
		}
	}
	else if (x_coord + x_shift >= MAP_WIDTH) {
		if (WRAP_X) {
			loc += x_shift - MAP_WIDTH;
		}
		else {
			// off the right side
			return NULL;
		}
	}
	else {
		loc += x_shift;
	}
	
	if (y_coord + y_shift < 0) {
		if (WRAP_Y) {
			loc += (y_shift * MAP_WIDTH) + MAP_SIZE;
		}
		else {
			// off the bottom
			return NULL;
		}
	}
	else if (y_coord + y_shift >= MAP_HEIGHT) {
		if (WRAP_Y) {
			loc += (y_shift * MAP_WIDTH) - MAP_SIZE;
		}
		else {
			// off the top
			return NULL;
		}
	}
	else {
		loc += (y_shift * MAP_WIDTH);
	}
	
	// again, we can ONLY return map locations
	if (loc >= 0 && loc < MAP_SIZE) {
		return real_room(loc);
	}
	else {
		return NULL;
	}
}


/**
* Removes all players from a room.
*
* @param room_data *room The room to clear of players.
* @param room_data *to_room Optional: A place to send them (or NULL for auto-detect).
*/
void relocate_players(room_data *room, room_data *to_room) {
	char_data *ch, *next_ch;
	room_data *target;
	
	// sanity
	if (to_room == room) {
		to_room = NULL;
	}
	
	for (ch = ROOM_PEOPLE(room); ch; ch = next_ch) {
		next_ch = ch->next_in_room;
	
		if (!IS_NPC(ch)) {
			if (!(target = to_room)) {
				target = find_load_room(ch);
			}
			// absolute chaos
			if (target == room) {
				// abort!
				return;
			}
			
			char_to_room(ch, target);
			GET_LAST_DIR(ch) = NO_DIR;
			look_at_room(ch);
			act("$n appears in the middle of the room!", TRUE, ch, NULL, NULL, TO_ROOM);
			enter_wtrigger(IN_ROOM(ch), ch, NO_DIR);
		}
	}
}


/**
* This approximates a straight line on the map, where "iter" is how far
* along the line.
*
* @param room_data *origin -- start of the line
* @param room_data *destination -- start of 
* @return room_data* The resulting position, or NULL if it's off the map.
*/
room_data *straight_line(room_data *origin, room_data *destination, int iter) {
	int x1, x2, y1, y2, new_x, new_y;
	double slope, dx, dy;
	room_vnum new_loc;
	
	x1 = X_COORD(origin);
	y1 = Y_COORD(origin);
	
	x2 = X_COORD(destination);
	y2 = Y_COORD(destination);
	
	dx = x2 - x1;
	dy = y2 - y1;
	
	slope = dy / dx;
	
	// moving left
	if (dx < 0) {
		iter *= -1;
	}
	
	new_x = x1 + iter;
	new_y = y1 + round(slope * (double)iter);
	
	// bounds check
	if (WRAP_X) {
		new_x = WRAP_X_COORD(new_x);
	}
	else {
		return NULL;
	}
	if (WRAP_Y) {
		new_y = WRAP_Y_COORD(new_y);
	}
	else {
		return NULL;
	}
	
	// new position as a vnum
	new_loc = new_y * MAP_WIDTH + new_x;
	if (new_loc < MAP_SIZE) {
		return real_room(new_loc);
	}
	else {
		// straight line should always stay on the map
		return NULL;
	}
}


/**
* Gets the x-coordinate for a room, based on its map location. This used to be
* a macro, but it needs to ensure there IS a map location.
*
* @param room_data *room The room to get the x-coordinate for.
* @return int The x-coordinate, or -1 if none.
*/
int X_COORD(room_data *room) {
	room_data *map = get_map_location_for(room);
	if (map) {
		return FLAT_X_COORD(map);
	}
	else {
		return -1;
	}
}


/**
* Gets the y-coordinate for a room, based on its map location. This used to be
* a macro, but it needs to ensure there IS a map location.
*
* @param room_data *room The room to get the y-coordinate for.
* @return int The y-coordinate, or -1 if none.
*/
int Y_COORD(room_data *room) {
	room_data *map = get_map_location_for(room);
	if (map) {
		return FLAT_Y_COORD(map);
	}
	else {
		return -1;
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// MISC UTILS //////////////////////////////////////////////////////////////

/**
* @return unsigned long long The current timestamp as microtime (1 million per second)
*/
unsigned long long microtime(void) {
	struct timeval time;
	
	gettimeofday(&time, NULL);
	return ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
}


/**
* This re-usable function is for making tweaks to all players, for example
* removing a player flag that's no longer used. It loads all players who aren't
* already in-game.
*
* Put your desired updates into update_one_player(). The players will be
* loaded and saved automatically.
*
* @param char_data *to_message Optional: a character to send error messages to, if this is called from a command.
* @param PLAYER_UPDATE_FUNC(*func)  A function pointer for the function to run on each player.
*/
void update_all_players(char_data *to_message, PLAYER_UPDATE_FUNC(*func)) {
	struct char_file_u chdata;
	descriptor_data *desc;
	char_data *ch;
	int pos;
	bool is_file;
	
	// verify there are no characters at menus
	for (desc = descriptor_list; desc; desc = desc->next) {
		if (STATE(desc) != CON_PLAYING && desc->character != NULL) {
			strcpy(buf, "update_all_players: Unable to update because of connection(s) at the menu. Try again later.");
			if (to_message) {
				msg_to_char(to_message, "%s\r\n", buf);
			}
			else {
				log("SYSERR: %s", buf);
			}
			// abort!
			return;
		}
	}
	
	// verify there are no disconnected players characters in-game, which might not be saved
	for (ch = character_list; ch; ch = ch->next) {
		if (!IS_NPC(ch) && !ch->desc) {
			sprintf(buf, "update_all_players: Unable to update because of linkdead player (%s). Try again later.", GET_NAME(ch));
			if (to_message) {
				msg_to_char(to_message, "%s\r\n", buf);
			}
			else {
				log("SYSERR: %s", buf);
			}
			// abort!
			return;
		}
	}

	// ok, ready to roll
	for (pos = 0; pos <= top_of_p_table; ++pos) {
		// need chdata either way; check deleted here
		if (load_char(player_table[pos].name, &chdata) <= NOBODY || IS_SET(chdata.char_specials_saved.act, PLR_DELETED)) {
			continue;
		}
		
		if (!(ch = find_or_load_player(player_table[pos].name, &is_file))) {
			continue;
		}
		
		if (func) {
			(func)(ch, is_file);
		}
		
		// save
		if (is_file) {
			store_loaded_char(ch);
			is_file = FALSE;
			ch = NULL;
		}
		else {
			SAVE_CHAR(ch);
		}
	}
}
