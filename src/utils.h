/* ************************************************************************
*   File: utils.h                                         EmpireMUD 2.0b3 *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2015                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/**
* Contents:
*   Core Utils
*   Adventure Utils
*   Bitvector Utils
*   Building Utils
*   Can See Utils
*   Can See Obj Utils
*   Character Utils
*   Craft Utils
*   Crop Utils
*   Descriptor Utils
*   Empire Utils
*   Fight Utils
*   Global Utils
*   Map Utils
*   Memory Utils
*   Mobile Utils
*   Object Utils
*   Objval Utils
*   Player Utils
*   Room Utils
*   Room Template Utils
*   Sector Utils
*   String Utils
*   Const Externs
*   Util Function Protos
*   Miscellaneous Utils
*   Consts for utils.c
*/

 //////////////////////////////////////////////////////////////////////////////
//// CORE UTILS //////////////////////////////////////////////////////////////

/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
	#define NULL (void *)0
#endif

#if !defined(FALSE)
	#define FALSE 0
#endif

#if !defined(TRUE)
	#define TRUE  (!FALSE)
#endif


/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
#endif


/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * EMPIRE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if defined(NOCRYPT) || !defined(EMPIRE_CRYPT)
	#define CRYPT(a, b) (a)
#else
	#define CRYPT(a, b) ((char *) crypt((a),(b)))
#endif


 //////////////////////////////////////////////////////////////////////////////
//// ADVENTURE UTILS /////////////////////////////////////////////////////////

#define GET_ADV_VNUM(adv)  ((adv)->vnum)
#define GET_ADV_NAME(adv)  ((adv)->name)
#define GET_ADV_AUTHOR(adv)  ((adv)->author)
#define GET_ADV_DESCRIPTION(adv)  ((adv)->description)
#define GET_ADV_START_VNUM(adv)  ((adv)->start_vnum)
#define GET_ADV_END_VNUM(adv)  ((adv)->end_vnum)
#define GET_ADV_MIN_LEVEL(adv)  ((adv)->min_level)
#define GET_ADV_MAX_LEVEL(adv)  ((adv)->max_level)
#define GET_ADV_MAX_INSTANCES(adv)  ((adv)->max_instances)
#define GET_ADV_RESET_TIME(adv)  ((adv)->reset_time)
#define GET_ADV_FLAGS(adv)  ((adv)->flags)
#define GET_ADV_LINKING(adv)  ((adv)->linking)
#define GET_ADV_PLAYER_LIMIT(adv)  ((adv)->player_limit)
#define GET_ADV_SCRIPTS(adv)  ((adv)->proto_script)

// utils
#define ADVENTURE_FLAGGED(adv, flg)  (IS_SET(GET_ADV_FLAGS(adv), (flg)) ? TRUE : FALSE)
#define INSTANCE_FLAGGED(i, flg)  (IS_SET((i)->flags, (flg)))
#define LINK_FLAGGED(lnkptr, flg)  (IS_SET((lnkptr)->flags, (flg)) ? TRUE : FALSE)


 //////////////////////////////////////////////////////////////////////////////
//// BITVECTOR UTILS /////////////////////////////////////////////////////////

#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit)  ((var) = (var) ^ (bit))

#define PLR_TOG_CHK(ch,flag)  ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag)  ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))


 //////////////////////////////////////////////////////////////////////////////
//// BUILDING UTILS //////////////////////////////////////////////////////////

#define GET_BLD_VNUM(bld)  ((bld)->vnum)
#define GET_BLD_NAME(bld)  ((bld)->name)
#define GET_BLD_TITLE(bld)  ((bld)->title)
#define GET_BLD_ICON(bld)  ((bld)->icon)
#define GET_BLD_COMMANDS(bld)  ((bld)->commands)
#define GET_BLD_DESC(bld)  ((bld)->description)
#define GET_BLD_MAX_DAMAGE(bld)  ((bld)->max_damage)
#define GET_BLD_FAME(bld)  ((bld)->fame)
#define GET_BLD_FLAGS(bld)  ((bld)->flags)
#define GET_BLD_UPGRADES_TO(bld)  ((bld)->upgrades_to)
#define GET_BLD_EX_DESCS(bld)  ((bld)->ex_description)
#define GET_BLD_EXTRA_ROOMS(bld)  ((bld)->extra_rooms)
#define GET_BLD_DESIGNATE_FLAGS(bld)  ((bld)->designate_flags)
#define GET_BLD_BASE_AFFECTS(bld)  ((bld)->base_affects)
#define GET_BLD_CITIZENS(bld)  ((bld)->citizens)
#define GET_BLD_MILITARY(bld)  ((bld)->military)
#define GET_BLD_ARTISAN(bld)  ((bld)->artisan_vnum)
#define GET_BLD_SPAWNS(bld)  ((bld)->spawns)
#define GET_BLD_INTERACTIONS(bld)  ((bld)->interactions)


 //////////////////////////////////////////////////////////////////////////////
//// CAN SEE UTILS ///////////////////////////////////////////////////////////

// this all builds up to CAN_SEE
#define LIGHT_OK(sub)  (!AFF_FLAGGED(sub, AFF_BLIND) && CAN_SEE_IN_DARK_ROOM((sub), (IN_ROOM(sub))))
#define INVIS_OK(sub, obj)  ((!AFF_FLAGGED(obj, AFF_INVISIBLE)) && (!AFF_FLAGGED((obj), AFF_HIDE) || AFF_FLAGGED((sub), AFF_SENSE_HIDE)))


#define MORT_CAN_SEE_NO_DARK(sub, obj)  ((INVIS_OK(sub, obj)) && !AFF_FLAGGED(sub, AFF_BLIND))
#define MORT_CAN_SEE_LIGHT(sub, obj)  (LIGHT_OK(sub) || (IN_ROOM(sub) == IN_ROOM(obj) && HAS_ABILITY((sub), ABIL_PREDATOR_VISION)))
#define MORT_CAN_SEE(sub, obj)  (MORT_CAN_SEE_LIGHT(sub, obj) && MORT_CAN_SEE_NO_DARK(sub, obj))

#define IMM_CAN_SEE(sub, obj)  (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))
#define IMM_CAN_SEE_NO_DARK(sub, obj)  (MORT_CAN_SEE_NO_DARK(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

// for things like "who"
#define CAN_SEE_GLOBAL(sub, obj)  (SELF(sub, obj) || (GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))))

// Can subject see character "obj"?
#define CAN_SEE_DARK(sub, obj)  (SELF(sub, obj) || (!EXTRACTED(obj) && CAN_SEE_GLOBAL(sub, obj) && IMM_CAN_SEE(sub, obj)))
#define CAN_SEE_NO_DARK(sub, obj)  (SELF(sub, obj) || (!EXTRACTED(obj) && CAN_SEE_GLOBAL(sub, obj) && IMM_CAN_SEE_NO_DARK(sub, obj)))

// The big question...
#define CAN_SEE(sub, obj)  (Global_ignore_dark ? CAN_SEE_NO_DARK(sub, obj) : CAN_SEE_DARK(sub, obj))

#define WIZHIDE_OK(sub, obj)  (!IS_IMMORTAL(obj) || !PRF_FLAGGED((obj), PRF_WIZHIDE) || (!IS_NPC(sub) && GET_ACCESS_LEVEL(sub) >= GET_ACCESS_LEVEL(obj) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))
#define INCOGNITO_OK(sub, obj)  (!IS_IMMORTAL(obj) || IS_IMMORTAL(sub) || !PRF_FLAGGED((obj), PRF_INCOGNITO))


 //////////////////////////////////////////////////////////////////////////////
//// CAN SEE OBJ UTILS ///////////////////////////////////////////////////////

#define CAN_SEE_OBJ_CARRIER(sub, obj)  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) && (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))
#define MORT_CAN_SEE_OBJ(sub, obj)  ((LIGHT_OK(sub) || obj->worn_by == sub || obj->carried_by == sub) && CAN_SEE_OBJ_CARRIER(sub, obj))
#define CAN_SEE_OBJ(sub, obj)  (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))


 //////////////////////////////////////////////////////////////////////////////
//// CHARACTER UTILS /////////////////////////////////////////////////////////

// ch: char_data
#define GET_AGE(ch)  ((!IS_NPC(ch) && IS_VAMPIRE(ch)) ? GET_APPARENT_AGE(ch) : age(ch)->year)
#define GET_EQ(ch, i)  ((ch)->equipment[i])
#define GET_PFILEPOS(ch)  ((ch)->pfilepos)
#define GET_REAL_AGE(ch)  (age(ch)->year)
#define IN_ROOM(ch)  ((ch)->in_room)
#define GET_LOYALTY(ch)  ((ch)->loyalty)

// ch->aff_attributes, ch->real_attributes:
#define GET_REAL_ATT(ch, att)  ((ch)->real_attributes[(att)])
#define GET_ATT(ch, att)  ((ch)->aff_attributes[(att)])
#define GET_STRENGTH(ch)  GET_ATT(ch, STRENGTH)
#define GET_DEXTERITY(ch)  GET_ATT(ch, DEXTERITY)
#define GET_CHARISMA(ch)  GET_ATT(ch, CHARISMA)
#define GET_GREATNESS(ch)  GET_ATT(ch, GREATNESS)
#define GET_INTELLIGENCE(ch)  GET_ATT(ch, INTELLIGENCE)
#define GET_WITS(ch)  GET_ATT(ch, WITS)

// ch->player: char_player_data
#define GET_ACCESS_LEVEL(ch)  ((ch)->player.access_level)
#define GET_LONG_DESC(ch)  ((ch)->player.long_descr)
#define GET_LORE(ch)  ((ch)->player.lore)
#define GET_PASSWD(ch)  ((ch)->player.passwd)
#define GET_PC_NAME(ch)  ((ch)->player.name)
#define GET_REAL_NAME(ch)  (GET_NAME(REAL_CHAR(ch)))
#define GET_REAL_SEX(ch)  ((ch)->player.sex)
#define GET_SHORT_DESC(ch)  ((ch)->player.short_descr)

// ch->points: char_point_data
#define GET_CURRENT_POOL(ch, pool)  ((ch)->points.current_pools[(pool)])
#define GET_MAX_POOL(ch, pool)  ((ch)->points.max_pools[(pool)])
#define GET_DEFICIT(ch, pool)  ((ch)->points.deficit[(pool)])
#define GET_EXTRA_ATT(ch, att)  ((ch)->points.extra_attributes[(att)])

