/* ************************************************************************
*   File: handler.h                                       EmpireMUD 2.0b3 *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2015                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

 //////////////////////////////////////////////////////////////////////////////
//// HANDLER CONSTS //////////////////////////////////////////////////////////

// for affect_join()
#define ADD_DURATION	BIT(0)
#define AVG_DURATION	BIT(1)
#define ADD_MODIFIER	BIT(2)
#define AVG_MODIFIER	BIT(3)


// for find_all_dots
#define FIND_INDIV  0
#define FIND_ALL  1
#define FIND_ALLDOT  2


// for generic_find()
#define FIND_CHAR_ROOM		BIT(0)
#define FIND_CHAR_WORLD		BIT(1)
#define FIND_OBJ_INV		BIT(2)
#define FIND_OBJ_ROOM		BIT(3)
#define FIND_OBJ_WORLD		BIT(4)
#define FIND_OBJ_EQUIP		BIT(5)
#define FIND_NO_DARK		BIT(6)


// for the interaction handlers (returns TRUE if the character performs the interaction; FALSE if it aborts)
#define INTERACTION_FUNC(name)	bool (name)(char_data *ch, struct interaction_item *interaction, room_data *inter_room, char_data *inter_mob, obj_data *inter_item)


 //////////////////////////////////////////////////////////////////////////////
//// HANDLER MACROS //////////////////////////////////////////////////////////

#define MATCH_ITEM_NAME(str, obj)  (isname((str), GET_OBJ_KEYWORDS(obj)) || (IS_DRINK_CONTAINER(obj) && GET_DRINK_CONTAINER_CONTENTS(obj) > 0 && isname((str), drinks[GET_DRINK_CONTAINER_TYPE(obj)])))
#define MATCH_CHAR_DISGUISED_NAME(str, ch)  (isname((str), PERS((ch),(ch),FALSE)))
#define MATCH_CHAR_NAME(str, ch)  ((!IS_NPC(ch) && GET_LASTNAME(ch) && isname((str), GET_LASTNAME(ch))) || isname((str), GET_PC_NAME(ch)) || MATCH_CHAR_DISGUISED_NAME(str, ch))
#define MATCH_CHAR_NAME_ROOM(viewer, str, target)  ((IS_DISGUISED(target) && !IS_IMMORTAL(viewer) && !SAME_EMPIRE(viewer, target)) ? MATCH_CHAR_DISGUISED_NAME(str, target) : MATCH_CHAR_NAME(str, target))


 //////////////////////////////////////////////////////////////////////////////
//// handler.c protos ////////////////////////////////////////////////////////

// affect handlers
void affect_from_char(char_data *ch, int type);
void affect_from_char_by_apply(char_data *ch, int type, int apply);
void affect_from_char_by_bitvector(char_data *ch, int type, bitvector_t bits);
void affects_from_char_by_aff_flag(char_data *ch, bitvector_t aff_flag);
void affect_from_room(room_data *room, int type);
void affect_join(char_data *ch, struct affected_type *af, int flags);
void affect_modify(char_data *ch, byte loc, sh_int mod, bitvector_t bitv, bool add);
void affect_remove(char_data *ch, struct affected_type *af);
void affect_remove_room(room_data *room, struct affected_type *af);
void affect_to_char(char_data *ch, struct affected_type *af);
void affect_to_room(room_data *room, struct affected_type *af);
void affect_total(char_data *ch);
extern bool affected_by_spell(char_data *ch, int type);
extern bool affected_by_spell_and_apply(char_data *ch, int type, int apply);
extern struct affected_type *create_aff(int type, int duration, int location, int modifier, bitvector_t bitvector, char_data *cast_by);
void apply_dot_effect(char_data *ch, sh_int type, sh_int duration, sh_int damage_type, sh_int damage, sh_int max_stack, char_data *cast_by);
void dot_remove(char_data *ch, struct over_time_effect_type *dot);
extern bool room_affected_by_spell(room_data *room, int type);

// affect shortcut macros
#define create_flag_aff(type, duration, bit, cast_by)  create_aff((type), (duration), APPLY_NONE, 0, (bit), (cast_by))
#define create_mod_aff(type, duration, loc, mod, cast_by)  create_aff((type), (duration), (loc), (mod), 0, (cast_by))

// character handlers
extern bool char_from_chair(char_data *ch);
extern bool char_to_chair(char_data *ch, obj_data *chair);
void extract_char(char_data *ch);
void extract_char_final(char_data *ch);
void perform_dismount(char_data *ch);
void perform_idle_out(char_data *ch);
void perform_mount(char_data *ch, char_data *mount);

// character location handlers
void char_from_room(char_data *ch);
void char_to_room(char_data *ch, room_data *room);

// character targeting handlers
extern char_data *find_closest_char(char_data *ch, char *arg, bool pc);
extern char_data *find_mob_in_room_by_vnum(room_data *room, mob_vnum vnum);
extern char_data *get_char_room(char *name, room_data *room);
extern char_data *get_char_room_vis(char_data *ch, char *name);
extern char_data *get_char_vis(char_data *ch, char *name, bitvector_t where);
extern char_data *get_player_vis(char_data *ch, char *name, bitvector_t flags);
extern char_data *get_char_world(char *name);

// coin handlers
extern bool can_afford_coins(char_data *ch, empire_data *type, int amount);
void charge_coins(char_data *ch, empire_data *type, int amount);
void cleanup_all_coins();
void cleanup_coins(char_data *ch);
void coin_string(struct coin_data *list, char *storage);
extern obj_data *create_money(empire_data *type, int amount);
#define decrease_coins(ch, emp, amount)  increase_coins(ch, emp, -1 * amount)
extern int exchange_coin_value(int amount, empire_data *convert_from, empire_data *convert_to);
double exchange_rate(empire_data *from, empire_data *to);
extern char *find_coin_arg(char *input, empire_data **emp_found, int *amount_found, bool assume_coins);
extern struct coin_data *find_coin_entry(struct coin_data *list, empire_data *emp);
extern int increase_coins(char_data *ch, empire_data *emp, int amount);
extern const char *money_amount(empire_data *type, int amount);
extern const char *money_desc(empire_data *type, int amount);
extern int total_coins(char_data *ch);

// cooldown handlers
void add_cooldown(char_data *ch, int type, int seconds_duration);
extern int get_cooldown_time(char_data *ch, int type);
void remove_cooldown(char_data *ch, struct cooldown_data *cool);
void remove_cooldown_by_type(char_data *ch, int type);

// empire handlers
void abandon_room(room_data *room);
void claim_room(room_data *room, empire_data *emp);
extern struct empire_political_data *create_relation(empire_data *a, empire_data *b);
extern int find_rank_by_name(empire_data *emp, char *name);
extern struct empire_political_data *find_relation(empire_data *from, empire_data *to);
extern struct empire_territory_data *find_territory_entry(empire_data *emp, room_data *room);
struct empire_trade_data *find_trade_entry(empire_data *emp, int type, obj_vnum vnum);
extern int increase_empire_coins(empire_data *emp_gaining, empire_data *coin_empire, int amount);
#define decrease_empire_coins(emp_gaining, coin_empire, amount)  increase_empire_coins((emp_gaining), (coin_empire), -1 * (amount))
void perform_abandon_room(room_data *room);

// empire targeting handlers
extern struct empire_city_data *find_city(empire_data *emp, room_data *loc);
extern struct empire_city_data *find_city_entry(empire_data *emp, room_data *location);
extern struct empire_city_data *find_city_by_name(empire_data *emp, char *name);
extern struct empire_city_data *find_closest_city(empire_data *emp, room_data *loc);
extern empire_data *get_empire_by_name(char *name);

// follow handlers
void add_follower(char_data *ch, char_data *leader, bool msg);
void stop_follower(char_data *ch);

// group handlers
extern int count_group_members(struct group_data *group);
extern struct group_data *create_group(char_data *leader);
void free_group(struct group_data *group);
extern bool in_same_group(char_data *ch, char_data *vict);
void join_group(char_data *ch, struct group_data *group);
void leave_group(char_data *ch);

// interaction handlers
extern bool check_exclusion_set(struct interact_exclusion_data **set, char code, double percent);
extern struct interact_exclusion_data *find_exclusion_data(struct interact_exclusion_data **set, char code);
void free_exclusion_data(struct interact_exclusion_data *list);
extern bool has_interaction(struct interaction_item *list, int type);
extern bool run_global_mob_interactions(char_data *ch, char_data *mob, int type, INTERACTION_FUNC(*func));
extern bool run_interactions(char_data *ch, struct interaction_item *run_list, int type, room_data *inter_room, char_data *inter_mob, obj_data *inter_item, INTERACTION_FUNC(*func));
extern bool run_room_interactions(char_data *ch, room_data *room, int type, INTERACTION_FUNC(*func));

// lore handlers
void add_lore(char_data *ch, int type, int value);
void remove_lore(char_data *ch, int type, int value);

// mob tagging handlers
extern bool find_id_in_tag_list(int id, struct mob_tag *list);
void expand_mob_tags(char_data *mob);
void free_mob_tags(struct mob_tag **list);
void tag_mob(char_data *mob, char_data *player);

// object handlers
void add_to_object_list(obj_data *obj);
extern obj_data *copy_warehouse_obj(obj_data *input);
void empty_obj_before_extract(obj_data *obj);
void extract_obj(obj_data *obj);
extern obj_data *fresh_copy_obj(obj_data *obj, int scale_level);
extern bool objs_are_identical(obj_data *obj_a, obj_data *obj_b);
void remove_from_object_list(obj_data *obj);

// object binding handlers
void bind_obj_to_group(obj_data *obj, struct group_data *group);
void bind_obj_to_player(obj_data *obj, char_data *ch);
void bind_obj_to_tag_list(obj_data *obj, struct mob_tag *list);
extern bool bind_ok(obj_data *obj, char_data *ch);
void reduce_obj_binding(obj_data *obj, char_data *player);

// object location handlers
void check_obj_in_void(obj_data *obj);
void equip_char(char_data *ch, obj_data *obj, int pos);
void obj_from_char(obj_data *object);
void obj_from_obj(obj_data *obj);
void obj_from_room(obj_data *object);
void object_list_no_owner(obj_data *list);
void obj_to_char(obj_data *object, char_data *ch);
void obj_to_char_or_room(obj_data *obj, char_data *ch);
void obj_to_obj(obj_data *obj, obj_data *obj_to);
void obj_to_room(obj_data *object, room_data *room);
void swap_obj_for_obj(obj_data *old, obj_data *new);
extern obj_data *unequip_char(char_data *ch, int pos);
void unequip_char_to_inventory(char_data *ch, int pos);
void unequip_char_to_room(char_data *ch, int pos);

// object message handlers
extern char *get_custom_message(obj_data *obj, int type);
extern bool has_custom_message(obj_data *obj, int type);

// object targeting handlers
extern obj_data *get_obj_in_equip_vis(char_data *ch, char *arg, obj_data *equipment[]);
extern obj_data *get_obj_in_list_num(int num, obj_data *list);
extern obj_data *get_obj_in_list_vnum(obj_vnum vnum, obj_data *list);
extern obj_data *get_obj_in_list_vis(char_data *ch, char *name, obj_data *list);
extern int get_obj_pos_in_equip_vis(char_data *ch, char *arg, obj_data *equipment[]);
extern obj_vnum get_obj_vnum_by_name(char *name);
extern obj_data *get_obj_vis(char_data *ch, char *name);
extern obj_data *get_object_in_equip_vis(char_data *ch, char *arg, obj_data *equipment[], int *pos);
extern obj_data *get_obj_world(char *name);

// offer handlers
extern struct offer_data *add_offer(char_data *ch, char_data *from, int type, int data);
void remove_offers_by_type(char_data *ch, int type);

// resource depletion handlers
void add_depletion(room_data *room, int type, bool multiple);
extern int get_depletion(room_data *room, int type);
void remove_depletion(room_data *room, int type);

// room handlers
void attach_building_to_room(bld_data *bld, room_data *room);
void attach_template_to_room(room_template *rmt, room_data *room);

// room extra data handlers
void add_to_room_extra_data(room_data *room, int type, int add_value);
extern struct room_extra_data *find_room_extra_data(room_data *room, int type);
extern int get_room_extra_data(room_data *room, int type);
void multiply_room_extra_data(room_data *room, int type, double multiplier);
void remove_room_extra_data(room_data *room, int type);
void set_room_extra_data(room_data *room, int type, int value);

// room targeting handlers
extern room_data *find_target_room(char_data *ch, char *rawroomstr);

// sector handlers
extern bool check_evolution_percent(struct evolution_data *evo);
extern struct evolution_data *get_evolution_by_type(sector_data *st, int type);
extern bool has_evolution_type(sector_data *st, int type);
sector_data *reverse_lookup_evolution_for_sector(sector_data *in_sect, int evo_type);

// storage handlers
void add_to_empire_storage(empire_data *emp, int island, obj_vnum vnum, int amount);
extern bool charge_stored_resource(empire_data *emp, int island, obj_vnum vnum, int amount);
extern bool delete_stored_resource(empire_data *emp, obj_vnum vnum);
extern struct empire_storage_data *find_island_storage_by_keywords(empire_data *emp, int island_id, char *keywords);
extern int find_lowest_storage_loc(obj_data *obj);
extern struct empire_storage_data *find_stored_resource(empire_data *emp, int island, obj_vnum vnum);
extern int get_total_stored_count(empire_data *emp, obj_vnum vnum, bool count_shipping);
extern bool obj_can_be_stored(obj_data *obj, room_data *loc);
extern bool retrieve_resource(char_data *ch, empire_data *emp, struct empire_storage_data *store, bool stolen);
extern int store_resource(char_data *ch, empire_data *emp, obj_data *obj);
extern bool stored_item_requires_withdraw(obj_data *obj);

// targeting handlers
extern int find_all_dots(char *arg);
extern int generic_find(char *arg, bitvector_t bitvector, char_data *ch, char_data **tar_ch, obj_data **tar_obj);
extern int get_number(char **name);

// unique storage handlers
void add_eus_entry(struct empire_unique_storage *eus, empire_data *emp);
extern bool delete_unique_storage_by_vnum(empire_data *emp, obj_vnum vnum);
extern struct empire_unique_storage *find_eus_entry(obj_data *obj, empire_data *emp, room_data *location);
void remove_eus_entry(struct empire_unique_storage *eus, empire_data *emp);
void store_unique_item(char_data *ch, obj_data *obj, empire_data *emp, room_data *room, bool *full);

// world handlers
extern struct room_direction_data *find_exit(room_data *room, int dir);
extern int get_direction_for_char(char_data *ch, int dir);
extern int parse_direction(char_data *ch, char *dir);


 //////////////////////////////////////////////////////////////////////////////
//// handlers from other files ///////////////////////////////////////////////

// act.item.c
void perform_remove(char_data *ch, int pos);

// books.c
extern book_data *book_proto(book_vnum vnum);
extern book_data *find_book_by_author(char *argument, int idnum);
extern book_data *find_book_in_library(char *argument, room_data *room);

// config.c
extern bitvector_t config_get_bitvector(char *key);
extern bool config_get_bool(char *key);
extern double config_get_double(char *key);
extern int config_get_int(char *key);
extern int *config_get_int_array(char *key, int *array_size);
extern const char *config_get_string(char *key);

// fight.c
void appear(char_data *ch);
extern bool can_fight(char_data *ch, char_data *victim);
extern int damage(char_data *ch, char_data *victim, int dam, int attacktype, byte damtype);
void engage_combat(char_data *ch, char_data *vict, bool melee);
void heal(char_data *ch, char_data *vict, int amount);
extern int hit(char_data *ch, char_data *victim, obj_data *weapon, bool normal_round);
extern bool is_fighting(char_data *ch);
void set_fighting(char_data *ch, char_data *victim, byte mode);
void stop_fighting(char_data *ch);
void update_pos(char_data *victim);

// limits.c
extern int limit_crowd_control(char_data *victim, int atype);

// morph.c
extern int get_morph_attack_type(char_data *ch);
extern bool MORPH_FLAGGED(char_data *ch, bitvector_t flag);
void perform_morph(char_data *ch, ubyte form);