// ch->points: specific pools
#define GET_HEALTH(ch)  GET_CURRENT_POOL(ch, HEALTH)
#define GET_MAX_HEALTH(ch)  GET_MAX_POOL(ch, HEALTH)
#define GET_MANA(ch)  GET_CURRENT_POOL(ch, MANA)
#define GET_MAX_MANA(ch)  GET_MAX_POOL(ch, MANA)
#define GET_MOVE(ch)  GET_CURRENT_POOL(ch, MOVE)
#define GET_MAX_MOVE(ch)  GET_MAX_POOL(ch, MOVE)
#define GET_BLOOD(ch)  GET_CURRENT_POOL(ch, BLOOD)
extern int GET_MAX_BLOOD(char_data *ch);	// this one is different than the other max pools, and max_pools[BLOOD] is not used.
#define GET_HEALTH_DEFICIT(ch)  GET_DEFICIT((ch), HEALTH)
#define GET_MOVE_DEFICIT(ch)  GET_DEFICIT((ch), MOVE)
#define GET_MANA_DEFICIT(ch)  GET_DEFICIT((ch), MANA)
#define GET_BONUS_INVENTORY(ch)  GET_EXTRA_ATT(ch, ATT_BONUS_INVENTORY)
#define GET_RESIST_PHYSICAL(ch)  GET_EXTRA_ATT(ch, ATT_RESIST_PHYSICAL)
#define GET_RESIST_MAGICAL(ch)  GET_EXTRA_ATT(ch, ATT_RESIST_MAGICAL)
#define GET_BLOCK(ch)  GET_EXTRA_ATT(ch, ATT_BLOCK)
#define GET_TO_HIT(ch)  GET_EXTRA_ATT(ch, ATT_TO_HIT)
#define GET_DODGE(ch)  GET_EXTRA_ATT(ch, ATT_DODGE)
#define GET_EXTRA_BLOOD(ch)  GET_EXTRA_ATT(ch, ATT_EXTRA_BLOOD)
#define GET_BONUS_PHYSICAL(ch)  GET_EXTRA_ATT(ch, ATT_BONUS_PHYSICAL)
#define GET_BONUS_MAGICAL(ch)  GET_EXTRA_ATT(ch, ATT_BONUS_MAGICAL)
#define GET_BONUS_HEALING(ch)  GET_EXTRA_ATT(ch, ATT_BONUS_HEALING)	// use total_bonus_healing(ch) for most uses
#define GET_HEAL_OVER_TIME(ch)  GET_EXTRA_ATT(ch, ATT_HEAL_OVER_TIME)
#define GET_CRAFTING_BONUS(ch)  GET_EXTRA_ATT(ch, ATT_CRAFTING_BONUS)

// ch->char_specials: char_special_data
#define FIGHTING(ch)  ((ch)->char_specials.fighting.victim)
#define FIGHT_MODE(ch)  ((ch)->char_specials.fighting.mode)
#define FIGHT_WAIT(ch)  ((ch)->char_specials.fighting.wait)
#define GET_EMPIRE_NPC_DATA(ch)  ((ch)->char_specials.empire_npc)
#define GET_FED_ON_BY(ch)  ((ch)->char_specials.fed_on_by)
#define GET_FEEDING_FROM(ch)  ((ch)->char_specials.feeding_from)
#define GET_HEALTH_REGEN(ch)  ((ch)->char_specials.health_regen)
#define GET_ID(x)  ((x)->id)
#define GET_IDNUM(ch)  (REAL_CHAR(ch)->char_specials.saved.idnum)
#define GET_LEADING(ch)  ((ch)->char_specials.leading)
#define GET_LED_BY(ch)  ((ch)->char_specials.led_by)
#define GET_MANA_REGEN(ch)  ((ch)->char_specials.mana_regen)
#define GET_MOVE_REGEN(ch)  ((ch)->char_specials.move_regen)
#define GET_POS(ch)  ((ch)->char_specials.position)
#define GET_PULLING(ch)  ((ch)->char_specials.pulling)
#define HUNTING(ch)  ((ch)->char_specials.hunting)
#define IS_CARRYING_N(ch)  ((ch)->char_specials.carry_items)
#define ON_CHAIR(ch)  ((ch)->char_specials.chair)


// definitions
#define AWAKE(ch)  (GET_POS(ch) > POS_SLEEPING || GET_POS(ch) == POS_DEAD)
#define CAN_CARRY_N(ch)  (25 + GET_BONUS_INVENTORY(ch) + (HAS_BONUS_TRAIT(ch, BONUS_INVENTORY) ? 5 : 0) + (GET_EQ((ch), WEAR_PACK) ? GET_PACK_CAPACITY(GET_EQ(ch, WEAR_PACK)) : 0))
#define CAN_CARRY_OBJ(ch,obj)  ((IS_CARRYING_N(ch) + GET_OBJ_INVENTORY_SIZE(obj)) <= CAN_CARRY_N(ch))
#define CAN_GET_OBJ(ch, obj)  (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && CAN_SEE_OBJ((ch),(obj)))
#define CAN_RECOGNIZE(ch, vict)  (IS_IMMORTAL(ch) || (!AFF_FLAGGED(vict, AFF_NO_SEE_IN_ROOM | AFF_HIDE) && !MORPH_FLAGGED(vict, MORPH_FLAG_ANIMAL) && !IS_DISGUISED(vict)))
#define CAN_RIDE_FLYING_MOUNT(ch)  (HAS_ABILITY((ch), ABIL_ALL_TERRAIN_RIDING) || HAS_ABILITY((ch), ABIL_FLY) || HAS_ABILITY((ch), ABIL_DRAGONRIDING))
#define CAN_SEE_IN_DARK(ch)  (HAS_INFRA(ch) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
#define CAN_SEE_IN_DARK_ROOM(ch, room)  ((WOULD_BE_LIGHT_WITHOUT_MAGIC_DARKNESS(room) || (room == IN_ROOM(ch) && (HAS_ABILITY(ch, ABIL_BY_MOONLIGHT))) || CAN_SEE_IN_DARK(ch)) && (!MAGIC_DARKNESS(room) || CAN_SEE_IN_MAGIC_DARKNESS(ch)))
#define CAN_SEE_IN_MAGIC_DARKNESS(ch)  (IS_NPC(ch) ? (get_approximate_level(ch) > 100) : HAS_ABILITY((ch), ABIL_DARKNESS))
#define CAN_SPEND_BLOOD(ch)  (!AFF_FLAGGED(ch, AFF_CANT_SPEND_BLOOD))
#define CAST_BY_ID(ch)  (IS_NPC(ch) ? (-1 * GET_MOB_VNUM(ch)) : GET_IDNUM(ch))
#define EFFECTIVELY_FLYING(ch)  (IS_RIDING(ch) ? MOUNT_FLAGGED(ch, MOUNT_FLYING) : AFF_FLAGGED(ch, AFF_FLY))
#define HAS_INFRA(ch)  AFF_FLAGGED(ch, AFF_INFRAVISION)
#define IS_HUMAN(ch)  (!IS_VAMPIRE(ch))
#define IS_MAGE(ch)  (IS_NPC(ch) ? GET_MAX_MANA(ch) > 0 : (GET_SKILL((ch), SKILL_NATURAL_MAGIC) > 0 || GET_SKILL((ch), SKILL_HIGH_SORCERY) > 0))
#define IS_OUTDOORS(ch)  IS_OUTDOOR_TILE(IN_ROOM(ch))
#define IS_VAMPIRE(ch)  (IS_NPC(ch) ? MOB_FLAGGED((ch), MOB_VAMPIRE) : PLR_FLAGGED((ch), PLR_VAMPIRE))

// helpers
#define AFF_FLAGGED(ch, flag)  (IS_SET(AFF_FLAGS(ch), (flag)))
#define EXTRACTED(ch)  (PLR_FLAGGED((ch), PLR_EXTRACTED) || MOB_FLAGGED((ch), MOB_EXTRACTED))
#define GET_NAME(ch)  (IS_NPC(ch) ? GET_SHORT_DESC(ch) : GET_PC_NAME(ch))
#define GET_REAL_LEVEL(ch)  (ch->desc && ch->desc->original ? GET_ACCESS_LEVEL(ch->desc->original) : GET_ACCESS_LEVEL(ch))
#define GET_SEX(ch)  (IS_DISGUISED(ch) ? GET_DISGUISED_SEX(ch) : GET_REAL_SEX(ch))
#define IS_DEAD(ch)  (GET_POS(ch) == POS_DEAD)
#define IS_INJURED(ch, flag)  (IS_SET(INJURY_FLAGS(ch), (flag)))
#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define REAL_CHAR(ch)  (((ch)->desc && (ch)->desc->original) ? (ch)->desc->original : (ch))
#define REAL_NPC(ch)  (IS_NPC(REAL_CHAR(ch)))

// wait!
#define GET_WAIT_STATE(ch)  ((ch)->wait)


 //////////////////////////////////////////////////////////////////////////////
//// CRAFT UTILS /////////////////////////////////////////////////////////////

#define GET_CRAFT_ABILITY(craft)  ((craft)->ability)
#define GET_CRAFT_BUILD_FACING(craft)  ((craft)->build_facing)
#define GET_CRAFT_BUILD_ON(craft)  ((craft)->build_on)
#define GET_CRAFT_BUILD_TYPE(craft)  ((craft)->build_type)
#define GET_CRAFT_FLAGS(craft)  ((craft)->flags)
#define GET_CRAFT_MIN_LEVEL(craft)  ((craft)->min_level)
#define GET_CRAFT_NAME(craft)  ((craft)->name)
#define GET_CRAFT_OBJECT(craft)  ((craft)->object)
#define GET_CRAFT_QUANTITY(craft)  ((craft)->quantity)
#define GET_CRAFT_REQUIRES_OBJ(craft)  ((craft)->requires_obj)
#define GET_CRAFT_RESOURCES(craft)  ((craft)->resources)
#define GET_CRAFT_TIME(craft)  ((craft)->time)
#define GET_CRAFT_TYPE(craft)  ((craft)->type)
#define GET_CRAFT_VNUM(craft)  ((craft)->vnum)

#define CRAFT_FLAGGED(cr, flg)  (IS_SET(GET_CRAFT_FLAGS(cr), (flg)) ? TRUE : FALSE)


 //////////////////////////////////////////////////////////////////////////////
//// CROP UTILS //////////////////////////////////////////////////////////////

#define GET_CROP_CLIMATE(crop)  ((crop)->climate)
#define GET_CROP_FLAGS(crop)  ((crop)->flags)
#define GET_CROP_ICONS(crop)  ((crop)->icons)
#define GET_CROP_INTERACTIONS(crop)  ((crop)->interactions)
#define GET_CROP_MAPOUT(crop)  ((crop)->mapout)
#define GET_CROP_NAME(crop)  ((crop)->name)
#define GET_CROP_SPAWNS(crop)  ((crop)->spawns)
#define GET_CROP_TITLE(crop)  ((crop)->title)
#define GET_CROP_VNUM(crop)  ((crop)->vnum)
#define GET_CROP_X_MAX(crop)  ((crop)->x_max)
#define GET_CROP_X_MIN(crop)  ((crop)->x_min)
#define GET_CROP_Y_MAX(crop)  ((crop)->y_max)
#define GET_CROP_Y_MIN(crop)  ((crop)->y_min)

// helpers
#define CROP_FLAGGED(crp, flg)  (IS_SET(GET_CROP_FLAGS(crp), (flg)))


 //////////////////////////////////////////////////////////////////////////////
//// DESCRIPTOR UTILS ////////////////////////////////////////////////////////

// basic
#define STATE(d)  ((d)->connected)


// olc
#define GET_OLC_TYPE(desc)  ((desc)->olc_type)
#define GET_OLC_VNUM(desc)  ((desc)->olc_vnum)
#define GET_OLC_STORAGE(desc)  ((desc)->olc_storage)
#define GET_OLC_ADVENTURE(desc)  ((desc)->olc_adventure)
#define GET_OLC_BOOK(desc)  ((desc)->olc_book)
#define GET_OLC_BUILDING(desc)  ((desc)->olc_building)
#define GET_OLC_CRAFT(desc)  ((desc)->olc_craft)
#define GET_OLC_CROP(desc)  ((desc)->olc_crop)
#define GET_OLC_GLOBAL(desc)  ((desc)->olc_global)
#define GET_OLC_MOBILE(desc)  ((desc)->olc_mobile)
#define GET_OLC_OBJECT(desc)  ((desc)->olc_object)
#define GET_OLC_ROOM_TEMPLATE(desc)  ((desc)->olc_room_template)
#define GET_OLC_SECTOR(desc)  ((desc)->olc_sector)
#define GET_OLC_TRIGGER(desc)  ((desc)->olc_trigger)


 //////////////////////////////////////////////////////////////////////////////
//// EMPIRE UTILS ////////////////////////////////////////////////////////////

#define EMPIRE_VNUM(emp)  ((emp)->vnum)
#define EMPIRE_LEADER(emp)  ((emp)->leader)
#define EMPIRE_CREATE_TIME(emp)  ((emp)->create_time)
#define EMPIRE_NAME(emp)  ((emp)->name)
#define EMPIRE_ADJECTIVE(emp)  ((emp)->adjective)
#define EMPIRE_BANNER(emp)  ((emp)->banner)
#define EMPIRE_NUM_RANKS(emp)  ((emp)->num_ranks)
#define EMPIRE_RANK(emp, num)  ((emp)->rank[(num)])
#define EMPIRE_FRONTIER_TRAITS(emp)  ((emp)->frontier_traits)
#define EMPIRE_COINS(emp)  ((emp)->coins)
#define EMPIRE_PRIV(emp, num)  ((emp)->priv[(num)])
#define EMPIRE_DESCRIPTION(emp)  ((emp)->description)
#define EMPIRE_DIPLOMACY(emp)  ((emp)->diplomacy)
#define EMPIRE_STORAGE(emp)  ((emp)->store)
#define EMPIRE_TRADE(emp)  ((emp)->trade)
#define EMPIRE_LOGS(emp)  ((emp)->logs)
#define EMPIRE_TERRITORY_LIST(emp)  ((emp)->territory_list)
#define EMPIRE_CITY_LIST(emp)  ((emp)->city_list)
#define EMPIRE_CITY_TERRITORY(emp)  ((emp)->city_terr)
#define EMPIRE_OUTSIDE_TERRITORY(emp)  ((emp)->outside_terr)
#define EMPIRE_WEALTH(emp)  ((emp)->wealth)
#define EMPIRE_POPULATION(emp)  ((emp)->population)
#define EMPIRE_MILITARY(emp)  ((emp)->military)
#define EMPIRE_MOTD(emp)  ((emp)->motd)
#define EMPIRE_GREATNESS(emp)  ((emp)->greatness)
#define EMPIRE_TECH(emp, num)  ((emp)->tech[(num)])
#define EMPIRE_ISLAND_TECH(emp, isle, tech)  ((emp)->island_tech[isle][tech])
#define EMPIRE_MEMBERS(emp)  ((emp)->members)
#define EMPIRE_TOTAL_MEMBER_COUNT(emp)  ((emp)->total_member_count)
#define EMPIRE_TOTAL_PLAYTIME(emp)  ((emp)->total_playtime)
#define EMPIRE_IMM_ONLY(emp)  ((emp)->imm_only)
#define EMPIRE_FAME(emp)  ((emp)->fame)
#define EMPIRE_LAST_LOGON(emp)  ((emp)->last_logon)
#define EMPIRE_CHORE(emp, num)  ((emp)->chore_active[(num)])
#define EMPIRE_SCORE(emp, num)  ((emp)->scores[(num)])
#define EMPIRE_SHIPPING_LIST(emp)  ((emp)->shipping_list)
#define EMPIRE_SORT_VALUE(emp)  ((emp)->sort_value)
#define EMPIRE_UNIQUE_STORAGE(emp)  ((emp)->unique_store)
#define EMPIRE_WORKFORCE_TRACKER(emp)  ((emp)->ewt_tracker)

// helpers
#define EMPIRE_HAS_TECH(emp, num)  (EMPIRE_TECH((emp), (num)) > 0)
#define EMPIRE_HAS_TECH_ON_ISLAND(emp, ii, tt)  (((emp) && (ii) != NO_ISLAND) ? EMPIRE_ISLAND_TECH(emp, ii, tt) : FALSE)
#define EMPIRE_IS_TIMED_OUT(emp)  (EMPIRE_LAST_LOGON(emp) + (config_get_int("whole_empire_timeout") * SECS_PER_REAL_DAY) < time(0))
#define GET_TOTAL_WEALTH(emp)  (EMPIRE_WEALTH(emp) + (int)(EMPIRE_COINS(emp) * COIN_VALUE))

// definitions
#define SAME_EMPIRE(ch, vict)  (!IS_NPC(ch) && !IS_NPC(vict) && GET_LOYALTY(ch) != NULL && GET_LOYALTY(ch) == GET_LOYALTY(vict))

// only some types of tiles are kept in the shortlist of territories -- particularly ones with associated chores
#define BELONGS_IN_TERRITORY_LIST(room)  (IS_ANY_BUILDING(room) || COMPLEX_DATA(room) || ROOM_SECT_FLAGGED(room, SECTF_CHORE))


 //////////////////////////////////////////////////////////////////////////////
//// FIGHT UTILS /////////////////////////////////////////////////////////////

#define SHOULD_APPEAR(ch)  AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)


 //////////////////////////////////////////////////////////////////////////////
//// GLOBAL UTILS ////////////////////////////////////////////////////////////

#define GET_GLOBAL_VNUM(glb)  ((glb)->vnum)
#define GET_GLOBAL_NAME(glb)  ((glb)->name)
#define GET_GLOBAL_TYPE(glb)  ((glb)->type)
#define GET_GLOBAL_FLAGS(glb)  ((glb)->flags)
#define GET_GLOBAL_TYPE_EXCLUDE(glb)  ((glb)->type_exclude)
#define GET_GLOBAL_TYPE_FLAGS(glb)  ((glb)->type_flags)
#define GET_GLOBAL_MIN_LEVEL(glb)  ((glb)->min_level)
#define GET_GLOBAL_MAX_LEVEL(glb)  ((glb)->max_level)
#define GET_GLOBAL_INTERACTIONS(glb)  ((glb)->interactions)


 //////////////////////////////////////////////////////////////////////////////
//// MAP UTILS ///////////////////////////////////////////////////////////////

// returns TRUE only if both x and y are in the bounds of the map
#define CHECK_MAP_BOUNDS(x, y)  ((x) >= 0 && (x) < MAP_WIDTH && (y) >= 0 && (y) < MAP_HEIGHT)

// flat coords ASSUME room is on the map -- otherwise use the X_COORD/Y_COORD macros
#define FLAT_X_COORD(room)  (GET_ROOM_VNUM(room) % MAP_WIDTH)
#define FLAT_Y_COORD(room)  (GET_ROOM_VNUM(room) / MAP_WIDTH)

extern int X_COORD(room_data *room);	// formerly #define X_COORD(room)  FLAT_X_COORD(get_map_location_for(room))
extern int Y_COORD(room_data *room);	// formerly #define Y_COORD(room)  FLAT_Y_COORD(get_map_location_for(room))

// wrap x/y "around the edge"
#define WRAP_X_COORD(x)  (WRAP_X ? (((x) < 0) ? ((x) + MAP_WIDTH) : (((x) >= MAP_WIDTH) ? ((x) - MAP_WIDTH) : (x))) : MAX(0, MIN(MAP_WIDTH-1, (x))))
#define WRAP_Y_COORD(y)  (WRAP_Y ? (((y) < 0) ? ((y) + MAP_HEIGHT) : (((y) >= MAP_HEIGHT) ? ((y) - MAP_HEIGHT) : (y))) : MAX(0, MIN(MAP_HEIGHT-1, (y))))


 //////////////////////////////////////////////////////////////////////////////
//// MEMORY UTILS ////////////////////////////////////////////////////////////

#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ log("SYSERR: malloc failure: %s: %d", __FILE__, __LINE__); abort(); } } while(0)


#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ log("SYSERR: realloc failure: %s: %d", __FILE__, __LINE__); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }


 //////////////////////////////////////////////////////////////////////////////
//// MOBILE UTILS ////////////////////////////////////////////////////////////

// ch->char_specials.saved: char_specials_saved
#define MOB_FLAGS(ch)  ((ch)->char_specials.saved.act)

// ch->mob_specials: mob_special_data
#define GET_CURRENT_SCALE_LEVEL(ch)  ((ch)->mob_specials.current_scale_level)
#define GET_MAX_SCALE_LEVEL(ch)  ((ch)->mob_specials.max_scale_level)
#define GET_MOB_VNUM(mob)  (IS_NPC(mob) ? (mob)->vnum : NOTHING)
#define GET_MIN_SCALE_LEVEL(ch)  ((ch)->mob_specials.min_scale_level)
#define MOB_NAME_SET(ch)  ((ch)->mob_specials.name_set)
#define MOB_ATTACK_TYPE(ch)  ((ch)->mob_specials.attack_type)
#define MOB_DAMAGE(ch)  ((ch)->mob_specials.damage)
#define MOB_DYNAMIC_NAME(ch)  ((ch)->mob_specials.dynamic_name)
#define MOB_DYNAMIC_SEX(ch)  ((ch)->mob_specials.dynamic_sex)
#define MOB_INSTANCE_ID(ch)  ((ch)->mob_specials.instance_id)
#define MOB_MOVE_TYPE(ch)  ((ch)->mob_specials.move_type)
#define MOB_PURSUIT(ch)  ((ch)->mob_specials.pursuit)
#define MOB_PURSUIT_LEASH_LOC(ch)  ((ch)->mob_specials.pursuit_leash_loc)
#define MOB_TAGGED_BY(ch)  ((ch)->mob_specials.tagged_by)
#define MOB_SPAWN_TIME(ch)  ((ch)->mob_specials.spawn_time)
#define MOB_TO_DODGE(ch)  ((ch)->mob_specials.to_dodge)
#define MOB_TO_HIT(ch)  ((ch)->mob_specials.to_hit)

// helpers
#define IS_MOB(ch)  (IS_NPC(ch) && GET_MOB_VNUM(ch) != NOTHING)
#define IS_TAGGED_BY(mob, player)  (IS_NPC(mob) && !IS_NPC(player) && find_id_in_tag_list(GET_IDNUM(player), MOB_TAGGED_BY(mob)))
#define MOB_FLAGGED(ch, flag)  (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))


 //////////////////////////////////////////////////////////////////////////////
//// OBJECT UTILS ////////////////////////////////////////////////////////////

// primary attributes
#define GET_AUTOSTORE_TIMER(obj)  ((obj)->autostore_timer)
#define GET_OBJ_ACTION_DESC(obj)  ((obj)->action_description)
#define GET_OBJ_AFF_FLAGS(obj)  ((obj)->obj_flags.bitvector)
#define GET_OBJ_CARRYING_N(obj)  ((obj)->obj_flags.carrying_n)
#define GET_OBJ_CURRENT_SCALE_LEVEL(obj)  ((obj)->obj_flags.current_scale_level)
#define GET_OBJ_EXTRA(obj)  ((obj)->obj_flags.extra_flags)
#define GET_OBJ_KEYWORDS(obj)  ((obj)->name)
#define GET_OBJ_LONG_DESC(obj)  ((obj)->description)
#define GET_OBJ_MATERIAL(obj)  ((obj)->obj_flags.material)
#define GET_OBJ_MAX_SCALE_LEVEL(obj)  ((obj)->obj_flags.max_scale_level)
#define GET_OBJ_MIN_SCALE_LEVEL(obj)  ((obj)->obj_flags.min_scale_level)
#define GET_OBJ_SHORT_DESC(obj)  ((obj)->short_description)
#define GET_OBJ_TIMER(obj)  ((obj)->obj_flags.timer)
#define GET_OBJ_TYPE(obj)  ((obj)->obj_flags.type_flag)
#define GET_OBJ_VAL(obj, val)  ((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEAR(obj)  ((obj)->obj_flags.wear_flags)
#define GET_PULLED_BY(obj, i)  (i == 0 ? (obj)->pulled_by1 : (obj)->pulled_by2)
#define GET_STOLEN_TIMER(obj)  ((obj)->stolen_timer)
#define IN_CHAIR(obj)  ((obj)->sitting)
#define LAST_OWNER_ID(obj)  ((obj)->last_owner_id)
#define OBJ_BOUND_TO(obj)  ((obj)->bound_to)
#define OBJ_VERSION(obj)  ((obj)->version)

// compound attributes
#define GET_OBJ_DESC(obj, ch, mode)  get_obj_desc((obj), (ch), (mode))
#define GET_OBJ_VNUM(obj)  ((obj)->vnum)

// definitions
#define GET_OBJ_INVENTORY_SIZE(obj)  (OBJ_FLAGGED((obj), OBJ_LARGE) ? 2 : 1)
#define IS_STOLEN(obj)  (GET_STOLEN_TIMER(obj) > 0 && (config_get_int("stolen_object_timer") * SECS_PER_REAL_MIN) + GET_STOLEN_TIMER(obj) > time(0))

// helpers
#define OBJ_FLAGGED(obj, flag)  (IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define OBJS(obj, vict)  (CAN_SEE_OBJ((vict), (obj)) ? GET_OBJ_DESC((obj), vict, 1) : "something")
#define OBJVAL_FLAGGED(obj, flag)  (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define CAN_WEAR(obj, part)  (IS_SET(GET_OBJ_WEAR(obj), (part)))

// for stacking, sotring, etc
#define OBJ_CAN_STACK(obj)  (GET_OBJ_TYPE(obj) != ITEM_CONTAINER && !OBJ_FLAGGED((obj), OBJ_ENCHANTED) && !IN_CHAIR(obj) && !IS_ARROW(obj))
#define OBJ_CAN_STORE(obj)  ((obj)->storage && !OBJ_BOUND_TO(obj) && !OBJ_FLAGGED((obj), OBJ_SUPERIOR | OBJ_ENCHANTED))
#define UNIQUE_OBJ_CAN_STORE(obj)  (!OBJ_BOUND_TO(obj) && !OBJ_CAN_STORE(obj) && !OBJ_FLAGGED((obj), OBJ_JUNK) && (!OBJ_FLAGGED((obj), OBJ_LIGHT) || GET_OBJ_TIMER(obj) == UNLIMITED) && !IS_STOLEN(obj))
#define OBJ_STACK_FLAGS  (OBJ_SUPERIOR | OBJ_KEEP)
#define OBJS_ARE_SAME(o1, o2)  (GET_OBJ_VNUM(o1) == GET_OBJ_VNUM(o2) && ((GET_OBJ_EXTRA(o1) & OBJ_STACK_FLAGS) == (GET_OBJ_EXTRA(o2) & OBJ_STACK_FLAGS)) && (!IS_DRINK_CONTAINER(o1) || GET_DRINK_CONTAINER_TYPE(o1) == GET_DRINK_CONTAINER_TYPE(o2)))


 //////////////////////////////////////////////////////////////////////////////
//// OBJVAL UTILS ////////////////////////////////////////////////////////////

// ITEM_POTION
#define IS_POTION(obj)  (GET_OBJ_TYPE(obj) == ITEM_POTION)
#define VAL_POTION_TYPE  0
#define VAL_POTION_SCALE  1
#define GET_POTION_TYPE(obj)  (IS_POTION(obj) ? GET_OBJ_VAL((obj), VAL_POTION_TYPE) : NOTHING)
#define GET_POTION_SCALE(obj)  (IS_POTION(obj) ? GET_OBJ_VAL((obj), VAL_POTION_SCALE) : 0)

// ITEM_POISON
#define IS_POISON(obj)  (GET_OBJ_TYPE(obj) == ITEM_POISON)
#define VAL_POISON_TYPE  0
#define VAL_POISON_CHARGES  1
#define GET_POISON_TYPE(obj)  (IS_POISON(obj) ? GET_OBJ_VAL((obj), VAL_POISON_TYPE) : NOTHING)
#define GET_POISON_CHARGES(obj)  (IS_POISON(obj) ? GET_OBJ_VAL((obj), VAL_POISON_CHARGES) : 0)

// ITEM_WEAPON
#define IS_WEAPON(obj)  (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
#define VAL_WEAPON_DAMAGE_BONUS  1
#define VAL_WEAPON_TYPE  2
#define GET_WEAPON_DAMAGE_BONUS(obj)  (IS_WEAPON(obj) ? GET_OBJ_VAL((obj), VAL_WEAPON_DAMAGE_BONUS) : 0)
#define GET_WEAPON_TYPE(obj)  (IS_WEAPON(obj) ? GET_OBJ_VAL((obj), VAL_WEAPON_TYPE) : TYPE_UNDEFINED)

#define IS_ANY_WEAPON(obj)  (IS_WEAPON(obj) || IS_MISSILE_WEAPON(obj))
#define IS_STAFF(obj)  (GET_WEAPON_TYPE(obj) == TYPE_LIGHTNING_STAFF || GET_WEAPON_TYPE(obj) == TYPE_BURN_STAFF || GET_WEAPON_TYPE(obj) == TYPE_AGONY_STAFF || OBJ_FLAGGED((obj), OBJ_STAFF))

// ITEM_ARMOR subtype
#define IS_ARMOR(obj)  (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
#define VAL_ARMOR_TYPE  0
#define GET_ARMOR_TYPE(obj)  (IS_ARMOR(obj) ? GET_OBJ_VAL((obj), VAL_ARMOR_TYPE) : NOTHING)

// ITEM_WORN subtype
#define IS_WORN_TYPE(obj)  (GET_OBJ_TYPE(obj) == ITEM_WORN)

// ITEM_OTHER
// ITEM_CONTAINER
#define IS_CONTAINER(obj)  (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
#define VAL_CONTAINER_MAX_CONTENTS  0
#define VAL_CONTAINER_FLAGS  1
#define GET_MAX_CONTAINER_CONTENTS(obj)  (IS_CONTAINER(obj) ? GET_OBJ_VAL((obj), VAL_CONTAINER_MAX_CONTENTS) : (IS_CART(obj) ? GET_OBJ_VAL((obj), VAL_CART_MAX_CONTENTS) : 0))
#define GET_CONTAINER_FLAGS(obj)  (IS_CONTAINER(obj) ? GET_OBJ_VAL((obj), VAL_CONTAINER_FLAGS) : 0)

// ITEM_DRINKCON
#define IS_DRINK_CONTAINER(obj)  (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
#define VAL_DRINK_CONTAINER_CAPACITY  0
#define VAL_DRINK_CONTAINER_CONTENTS  1
#define VAL_DRINK_CONTAINER_TYPE  2
#define GET_DRINK_CONTAINER_CAPACITY(obj)  (IS_DRINK_CONTAINER(obj) ? GET_OBJ_VAL((obj), VAL_DRINK_CONTAINER_CAPACITY) : 0)
#define GET_DRINK_CONTAINER_CONTENTS(obj)  (IS_DRINK_CONTAINER(obj) ? GET_OBJ_VAL((obj), VAL_DRINK_CONTAINER_CONTENTS) : 0)
#define GET_DRINK_CONTAINER_TYPE(obj)  (IS_DRINK_CONTAINER(obj) ? GET_OBJ_VAL((obj), VAL_DRINK_CONTAINER_TYPE) : NOTHING)

// ITEM_FOOD
#define IS_FOOD(obj)  (GET_OBJ_TYPE(obj) == ITEM_FOOD)
#define VAL_FOOD_HOURS_OF_FULLNESS  0
#define VAL_FOOD_CROP_TYPE  1
#define GET_FOOD_HOURS_OF_FULLNESS(obj)  (IS_FOOD(obj) ? GET_OBJ_VAL((obj), VAL_FOOD_HOURS_OF_FULLNESS) : 0)
#define IS_PLANTABLE_FOOD(obj)  (IS_FOOD(obj) && OBJ_FLAGGED((obj), OBJ_PLANTABLE))
#define GET_FOOD_CROP_TYPE(obj)  (IS_PLANTABLE_FOOD(obj) ? GET_OBJ_VAL((obj), VAL_FOOD_CROP_TYPE) : NOTHING)

// ITEM_BOAT
// ITEM_BOARD
// ITEM_CORPSE
#define IS_CORPSE(obj)  (GET_OBJ_TYPE(obj) == ITEM_CORPSE)
#define VAL_CORPSE_IDNUM  0
#define VAL_CORPSE_FLAGS  2
#define IS_NPC_CORPSE(obj)  (IS_CORPSE(obj) && GET_OBJ_VAL((obj), VAL_CORPSE_IDNUM) >= 0)
#define IS_PC_CORPSE(obj)  (IS_CORPSE(obj) && GET_OBJ_VAL((obj), VAL_CORPSE_IDNUM) < 0)
#define GET_CORPSE_NPC_VNUM(obj)  (IS_NPC_CORPSE(obj) ? GET_OBJ_VAL((obj), VAL_CORPSE_IDNUM) : NOBODY)
#define GET_CORPSE_PC_ID(obj)  (IS_PC_CORPSE(obj) ? (-1 * GET_OBJ_VAL((obj), VAL_CORPSE_IDNUM)) : NOBODY)
#define GET_CORPSE_FLAGS(obj)  (IS_CORPSE(obj) ? GET_OBJ_VAL((obj), VAL_CORPSE_FLAGS) : 0)
#define IS_CORPSE_FLAG_SET(obj, flag)  (GET_CORPSE_FLAGS(obj) & (flag))

// ITEM_COINS
#define IS_COINS(obj)  (GET_OBJ_TYPE(obj) == ITEM_COINS)
#define VAL_COINS_AMOUNT  0
#define VAL_COINS_EMPIRE_ID  1
#define GET_COINS_AMOUNT(obj)  (IS_COINS(obj) ? GET_OBJ_VAL((obj), VAL_COINS_AMOUNT) : 0)
#define GET_COINS_EMPIRE_ID(obj)  (IS_COINS(obj) ? GET_OBJ_VAL((obj), VAL_COINS_EMPIRE_ID) : 0)

// ITEM_MAIL

// ITEM_WEALTH
#define IS_WEALTH_ITEM(obj)  (GET_OBJ_TYPE(obj) == ITEM_WEALTH)
#define VAL_WEALTH_VALUE  0
#define VAL_WEALTH_AUTOMINT  1
#define GET_WEALTH_VALUE(obj)  (IS_WEALTH_ITEM(obj) ? GET_OBJ_VAL((obj), VAL_WEALTH_VALUE) : 0)
#define GET_WEALTH_AUTOMINT(obj)  (IS_WEALTH_ITEM(obj) ? GET_OBJ_VAL((obj), VAL_WEALTH_AUTOMINT) : 0)

// ITEM_CART
#define IS_CART(obj)  (GET_OBJ_TYPE(obj) == ITEM_CART)
#define VAL_CART_MAX_CONTENTS  0
#define VAL_CART_ANIMALS_REQUIRED  1
#define VAL_CART_FIRING_DATA  2	// this val is 1 for can-fire, >1 for cooling down to fire
#define GET_MAX_CART_CONTENTS(obj)  (IS_CART(obj) ? GET_OBJ_VAL((obj), VAL_CART_MAX_CONTENTS) : (IS_CONTAINER(obj) ? GET_OBJ_VAL((obj), VAL_CONTAINER_MAX_CONTENTS) : 0))
#define GET_CART_ANIMALS_REQUIRED(obj)  (IS_CART(obj) ? GET_OBJ_VAL((obj), VAL_CART_ANIMALS_REQUIRED) : 0)
#define CART_CAN_FIRE(obj)  (IS_CART(obj) ? (GET_OBJ_VAL((obj), VAL_CART_FIRING_DATA) > 0) : FALSE)

// ITEM_SHIP
#define IS_SHIP(obj)  (GET_OBJ_TYPE(obj) == ITEM_SHIP)
#define VAL_SHIP_TYPE  0
#define VAL_SHIP_RESOURCES_REMAINING  1
#define VAL_SHIP_MAIN_ROOM  2
#define GET_SHIP_TYPE(obj)  (IS_SHIP(obj) ? GET_OBJ_VAL((obj), VAL_SHIP_TYPE) : NOTHING)
#define GET_SHIP_RESOURCES_REMAINING(obj)  (IS_SHIP(obj) ? GET_OBJ_VAL((obj), VAL_SHIP_RESOURCES_REMAINING) : 0)
#define GET_SHIP_MAIN_ROOM(obj)  (IS_SHIP(obj) ? GET_OBJ_VAL((obj), VAL_SHIP_MAIN_ROOM) : NOWHERE)

// ITEM_MISSILE_WEAPON
#define IS_MISSILE_WEAPON(obj)  (GET_OBJ_TYPE(obj) == ITEM_MISSILE_WEAPON)
#define VAL_MISSILE_WEAPON_SPEED  0
#define VAL_MISSILE_WEAPON_DAMAGE  1
#define VAL_MISSILE_WEAPON_TYPE  2
#define GET_MISSILE_WEAPON_SPEED(obj)  (IS_MISSILE_WEAPON(obj) ? GET_OBJ_VAL((obj), VAL_MISSILE_WEAPON_SPEED) : 0)
#define GET_MISSILE_WEAPON_DAMAGE(obj)  (IS_MISSILE_WEAPON(obj) ? GET_OBJ_VAL((obj), VAL_MISSILE_WEAPON_DAMAGE) : 0)
#define GET_MISSILE_WEAPON_TYPE(obj)  (IS_MISSILE_WEAPON(obj) ? GET_OBJ_VAL((obj), VAL_MISSILE_WEAPON_TYPE) : NOTHING)

// ITEM_ARROW
#define IS_ARROW(obj)  (GET_OBJ_TYPE(obj) == ITEM_ARROW)
#define VAL_ARROW_QUANTITY  0
#define VAL_ARROW_DAMAGE_BONUS  1
#define VAL_ARROW_TYPE  2
#define GET_ARROW_DAMAGE_BONUS(obj)  (IS_ARROW(obj) ? GET_OBJ_VAL((obj), VAL_ARROW_DAMAGE_BONUS) : 0)
#define GET_ARROW_TYPE(obj)  (IS_ARROW(obj) ? GET_OBJ_VAL((obj), VAL_ARROW_TYPE) : NOTHING)
#define GET_ARROW_QUANTITY(obj)  (IS_ARROW(obj) ? GET_OBJ_VAL((obj), VAL_ARROW_QUANTITY) : 0)

// ITEM_INSTRUMENT
#define IS_INSTRUMENT(obj)  (GET_OBJ_TYPE(obj) == ITEM_INSTRUMENT)

// ITEM_SHIELD
#define IS_SHIELD(obj)  (GET_OBJ_TYPE(obj) == ITEM_SHIELD)

// ITEM_PORTAL
#define IS_PORTAL(obj)  (GET_OBJ_TYPE(obj) == ITEM_PORTAL)
#define VAL_PORTAL_TARGET_VNUM  0
#define GET_PORTAL_TARGET_VNUM(obj)  (IS_PORTAL(obj) ? GET_OBJ_VAL((obj), VAL_PORTAL_TARGET_VNUM) : NOWHERE)

// ITEM_PACK
#define IS_PACK(obj)  (GET_OBJ_TYPE(obj) == ITEM_PACK)
#define VAL_PACK_CAPACITY  0
#define GET_PACK_CAPACITY(obj)  (IS_PACK(obj) ? GET_OBJ_VAL((obj), VAL_PACK_CAPACITY) : 0)

// ITEM_BOOK
#define IS_BOOK(obj)  (GET_OBJ_TYPE(obj) == ITEM_BOOK)
#define VAL_BOOK_ID  0
#define GET_BOOK_ID(obj)  (IS_BOOK(obj) ? GET_OBJ_VAL((obj), VAL_BOOK_ID) : 0)


 //////////////////////////////////////////////////////////////////////////////
//// PLAYER UTILS ////////////////////////////////////////////////////////////

// ch->char_specials.saved: char_special_data_saved
#define AFF_FLAGS(ch)  ((ch)->char_specials.saved.affected_by)
#define INJURY_FLAGS(ch)  ((ch)->char_specials.saved.injuries)
#define PLR_FLAGS(ch)  (REAL_CHAR(ch)->char_specials.saved.act)
#define SYSLOG_FLAGS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.syslogs))

// ch->player_specials: player_special_data
#define GET_ALIASES(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_CREATION_ALT_ID(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->create_alt_id))
#define GET_GEAR_LEVEL(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->gear_level))
#define GET_LASTNAME(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->lastname))
#define GET_LAST_TELL(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_MARK_LOCATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->marked_location))
#define GET_GROUP_INVITE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->group_invite_by))
#define GET_PLAYER_COINS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->coins))
#define GET_PROMPT(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->prompt))
#define GET_FIGHT_PROMPT(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->fight_prompt))
#define GET_SLASH_CHANNELS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->slash_channels))
#define GET_TITLE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->title))
#define GET_OFFERS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->offers))
#define POOFIN(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define REBOOT_CONF(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->reboot_conf))
#define REREAD_EMPIRE_TECH_ON_LOGIN(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->reread_empire_tech_on_login))
#define RESTORE_ON_LOGIN(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->restore_on_login))


// ch->player_specials.saved: player_special_data_saved
#define CAN_GAIN_NEW_SKILLS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.can_gain_new_skills))
#define CAN_GET_BONUS_SKILLS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.can_get_bonus_skills))
#define CREATION_ARCHETYPE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.creation_archetype))
#define GET_ACCOUNT_ID(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.account_id))
#define GET_ACTION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.action))
#define GET_ACTION_ROOM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.action_room))
#define GET_ACTION_CYCLE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.action_cycle))
#define GET_ACTION_TIMER(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.action_timer))
#define GET_ACTION_VNUM(ch, n)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.action_vnum[(n)]))
#define GET_ADMIN_NOTES(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.admin_notes))
#define GET_ADVENTURE_SUMMON_RETURN_LOCATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.adventure_summon_return_location))
#define GET_ADVENTURE_SUMMON_RETURN_MAP(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.adventure_summon_return_map))
#define GET_APPARENT_AGE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.apparent_age))
#define GET_BAD_PWS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_BONUS_TRAITS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bonus_traits))
#define GET_CLASS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.character_class))
#define GET_CLASS_PROGRESSION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.class_progression))
#define GET_CLASS_ROLE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.class_role))
#define GET_COMPUTED_LEVEL(ch)  (GET_SKILL_LEVEL(ch) + GET_GEAR_LEVEL(ch))
#define GET_COND(ch, i)  CHECK_PLAYER_SPECIAL(REAL_CHAR(ch), (REAL_CHAR(ch)->player_specials->saved.conditions[(i)]))
#define GET_CONFUSED_DIR(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.confused_dir))
#define GET_CREATION_HOST(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.creation_host))
#define GET_CUSTOM_COLOR(ch, pos)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.custom_colors[(pos)]))
#define GET_DAILY_CYCLE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.daily_cycle))
#define GET_DISGUISED_NAME(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.disguised_name))
#define GET_DISGUISED_SEX(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.disguised_sex))
#define GET_EXP_TODAY(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.exp_today))
#define GET_FREE_SKILL_RESETS(ch, skill)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[(skill)].resets))
#define GET_GRANT_FLAGS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.grants))
#define GET_IGNORE_LIST(ch, pos)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.ignore_list[(pos)]))
#define GET_IMMORTAL_LEVEL(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.immortal_level))
#define GET_INVIS_LEV(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_LAST_CORPSE_ID(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.last_corpse_id))
#define GET_LAST_DEATH_TIME(ch)  CHECK_PLAYER_SPECIAL(REAL_CHAR(ch), (REAL_CHAR(ch)->player_specials->saved.last_death_time))
#define GET_LAST_DIR(ch)  CHECK_PLAYER_SPECIAL(REAL_CHAR(ch), (REAL_CHAR(ch)->player_specials->saved.last_direction))
#define GET_LAST_KNOWN_LEVEL(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.last_known_level))
#define GET_HIGHEST_KNOWN_LEVEL(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.highest_known_level))
#define GET_LAST_ROOM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.last_room))
#define GET_LAST_TIP(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.last_tip))
#define GET_LOADROOM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_LOAD_ROOM_CHECK(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room_check))
#define GET_EMPIRE_VNUM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.empire))
#define GET_MAPSIZE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.mapsize))
#define GET_MORPH(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.morph))
#define GET_MOUNT_FLAGS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.mount_flags))
#define GET_MOUNT_VNUM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.mount_vnum))
#define GET_OLC_FLAGS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.olc_flags))
#define GET_OLC_MAX_VNUM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.olc_max_vnum))
#define GET_OLC_MIN_VNUM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.olc_min_vnum))
#define GET_PC_CLASS(ch)  (IS_NPC(ch) ? 0 : GET_CLASS(ch))
#define GET_PLEDGE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pledge))
#define GET_PROMO_ID(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.promo_id))
#define GET_RANK(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.rank))
#define GET_RECENT_DEATH_COUNT(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.recent_death_count))
#define GET_REFERRED_BY(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.referred_by))
#define GET_RESOURCE(ch, i)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.resources[i]))
#define GET_REWARDED_TODAY(ch, pos)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.rewarded_today[(pos)]))
#define GET_SKILL(ch, sk)  (IS_NPC(ch) ? 0 : CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[sk].level)))
#define GET_SKILL_EXP(ch, sk)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[sk].exp))
#define GET_SKILL_LEVEL(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skill_level))
#define GET_DAILY_BONUS_EXPERIENCE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.daily_bonus_experience))
#define GET_STORED_SLASH_CHANNEL(ch, pos)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.slash_channels[(pos)]))
#define GET_TOMB_ROOM(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.tomb_room))
#define IS_DISGUISED(ch)  (!IS_NPC(ch) && PLR_FLAGGED((ch), PLR_DISGUISED))
#define NOSKILL_BLOCKED(ch, skill)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[(skill)].noskill))
#define PRF_FLAGS(ch)  CHECK_PLAYER_SPECIAL(REAL_CHAR(ch), (REAL_CHAR(ch)->player_specials->saved.pref))
#define USING_POISON(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.using_poison))

// helpers
#define GET_LEVELS_GAINED_FROM_ABILITY(ch, ab)  ((!IS_NPC(ch) && (ab) >= 0 && (ab) <= NUM_ABILITIES) ? CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.abilities[(ab)].levels_gained) : 0)
#define HAS_ABILITY(ch, ab)  ((!IS_NPC(ch) && (ab) >= 0 && (ab) <= NUM_ABILITIES) ? CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.abilities[(ab)].purchased) : FALSE)
#define HAS_BONUS_TRAIT(ch, flag)  (!IS_NPC(ch) && IS_SET(GET_BONUS_TRAITS(ch), (flag)))
#define IS_AFK(ch)  (!IS_NPC(ch) && (PRF_FLAGGED((ch), PRF_AFK) || ((ch)->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN) >= 10))
#define IS_GRANTED(ch, flag)  (!IS_NPC(ch) && IS_SET(GET_GRANT_FLAGS(ch), (flag)))
#define IS_PVP_FLAGGED(ch)  (!IS_NPC(ch) && (PRF_FLAGGED((ch), PRF_ALLOW_PVP) || get_cooldown_time((ch), COOLDOWN_PVP_FLAG) > 0))
#define IS_WRITING(ch)  (PLR_FLAGGED(ch, PLR_WRITING))
#define MOUNT_FLAGGED(ch, flag)  (!IS_NPC(ch) && IS_SET(GET_MOUNT_FLAGS(ch), (flag)))
#define NOHASSLE(ch)  (!IS_NPC(ch) && IS_IMMORTAL(ch) && PRF_FLAGGED((ch), PRF_NOHASSLE))
#define PLR_FLAGGED(ch, flag)  (!REAL_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag)  (!REAL_NPC(ch) && IS_SET(PRF_FLAGS(ch), (flag)))
#define OLC_FLAGGED(ch, flag)  (!IS_NPC(ch) && IS_SET(GET_OLC_FLAGS(ch), (flag)))

// definitions
#define IS_HOSTILE(ch)  (!IS_NPC(ch) && (get_cooldown_time((ch), COOLDOWN_HOSTILE_FLAG) > 0 || get_cooldown_time((ch), COOLDOWN_ROGUE_FLAG) > 0))
#define IS_HUNGRY(ch)  (GET_COND(ch, FULL) >= 360 && !HAS_ABILITY(ch, ABIL_UNNATURAL_THIRST))
#define IS_DRUNK(ch)  (GET_COND(ch, DRUNK) >= 360)
#define IS_GOD(ch)  (GET_ACCESS_LEVEL(ch) == LVL_GOD)
#define IS_IMMORTAL(ch)  (GET_ACCESS_LEVEL(ch) >= LVL_START_IMM)
#define IS_RIDING(ch)  (!IS_NPC(ch) && GET_MOUNT_VNUM(ch) != NOTHING && MOUNT_FLAGGED(ch, MOUNT_RIDING))
#define IS_THIRSTY(ch)  (GET_COND(ch, THIRST) >= 360 && !HAS_ABILITY(ch, ABIL_UNNATURAL_THIRST) && !HAS_ABILITY(ch, ABIL_SATED_THIRST))
#define IS_BLOOD_STARVED(ch)  (IS_VAMPIRE(ch) && GET_BLOOD(ch) <= config_get_int("blood_starvation_level"))

// for act() and act-like things (requires to_sleeping and is_spammy set to true/false)
#define SENDOK(ch)  (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && (to_sleeping || AWAKE(ch)) && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_NOSPAM) || !is_spammy) && (IS_NPC(ch) || !PLR_FLAGGED((ch), PLR_WRITING)))


 //////////////////////////////////////////////////////////////////////////////
//// ROOM UTILS //////////////////////////////////////////////////////////////

// basic room data
#define GET_ROOM_VNUM(room)  ((room)->vnum)
#define ROOM_AFF_FLAGS(room)  ((room)->affects)
#define ROOM_AFFECTS(room)  ((room)->af)
#define ROOM_BASE_FLAGS(room)  ((room)->base_affects)
#define ROOM_CONTENTS(room)  ((room)->contents)
#define ROOM_DEPLETION(room)  ((room)->depletion)
#define ROOM_LAST_SPAWN_TIME(room)  ((room)->last_spawn_time)
#define ROOM_LIGHTS(room)  ((room)->light)
#define ROOM_ORIGINAL_SECT(room)  ((room)->original_sector)
#define ROOM_OWNER(room)  ((room)->owner)
#define ROOM_PEOPLE(room)  ((room)->people)
#define ROOM_TRACKS(room)  ((room)->tracks)
#define SECT(room)  ((room)->sector_type)


// room->complex data
#define COMPLEX_DATA(room)  ((room)->complex)
#define GET_BUILDING(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->bld_ptr : NULL)
#define GET_ROOM_TEMPLATE(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->rmt_ptr : NULL)
#define BOAT_ROOM(room)  (GET_BOAT(room) ? IN_ROOM(GET_BOAT(room)) : room)
#define BUILDING_BURNING(room)  (COMPLEX_DATA(HOME_ROOM(room)) ? COMPLEX_DATA(HOME_ROOM(room))->burning : 0)
#define BUILDING_DAMAGE(room)  (COMPLEX_DATA(HOME_ROOM(room)) ? COMPLEX_DATA(HOME_ROOM(room))->damage : 0)
#define BUILDING_DISREPAIR(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->disrepair : 0)
#define BUILDING_ENTRANCE(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->entrance : NO_DIR)
#define BUILDING_RESOURCES(room)  (COMPLEX_DATA(room) ? GET_BUILDING_RESOURCES(room) : NULL)
#define GET_BOAT(room)  (COMPLEX_DATA(HOME_ROOM(room)) ? COMPLEX_DATA(HOME_ROOM(room))->boat : NULL)
#define GET_BUILDING_RESOURCES(room)  (COMPLEX_DATA(room)->to_build)
#define GET_INSIDE_ROOMS(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->inside_rooms : 0)
#define HOME_ROOM(room)  ((COMPLEX_DATA(room) && COMPLEX_DATA(room)->home_room) ? COMPLEX_DATA(room)->home_room : (room))
#define IS_COMPLETE(room)  (!BUILDING_RESOURCES(room))
#define ROOM_PATRON(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->patron : NOBODY)
#define ROOM_PRIVATE_OWNER(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->private_owner : NOBODY)
#define ROOM_INSTANCE(room)  (COMPLEX_DATA(room) ? COMPLEX_DATA(room)->instance : NULL)


// exits
#define CAN_GO(ch, ex) (ex->room_ptr && !EXIT_FLAGGED(ex, EX_CLOSED))
#define EXIT_FLAGGED(exit, flag)  (IS_SET((exit)->exit_info, (flag)))

// building data by room
#define BLD_MAX_ROOMS(room)  (GET_BUILDING(HOME_ROOM(room)) ? GET_BLD_EXTRA_ROOMS(GET_BUILDING(HOME_ROOM(room))) : 0)
#define BLD_BASE_AFFECTS(room)  (GET_BUILDING(HOME_ROOM(room)) ? GET_BLD_BASE_AFFECTS(GET_BUILDING(HOME_ROOM(room))) : NOBITS)

// customs
#define ROOM_CUSTOM_NAME(room)  ((room)->name)
#define ROOM_CUSTOM_ICON(room)  ((room)->icon)
#define ROOM_CUSTOM_DESCRIPTION(room)  ((room)->description)

// definitions
#define BLD_ALLOWS_MOUNTS(room)  (ROOM_IS_CLOSED(room) ? (ROOM_BLD_FLAGGED((room), BLD_ALLOW_MOUNTS | BLD_OPEN) || RMT_FLAGGED((room), RMT_OUTDOOR)) : TRUE)
#define CAN_CHOP_ROOM(room)  (has_evolution_type(SECT(room), EVO_CHOPPED_DOWN) || (ROOM_SECT_FLAGGED((room), SECTF_CROP) && ROOM_CROP_FLAGGED((room), CROPF_IS_ORCHARD)))
#define DEPLETION_LIMIT(room)  (ROOM_BLD_FLAGGED((room), BLD_HIGH_DEPLETION) ? config_get_int("high_depletion") : config_get_int("common_depletion"))
#define IS_CITY_CENTER(room)  (BUILDING_VNUM(room) == BUILDING_CITY_CENTER)
#define IS_DARK(room)  (MAGIC_DARKNESS(room) || (!IS_ANY_BUILDING(room) && ROOM_LIGHTS(room) == 0 && (!ROOM_OWNER(room) || !EMPIRE_HAS_TECH(ROOM_OWNER(room), TECH_CITY_LIGHTS)) && !RMT_FLAGGED((room), RMT_LIGHT) && (weather_info.sunlight == SUN_DARK || RMT_FLAGGED((room), RMT_DARK))))
#define IS_LIGHT(room)  (!MAGIC_DARKNESS(room) && WOULD_BE_LIGHT_WITHOUT_MAGIC_DARKNESS(room))
#define IS_REAL_LIGHT(room)  (!MAGIC_DARKNESS(room) && (!IS_DARK(room) || RMT_FLAGGED((room), RMT_LIGHT) || IS_INSIDE(room) || (ROOM_OWNER(room) && IS_ANY_BUILDING(room))))
#define ISLAND_FLAGGED(room, flag)  ((GET_ISLAND_ID(room) != NO_ISLAND) ? IS_SET(get_island(GET_ISLAND_ID(room), TRUE)->flags, (flag)) : FALSE)
#define MAGIC_DARKNESS(room)  (ROOM_AFF_FLAGGED((room), ROOM_AFF_DARK))
#define ROOM_CAN_MINE(room)  (ROOM_SECT_FLAGGED((room), SECTF_CAN_MINE) || ROOM_BLD_FLAGGED((room), BLD_MINE) || (IS_ROAD(room) && SECT_FLAGGED(ROOM_ORIGINAL_SECT(room), SECTF_CAN_MINE)))
#define ROOM_IS_CLOSED(room)  (IS_INSIDE(room) || IS_ADVENTURE_ROOM(room) || (IS_ANY_BUILDING(room) && !ROOM_BLD_FLAGGED(room, BLD_OPEN) && (IS_COMPLETE(room) || ROOM_BLD_FLAGGED(room, BLD_CLOSED))))
#define SHOW_PEOPLE_IN_ROOM(room)  (!ROOM_IS_CLOSED(room) && !ROOM_SECT_FLAGGED(room, SECTF_OBSCURE_VISION))
#define WOULD_BE_LIGHT_WITHOUT_MAGIC_DARKNESS(room)  (!IS_DARK(room) || RMT_FLAGGED((room), RMT_LIGHT) || adjacent_room_is_light(room) || IS_ANY_BUILDING(room))

// extra data
#define ROOM_CROP_TYPE(room)  (ROOM_SECT_FLAGGED(room, SECTF_HAS_CROP_DATA) ? get_room_extra_data((room), ROOM_EXTRA_CROP_TYPE) : NOTHING)

// interaction checks (leading up to CAN_INTERACT_ROOM)
#define BLD_CAN_INTERACT_ROOM(room, type)  (GET_BUILDING(room) && has_interaction(GET_BLD_INTERACTIONS(GET_BUILDING(room)), (type)))
#define CROP_CAN_INTERACT_ROOM(room, type)  (ROOM_CROP_TYPE(room) != NOTHING && has_interaction(GET_CROP_INTERACTIONS(crop_proto(ROOM_CROP_TYPE(room))), (type)))
#define RMT_CAN_INTERACT_ROOM(room, type)  (GET_ROOM_TEMPLATE(room) && has_interaction(GET_RMT_INTERACTIONS(GET_ROOM_TEMPLATE(room)), (type)))
#define SECT_CAN_INTERACT_ROOM(room, type)  has_interaction(GET_SECT_INTERACTIONS(SECT(room)), (type))
#define CAN_INTERACT_ROOM(room, type)  (SECT_CAN_INTERACT_ROOM((room), (type)) || BLD_CAN_INTERACT_ROOM((room), (type)) || RMT_CAN_INTERACT_ROOM((room), (type)) || CROP_CAN_INTERACT_ROOM((room), (type)))

// fundamental types
#define BUILDING_VNUM(room)  (GET_BUILDING(room) ? GET_BLD_VNUM(GET_BUILDING(room)) : NOTHING)
#define ROOM_TEMPLATE_VNUM(room)  (GET_ROOM_TEMPLATE(room) ? GET_RMT_VNUM(GET_ROOM_TEMPLATE(room)) : NOTHING)

// helpers
#define BLD_DESIGNATE_FLAGGED(room, flag)  (GET_BUILDING(HOME_ROOM(room)) && IS_SET(GET_BLD_DESIGNATE_FLAGS(GET_BUILDING(HOME_ROOM(room))), (flag)))
#define RMT_FLAGGED(room, flag)  (GET_ROOM_TEMPLATE(room) && IS_SET(GET_RMT_FLAGS(GET_ROOM_TEMPLATE(room)), (flag)))
#define ROOM_AFF_FLAGGED(r, flag)  (IS_SET(ROOM_AFF_FLAGS(r), (flag)))
#define ROOM_BLD_FLAGGED(room, flag)  (GET_BUILDING(room) && IS_SET(GET_BLD_FLAGS(GET_BUILDING(room)), (flag)))
#define ROOM_CROP_FLAGGED(room, flg)  (ROOM_SECT_FLAGGED(room, SECTF_HAS_CROP_DATA) && CROP_FLAGGED(crop_proto(ROOM_CROP_TYPE(room)), (flg)))
#define ROOM_SECT_FLAGGED(room, flg)  SECT_FLAGGED(SECT(room), (flg))
#define SHIFT_CHAR_DIR(ch, room, dir)  SHIFT_DIR((room), confused_dirs[get_north_for_char(ch)][0][(dir)])
#define SHIFT_DIR(room, dir)  real_shift((room), shift_dir[(dir)][0], shift_dir[(dir)][1])

// island info
extern int GET_ISLAND_ID(room_data *room);	// formerly #define GET_ISLAND_ID(room)  (get_map_location_for(room)->island)
#define SET_ISLAND_ID(room, id)  ((room)->island = id)

// room types
#define IS_ADVENTURE_ROOM(room)  ROOM_SECT_FLAGGED((room), SECTF_ADVENTURE)
#define IS_ANY_BUILDING(room)  ROOM_SECT_FLAGGED((room), SECTF_MAP_BUILDING | SECTF_INSIDE)
#define IS_DISMANTLING(room)  (ROOM_AFF_FLAGGED((room), ROOM_AFF_DISMANTLING))
#define IS_INSIDE(room)  ROOM_SECT_FLAGGED((room), SECTF_INSIDE)
#define IS_MAP_BUILDING(room)  ROOM_SECT_FLAGGED((room), SECTF_MAP_BUILDING)
#define IS_OUTDOOR_TILE(room)  (RMT_FLAGGED((room), RMT_OUTDOOR) || (!IS_ADVENTURE_ROOM(room) && (!IS_ANY_BUILDING(room) || (IS_MAP_BUILDING(room) && ROOM_BLD_FLAGGED((room), BLD_OPEN)))))
#define IS_PUBLIC(room)  (ROOM_AFF_FLAGGED((room), ROOM_AFF_PUBLIC))
#define IS_ROAD(room)  ROOM_SECT_FLAGGED((room), SECTF_IS_ROAD)
#define IS_UNCLAIMABLE(room)  (ROOM_AFF_FLAGGED((room), ROOM_AFF_UNCLAIMABLE))
#define IS_WATER_BUILDING(room)  (ROOM_BLD_FLAGGED((room), BLD_SAIL))


 //////////////////////////////////////////////////////////////////////////////
//// ROOM TEMPLATE UTILS /////////////////////////////////////////////////////

#define GET_RMT_VNUM(rmt)  ((rmt)->vnum)
#define GET_RMT_TITLE(rmt)  ((rmt)->title)
#define GET_RMT_DESC(rmt)  ((rmt)->description)
#define GET_RMT_FLAGS(rmt)  ((rmt)->flags)
#define GET_RMT_BASE_AFFECTS(rmt)  ((rmt)->base_affects)
#define GET_RMT_SPAWNS(rmt)  ((rmt)->spawns)
#define GET_RMT_EX_DESCS(rmt)  ((rmt)->ex_description)
#define GET_RMT_EXITS(rmt)  ((rmt)->exits)
#define GET_RMT_INTERACTIONS(rmt)  ((rmt)->interactions)
#define GET_RMT_SCRIPTS(rmt)  ((rmt)->proto_script)


 //////////////////////////////////////////////////////////////////////////////
//// SECTOR UTILS ////////////////////////////////////////////////////////////

#define GET_SECT_BUILD_FLAGS(sect)  ((sect)->build_flags)
#define GET_SECT_CLIMATE(sect)  ((sect)->climate)
#define GET_SECT_COMMANDS(sect)  ((sect)->commands)
#define GET_SECT_EVOS(sect)  ((sect)->evolution)
#define GET_SECT_FLAGS(sect)  ((sect)->flags)
#define GET_SECT_ICONS(sect)  ((sect)->icons)
#define GET_SECT_INTERACTIONS(sect)  ((sect)->interactions)
#define GET_SECT_MAPOUT(sect)  ((sect)->mapout)
#define GET_SECT_MOVE_LOSS(sect)  ((sect)->movement_loss)
#define GET_SECT_NAME(sect)  ((sect)->name)
#define GET_SECT_ROADSIDE_ICON(sect)  ((sect)->roadside_icon)
#define GET_SECT_SPAWNS(sect)  ((sect)->spawns)
#define GET_SECT_TITLE(sect)  ((sect)->title)
#define GET_SECT_VNUM(sect)  ((sect)->vnum)

// utils
#define IS_WATER_SECT(sct)  SECT_FLAGGED((sct), SECTF_FRESH_WATER | SECTF_OCEAN | SECTF_SHALLOW_WATER)
#define SECT_FLAGGED(sct, flg)  (IS_SET(GET_SECT_FLAGS(sct), (flg)))


 //////////////////////////////////////////////////////////////////////////////
//// STRING UTILS ////////////////////////////////////////////////////////////

#define HSHR(ch)  (MOB_FLAGGED((ch), MOB_PLURAL) ? "their" : (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "his":"her") :"its"))
#define HSSH(ch)  (MOB_FLAGGED((ch), MOB_PLURAL) ? "they" : (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "he" :"she") : "it"))
#define HMHR(ch)  (MOB_FLAGGED((ch), MOB_PLURAL) ? "them" : (GET_SEX(ch) ? (GET_SEX(ch) == SEX_MALE ? "him":"her") : "it"))

#define AN(string)  (strchr("aeiouAEIOU", *string) ? "an" : "a")
#define SANA(obj)  (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")
#define ANA(obj)  (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")

#define LOWER(c)  (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)  (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define NULLSAFE(foo)  ((foo) ? (foo) : "")

#define PLURAL(num)  ((num) != 1 ? "s" : "")

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")


 //////////////////////////////////////////////////////////////////////////////
//// CONST EXTERNS ///////////////////////////////////////////////////////////

extern FILE *logfile;	// comm.c
extern const int shift_dir[][2];	// constants.c
extern struct weather_data weather_info;	// db.c


 //////////////////////////////////////////////////////////////////////////////
//// UTIL FUNCTION PROTOS ////////////////////////////////////////////////////

// log information
#define log  basic_mud_log

// basic functions from utils.c
extern bool any_players_in_room(room_data *room);
extern char *PERS(char_data *ch, char_data *vict, bool real);
extern double diminishing_returns(double val, double scale);
extern int att_max(char_data *ch);
extern int count_bits(bitvector_t bitset);
extern int dice(int number, int size);
extern int number(int from, int to);
extern struct time_info_data *age(char_data *ch);
extern struct time_info_data *mud_time_passed(time_t t2, time_t t1);
extern struct time_info_data *real_time_passed(time_t t2, time_t t1);

// empire utils from utils.c
extern int count_members_online(empire_data *emp);
extern int count_tech(empire_data *emp);

// empire diplomacy utils from utils.c
extern bool char_has_relationship(char_data *ch_a, char_data *ch_b, bitvector_t dipl_bits);
extern bool empire_is_friendly(empire_data *emp, empire_data *enemy);
extern bool empire_is_hostile(empire_data *emp, empire_data *enemy, room_data *loc);
extern bool has_empire_trait(empire_data *emp, room_data *loc, bitvector_t trait);
extern bool has_relationship(empire_data *emp, empire_data *fremp, bitvector_t diplomacy);

// interpreter utils from utils.c
extern char *any_one_arg(char *argument, char *first_arg);
extern char *any_one_word(char *argument, char *first_arg);
void comma_args(char *string, char *arg1, char *arg2);
extern int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
extern int is_abbrev(const char *arg1, const char *arg2);
extern bool is_multiword_abbrev(const char *arg, const char *phrase);
extern int is_number(const char *str);
extern char *one_argument(char *argument, char *first_arg);
extern char *one_word(char *argument, char *first_arg);
extern int reserved_word(char *argument);
extern int search_block(char *arg, const char **list, int exact);
void skip_spaces(char **string);
extern char *two_arguments(char *argument, char *first_arg, char *second_arg);

// permission utils from utils.c
extern bool can_build_or_claim_at_war(char_data *ch, room_data *loc);
extern bool can_use_room(char_data *ch, room_data *room, int mode);
extern bool emp_can_use_room(empire_data *emp, room_data *room, int mode);
extern bool has_permission(char_data *ch, int type);
extern bool has_tech_available(char_data *ch, int tech);
extern bool has_tech_available_room(room_data *room, int tech);
extern bool is_at_war(empire_data *emp);
extern int land_can_claim(empire_data *emp, bool outside_only);

// file utilities from utils.c
extern int get_filename(char *orig_name, char *filename, int mode);
extern int get_line(FILE *fl, char *buf);
extern int touch(const char *path);

// logging functions from utils.c
extern char *room_log_identifier(room_data *room);
void basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void log_to_empire(empire_data *emp, int type, const char *str, ...) __attribute__((format(printf, 3, 4)));
void mortlog(const char *str, ...) __attribute__((format(printf, 1, 2)));
void syslog(bitvector_t type, int level, bool file, const char *str, ...) __attribute__((format(printf, 4, 5)));

// mobile functions from utils.c
extern char *get_mob_name_by_proto(mob_vnum vnum);

// object functions from utils.c
extern char *get_obj_name_by_proto(obj_vnum vnum);
extern obj_data *get_top_object(obj_data *obj);
extern double rate_item(obj_data *obj);

// player functions from utils.c
void command_lag(char_data *ch, int wait_type);
void determine_gear_level(char_data *ch);
extern bool wake_and_stand(char_data *ch);

// resource functions from utils.c
void extract_resources(char_data *ch, Resource list[], bool ground);
void give_resources(char_data *ch, Resource list[], bool split);
extern bool has_resources(char_data *ch, Resource list[], bool ground, bool send_msgs);

// string functions from utils.c
extern bitvector_t asciiflag_conv(char *flag);
extern char *bitv_to_alpha(bitvector_t flags);
extern char *delete_doubledollar(char *string);
extern const char *double_percents(const char *string);
extern bool isname(const char *str, const char *namelist);
extern char *level_range_string(int min, int max, int current);
extern bool multi_isname(const char *arg, const char *namelist);
extern char *CAP(char *txt);
extern char *fname(const char *namelist);
extern char *reverse_strstr(char *haystack, char *needle);
extern char *str_dup(const char *source);
extern char *str_replace(char *search, char *replace, char *subject);
extern char *str_str(char *cs, char *ct);
extern char *strip_color(char *input);
void strip_crlf(char *buffer);
extern char *strtolower(char *str);
extern char *strtoupper(char *str);
extern int count_color_codes(char *string);
extern int count_icon_codes(char *string);
extern bool strchrstr(const char *haystack, const char *needles);
extern int str_cmp(const char *arg1, const char *arg2);
extern int strn_cmp(const char *arg1, const char *arg2, int n);
void prettier_sprintbit(bitvector_t bitvector, const char *names[], char *result);
void prune_crlf(char *txt);
extern const char *skip_filler(char *string);
void sprintbit(bitvector_t vektor, const char *names[], char *result, bool space);
void sprinttype(int type, const char *names[], char *result);

// world functions in utils.c
extern bool check_sunny(room_data *room);
extern bool find_flagged_sect_within_distance_from_char(char_data *ch, bitvector_t with_flags, bitvector_t without_flags, int distance);
extern bool find_flagged_sect_within_distance_from_room(room_data *room, bitvector_t with_flags, bitvector_t without_flags, int distance);
extern bool find_sect_within_distance_from_char(char_data *ch, sector_vnum sect, int distance);
extern bool find_sect_within_distance_from_room(room_data *room, sector_vnum sect, int distance);
extern int compute_distance(room_data *from, room_data *to);
extern int count_adjacent_sectors(room_data *room, sector_vnum sect, bool count_original_sect);
extern int distance_to_nearest_player(room_data *room);
extern int get_direction_to(room_data *from, room_data *to);
extern room_data *get_map_location_for(room_data *room);
extern room_data *real_shift(room_data *origin, int x_shift, int y_shift);
extern room_data *straight_line(room_data *origin, room_data *destination, int iter);
extern sector_data *find_first_matching_sector(bitvector_t with_flags, bitvector_t without_flags);

// misc functions from utils.c
extern unsigned long long microtime(void);

// utils from act.action.c
void cancel_action(char_data *ch);
void start_action(char_data *ch, int type, int timer);

// utils from act.empire.c
extern bool is_in_city_for_empire(room_data *loc, empire_data *emp, bool check_wait, bool *too_soon);

// utils from act.informative.c
extern char *get_obj_desc(obj_data *obj, char_data *ch, int mode);

// utils from limits.c
extern bool can_teleport_to(char_data *ch, room_data *loc, bool check_owner);
void gain_condition(char_data *ch, int condition, int value);

// utils from mapview.c
extern bool adjacent_room_is_light(room_data *room);
void look_at_room_by_loc(char_data *ch, room_data *room, bitvector_t options);
#define look_at_room(ch)  look_at_room_by_loc((ch), IN_ROOM(ch), NOBITS)


 //////////////////////////////////////////////////////////////////////////////
//// MISCELLANEOUS UTILS /////////////////////////////////////////////////////

// for the easy-update system
#define PLAYER_UPDATE_FUNC(name)  void (name)(char_data *ch, bool is_file)

// Group related defines
#define GROUP(ch)  (ch->group)
#define GROUP_LEADER(group)  (group->leader)
#define GROUP_FLAGS(group)  (group->group_flags)

// handy
#define SELF(sub, obj)  ((sub) == (obj))

// this is sort of a config
#define HAPPY_COLOR(cur, base)  (cur < base ? "&r" : (cur > base ? "&g" : "&0"))


// supplementary math
#define ABSOLUTE(x)  ((x < 0) ? ((x) * -1) : (x))


/* undefine MAX and MIN so that our macros are used instead */
#ifdef MAX
#undef MAX
#endif
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b)  ((a) < (b) ? (a) : (b))


/* 
 * npc safeguard:
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
	#define CHECK_PLAYER_SPECIAL(ch, var)  (*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
	#define CHECK_PLAYER_SPECIAL(ch, var)  (var)
#endif


// bounds-safe adding
#define SAFE_ADD(field, amount, min, max, warn)  do { \
		long long _safe_add_old_val = (field);	\
		field += (amount);	\
		if ((amount) > 0 && (field) < _safe_add_old_val) {	\
			(field) = (max);	\
			if (warn) {	\
				log("SYSERR: SAFE_ADD out of bounds at %s:%d.", __FILE__, __LINE__);	\
			}	\
		}	\
		else if ((amount) < 0 && (field) > _safe_add_old_val) {	\
			(field) = (min);	\
			if (warn) {	\
				log("SYSERR: SAFE_ADD out of bounds at %s:%d.", __FILE__, __LINE__);	\
			}	\
		}	\
	} while (0)


// shortcudt for messaging
#define send_config_msg(ch, conf)  msg_to_char((ch), "%s\r\n", config_get_string(conf))


 //////////////////////////////////////////////////////////////////////////////
//// CONSTS FOR UTILS.C //////////////////////////////////////////////////////

// get_filename()
#define CRASH_FILE  0
#define ETEXT_FILE  1
#define ALIAS_FILE  2
#define LORE_FILE  4
#define SCRIPT_VARS_FILE  5
