# Native Python hook signatures

Generated from the native adapters by `tools/generate_hook_inventory.py`.
Arguments appear in callback order; C++ type names identify the native object or value.
Non-const `&` arguments are mutable wrappers: use `.get()` and `.set(value)`.
Native objects are borrowed handles; mutable references are callback-scoped and expire after return.
Null native pointers become `None`. `MAP_ON_BEFORE_CREATE_INSTANCE_SCRIPT` receives a mutable
`InstanceScript*` output reference for its second argument, or `None` when no output location exists.

`None` preserves the native default. Permit callbacks combine boolean results with AND;
handled/override callbacks combine them with OR. This table records native dispatch semantics,
not proof that every event has been observed in the deployed realm.

Map registry entries are published after native data initialization. `MAP_ON_CREATE_MAP` runs
outside the creation mutex exactly once. Existing lookups may return while that notification
is still running; nested map creation and same-map lookup remain available. Map iteration
workers run over pointer snapshots, so newly created maps cannot invalidate active iterators.
Shutdown retains the core's quiesced raw-pointer lifetime model. Finite maps created during
destruction callbacks are drained; trusted scripts that continually recreate maps can prolong
shutdown, just as any unbounded in-process callback can. Callbacks are not forcibly preempted.

## Compatibility and native callsite behavior

- The two `PLAYER_GET_CUSTOM_*ARENA*` names forward on the current arena getters, with
  `(player, slot, result)` arguments; mutate `result` rather than return a number.
- The upstream misspelling `ITEM_ON_GOSSIPE_SELECT` and `_CODE` remains supported.
- Item remove/expiry/quest-accept and creature/gameobject quest-accept results handle the
  script fallback. Their native gameplay callers ignore the result; returning True does not
  prevent item destruction or undo quest acceptance. Use the corresponding `PLAYER_CAN_*`
  permission hooks for actual pre-action decisions.
- Creature/gameobject quest-reward runs after reward application; True handles follow-up dialogue/AI.
- Creature quest-select and quest-complete now dispatch from the validated questgiver requests.
- `PLAYER_ON_BEFORE_INIT_TALENT_FOR_LEVEL` can change the pending talent-point count; its level
  parameter is a snapshot in the current core and does not change the character's level.
- The legacy weapon-dependent aura permit hook runs when the current core selects active damage
  aura contributions for an equipped attack slot (`apply=True`). Removal recomputes the remaining
  active contributions rather than replaying the removed aura with `apply=False`.
- World before-config, after-config, and custom-table callbacks on initial boot occur before scripts
  are loaded. Registered callbacks receive subsequent native events/reloads; initial startup is
  `WORLD_ON_STARTUP` after script activation.

| Hook | Callback arguments | Return semantics |
| --- | --- | --- |
| `ACCOUNT_ON_ACCOUNT_LOGIN` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_ON_BEFORE_ACCOUNT_DELETE` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_ON_LAST_IP_UPDATE` | `(uint32 accountId, std::string ip)` | notification; return ignored |
| `ACCOUNT_ON_FAILED_ACCOUNT_LOGIN` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_ON_EMAIL_CHANGE` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_ON_FAILED_EMAIL_CHANGE` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_ON_PASSWORD_CHANGE` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_ON_FAILED_PASSWORD_CHANGE` | `(uint32 accountId)` | notification; return ignored |
| `ACCOUNT_CAN_ACCOUNT_CREATE_CHARACTER` | `(uint32 accountId, uint8 charRace, uint8 charClass)` | permit; AND; default True |
| `ACHIEVEMENT_SET_REALM_COMPLETED` | `(AchievementEntry const* achievement)` | notification; return ignored |
| `ACHIEVEMENT_IS_COMPLETED_CRITERIA` | `(AchievementMgr* mgr, AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement, CriteriaProgress const* progress)` | permit; AND; default True |
| `ACHIEVEMENT_IS_REALM_COMPLETED` | `(AchievementGlobalMgr const* globalmgr, AchievementEntry const* achievement, SystemTimePoint completionTime)` | permit; AND; default True |
| `ACHIEVEMENT_ON_BEFORE_CHECK_CRITERIA` | `(AchievementMgr* mgr, std::list<AchievementCriteriaEntry const*> const* achievementCriteriaList)` | notification; return ignored |
| `ACHIEVEMENT_CAN_CHECK_CRITERIA` | `(AchievementMgr* mgr, AchievementCriteriaEntry const* achievementCriteria)` | permit; AND; default True |
| `ARENA_CAN_ADD_MEMBER` | `(ArenaTeam* team, ObjectGuid PlayerGuid)` | permit; AND; default True |
| `ARENA_ON_GET_POINTS` | `(ArenaTeam* team, uint32 memberRating, float& points)` | notification; return ignored |
| `ARENA_CAN_SAVE_TO_DB` | `(ArenaTeam* team)` | permit; AND; default True |
| `ARENA_ON_BEFORE_CHECK_WIN_CONDITION` | `(Battleground* const bg )` | permit; AND; default True |
| `ARENA_ON_ARENA_START` | `(Battleground* bg )` | notification; return ignored |
| `ARENA_ON_BEFORE_TEAM_MEMBER_UPDATE` | `(ArenaTeam* team, Player* player, bool won, uint32 opponentMatchmakerRating, int32 matchmakerChange)` | permit; AND; default True |
| `ARENA_CAN_SAVE_ARENA_STATS_FOR_MEMBER` | `(ArenaTeam* team, ObjectGuid playerGuid)` | permit; AND; default True |
| `ARENA_ON_GET_START_PERSONAL_RATING` | `(ArenaTeam* team, ObjectGuid playerGuid, uint32& personalRating)` | notification; return ignored |
| `ARENATEAM_ON_GET_SLOT_BY_TYPE` | `(const uint32 type, uint8& slot)` | notification; return ignored |
| `ARENATEAM_ON_GET_ARENA_POINTS` | `(ArenaTeam* team, float& points)` | notification; return ignored |
| `ARENATEAM_ON_TYPEID_TO_QUEUEID` | `(const BattlegroundTypeId bgTypeId, const uint8 arenaType, uint32& queueTypeID)` | notification; return ignored |
| `ARENATEAM_ON_QUEUEID_TO_ARENA_TYPE` | `(const BattlegroundQueueTypeId bgQueueTypeId, uint8& ArenaType)` | notification; return ignored |
| `ARENATEAM_ON_SET_ARENA_MAX_PLAYERS_PER_TEAM` | `(const uint8 arenaType, uint32& maxPlayerPerTeam)` | notification; return ignored |
| `CREATURE_ON_GOSSIP_HELLO` | `(Player* player, Creature* creature)` | handled/override; OR; default False |
| `CREATURE_ON_GOSSIP_SELECT` | `(Player* player, Creature* creature, uint32 sender, uint32 action)` | handled/override; OR; default False |
| `CREATURE_ON_GOSSIP_SELECT_CODE` | `(Player* player, Creature* creature, uint32 sender, uint32 action, char const* code)` | handled/override; OR; default False |
| `CREATURE_ON_QUEST_ACCEPT` | `(Player* player, Creature* creature, Quest const* quest)` | handled/override; OR; default False |
| `CREATURE_ON_QUEST_SELECT` | `(Player* player, Creature* creature, Quest const* quest)` | handled/override; OR; default False |
| `CREATURE_ON_QUEST_COMPLETE` | `(Player* player, Creature* creature, Quest const* quest)` | handled/override; OR; default False |
| `CREATURE_ON_QUEST_REWARD` | `(Player* player, Creature* creature, Quest const* quest, uint32 opt)` | handled/override; OR; default False |
| `CREATURE_ON_ADD_WORLD` | `(Creature* creature)` | notification; return ignored |
| `CREATURE_ON_REMOVE_WORLD` | `(Creature* creature)` | notification; return ignored |
| `GAMEOBJECT_ON_GOSSIP_HELLO` | `(Player* player, GameObject* go)` | handled/override; OR; default False |
| `GAMEOBJECT_ON_GOSSIP_SELECT` | `(Player* player, GameObject* go, uint32 sender, uint32 action)` | handled/override; OR; default False |
| `GAMEOBJECT_ON_GOSSIP_SELECT_CODE` | `(Player* player, GameObject* go, uint32 sender, uint32 action, char const* code)` | handled/override; OR; default False |
| `GAMEOBJECT_ON_QUEST_ACCEPT` | `(Player* player, GameObject* go, Quest const* quest)` | handled/override; OR; default False |
| `GAMEOBJECT_ON_QUEST_REWARD` | `(Player* player, GameObject* go, Quest const* quest, uint32 opt)` | handled/override; OR; default False |
| `GAMEOBJECT_ON_DESTROYED` | `(GameObject* go, Player* player)` | notification; return ignored |
| `GAMEOBJECT_ON_DAMAGED` | `(GameObject* go, Player* player)` | notification; return ignored |
| `GAMEOBJECT_ON_MODIFY_HEALTH` | `(GameObject* go, Unit* attackerOrHealer, int32& change, SpellInfo const* spellInfo)` | notification; return ignored |
| `GAMEOBJECT_ON_LOOT_STATE_CHANGED` | `(GameObject* go, uint32 state, Unit* unit)` | notification; return ignored |
| `GAMEOBJECT_ON_UPDATE` | `(GameObject* go, uint32 diff)` | notification; return ignored |
| `GAMEOBJECT_ON_ADD_WORLD` | `(GameObject* go)` | notification; return ignored |
| `GAMEOBJECT_ON_REMOVE_WORLD` | `(GameObject* go)` | notification; return ignored |
| `ITEM_ON_QUEST_ACCEPT` | `(Player* player, Item* item, Quest const* quest)` | handled/override; OR; default False |
| `ITEM_ON_USE` | `(Player* player, Item* item, SpellCastTargets const& targets)` | handled/override; OR; default False |
| `ITEM_ON_EXPIRE` | `(Player* player, ItemTemplate const* proto)` | handled/override; OR; default False |
| `ITEM_ON_REMOVE` | `(Player* player, Item* item)` | handled/override; OR; default False |
| `ITEM_ON_CAST_COMBAT_SPELL` | `(Player* player, Unit* victim, SpellInfo const* spellInfo, Item* item)` | permit; AND; default True |
| `ITEM_ON_GOSSIPE_SELECT` | `(Player* player, Item* item, uint32 sender, uint32 action)` | notification; return ignored |
| `ITEM_ON_GOSSIPE_SELECT_CODE` | `(Player* player, Item* item, uint32 sender, uint32 action, char const* code)` | notification; return ignored |
| `MAP_ON_CREATE_MAP` | `(Map* map)` | notification; return ignored |
| `MAP_ON_DESTROY_MAP` | `(Map* map)` | notification; return ignored |
| `MAP_ON_PLAYER_ENTER_ALL` | `(Map* map, Player* player)` | notification; return ignored |
| `MAP_ON_PLAYER_LEAVE_ALL` | `(Map* map, Player* player)` | notification; return ignored |
| `MAP_ON_MAP_UPDATE` | `(Map* map, uint32 diff)` | notification; return ignored |
| `MAP_ON_BEFORE_CREATE_INSTANCE_SCRIPT` | `(InstanceMap* instanceMap, InstanceScript** instanceData, bool load, std::string data, uint32 completedEncounterMask)` | notification; return ignored |
| `MAP_ON_DESTROY_INSTANCE` | `(MapInstanced* mapInstanced, Map* map)` | notification; return ignored |
| `PLAYER_ON_PLAYER_JUST_DIED` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_CALCULATE_TALENTS_POINTS` | `(Player const* player, uint32& talentPointsForLevel)` | notification; return ignored |
| `PLAYER_ON_PLAYER_RELEASED_GHOST` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_SEND_INITIAL_PACKETS_BEFORE_ADD_TO_MAP` | `(Player* player, WorldPacket& data)` | notification; return ignored |
| `PLAYER_ON_BATTLEGROUND_DESERTION` | `(Player* player, BattlegroundDesertionType const desertionType)` | notification; return ignored |
| `PLAYER_ON_PLAYER_COMPLETE_QUEST` | `(Player* player, Quest const* quest_id)` | notification; return ignored |
| `PLAYER_ON_PVP_KILL` | `(Player* killer, Player* killed)` | notification; return ignored |
| `PLAYER_ON_PLAYER_PVP_FLAG_CHANGE` | `(Player* player, bool state)` | notification; return ignored |
| `PLAYER_ON_CREATURE_KILL` | `(Player* killer, Creature* killed)` | notification; return ignored |
| `PLAYER_ON_CREATURE_KILLED_BY_PET` | `(Player* PetOwner, Creature* killed)` | notification; return ignored |
| `PLAYER_ON_PLAYER_KILLED_BY_CREATURE` | `(Creature* killer, Player* killed)` | notification; return ignored |
| `PLAYER_ON_LEVEL_CHANGED` | `(Player* player, uint8 oldlevel)` | notification; return ignored |
| `PLAYER_ON_FREE_TALENT_POINTS_CHANGED` | `(Player* player, uint32 points)` | notification; return ignored |
| `PLAYER_ON_TALENTS_RESET` | `(Player* player, bool noCost)` | notification; return ignored |
| `PLAYER_ON_AFTER_SPEC_SLOT_CHANGED` | `(Player* player, uint8 newSlot)` | notification; return ignored |
| `PLAYER_ON_BEFORE_UPDATE` | `(Player* player, uint32 p_time)` | notification; return ignored |
| `PLAYER_ON_UPDATE` | `(Player* player, uint32 p_time)` | notification; return ignored |
| `PLAYER_ON_MONEY_CHANGED` | `(Player* player, int32& amount)` | notification; return ignored |
| `PLAYER_ON_BEFORE_LOOT_MONEY` | `(Player* player, Loot* loot)` | notification; return ignored |
| `PLAYER_ON_GIVE_EXP` | `(Player* player, uint32& amount, Unit* victim, uint8 xpSource)` | notification; return ignored |
| `PLAYER_ON_REPUTATION_CHANGE` | `(Player* player, uint32 factionID, int32& standing, bool incremental)` | permit; AND; default True |
| `PLAYER_ON_REPUTATION_RANK_CHANGE` | `(Player* player, uint32 factionID, ReputationRank newRank, ReputationRank olRank, bool increased)` | notification; return ignored |
| `PLAYER_ON_LEARN_SPELL` | `(Player* player, uint32 spellID)` | notification; return ignored |
| `PLAYER_ON_FORGOT_SPELL` | `(Player* player, uint32 spellID)` | notification; return ignored |
| `PLAYER_ON_DUEL_REQUEST` | `(Player* target, Player* challenger)` | notification; return ignored |
| `PLAYER_ON_DUEL_START` | `(Player* player1, Player* player2)` | notification; return ignored |
| `PLAYER_ON_DUEL_END` | `(Player* winner, Player* loser, DuelCompleteType type)` | notification; return ignored |
| `PLAYER_ON_BEFORE_SEND_CHAT_MESSAGE` | `(Player* player, uint32& type, uint32& lang, std::string& msg)` | notification; return ignored |
| `PLAYER_ON_EMOTE` | `(Player* player, uint32 emote)` | notification; return ignored |
| `PLAYER_ON_TEXT_EMOTE` | `(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid guid)` | notification; return ignored |
| `PLAYER_ON_SPELL_CAST` | `(Player* player, Spell* spell, bool skipCheck)` | notification; return ignored |
| `PLAYER_ON_LOAD_FROM_DB` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_LOGIN` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_BEFORE_LOGOUT` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_LOGOUT` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_CREATE` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_DELETE` | `(ObjectGuid guid, uint32 accountId)` | notification; return ignored |
| `PLAYER_ON_FAILED_DELETE` | `(ObjectGuid guid, uint32 accountId)` | notification; return ignored |
| `PLAYER_ON_SAVE` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_BIND_TO_INSTANCE` | `(Player* player, Difficulty difficulty, uint32 mapId, bool permanent)` | notification; return ignored |
| `PLAYER_ON_UPDATE_ZONE` | `(Player* player, uint32 newZone, uint32 newArea)` | notification; return ignored |
| `PLAYER_ON_UPDATE_AREA` | `(Player* player, uint32 oldArea, uint32 newArea)` | notification; return ignored |
| `PLAYER_ON_MAP_CHANGED` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_BEFORE_TELEPORT` | `(Player* player, uint32 mapid, float x, float y, float z, float orientation, uint32 options, Unit* target)` | permit; AND; default True |
| `PLAYER_ON_UPDATE_FACTION` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_ADD_TO_BATTLEGROUND` | `(Player* player, Battleground* bg)` | notification; return ignored |
| `PLAYER_ON_QUEUE_RANDOM_DUNGEON` | `(Player* player, uint32 & rDungeonId)` | notification; return ignored |
| `PLAYER_ON_REMOVE_FROM_BATTLEGROUND` | `(Player* player, Battleground* bg)` | notification; return ignored |
| `PLAYER_ON_ACHI_COMPLETE` | `(Player* player, AchievementEntry const* achievement)` | notification; return ignored |
| `PLAYER_ON_BEFORE_ACHI_COMPLETE` | `(Player* player, AchievementEntry const* achievement)` | permit; AND; default True |
| `PLAYER_ON_CRITERIA_PROGRESS` | `(Player* player, AchievementCriteriaEntry const* criteria)` | notification; return ignored |
| `PLAYER_ON_BEFORE_CRITERIA_PROGRESS` | `(Player* player, AchievementCriteriaEntry const* criteria)` | permit; AND; default True |
| `PLAYER_ON_ACHI_SAVE` | `(CharacterDatabaseTransaction trans, Player* player, uint16 achId, CompletedAchievementData achiData)` | notification; return ignored |
| `PLAYER_ON_CRITERIA_SAVE` | `(CharacterDatabaseTransaction trans, Player* player, uint16 achId, CriteriaProgress criteriaData)` | notification; return ignored |
| `PLAYER_ON_GOSSIP_SELECT` | `(Player* player, uint32 menu_id, uint32 sender, uint32 action)` | notification; return ignored |
| `PLAYER_ON_GOSSIP_SELECT_CODE` | `(Player* player, uint32 menu_id, uint32 sender, uint32 action, char const* code)` | notification; return ignored |
| `PLAYER_ON_BEING_CHARMED` | `(Player* player, Unit* charmer, uint32 oldFactionId, uint32 newFactionId)` | notification; return ignored |
| `PLAYER_ON_AFTER_SET_VISIBLE_ITEM_SLOT` | `(Player* player, uint8 slot, Item* item)` | notification; return ignored |
| `PLAYER_ON_AFTER_MOVE_ITEM_FROM_INVENTORY` | `(Player* player, Item* it, uint8 bag, uint8 slot, bool update)` | notification; return ignored |
| `PLAYER_ON_EQUIP` | `(Player* player, Item* it, uint8 bag, uint8 slot, bool update)` | notification; return ignored |
| `PLAYER_ON_PLAYER_JOIN_BG` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_PLAYER_JOIN_ARENA` | `(Player* player)` | notification; return ignored |
| `PLAYER_GET_CUSTOM_GET_ARENA_TEAM_ID` | `(Player* player, uint8 slot, uint32& result)` | notification; return ignored |
| `PLAYER_GET_CUSTOM_ARENA_PERSONAL_RATING` | `(Player* player, uint8 slot, uint32& result)` | notification; return ignored |
| `PLAYER_ON_GET_MAX_PERSONAL_ARENA_RATING_REQUIREMENT` | `(Player const* player, uint32 minSlot, uint32& maxArenaRating)` | notification; return ignored |
| `PLAYER_ON_LOOT_ITEM` | `(Player* player, Item* item, uint32 count, ObjectGuid lootguid)` | notification; return ignored |
| `PLAYER_ON_BEFORE_FILL_QUEST_LOOT_ITEM` | `(Player* player, LootItem& item)` | notification; return ignored |
| `PLAYER_ON_STORE_NEW_ITEM` | `(Player* player, Item* item, uint32 count)` | notification; return ignored |
| `PLAYER_ON_CREATE_ITEM` | `(Player* player, Item* item, uint32 count)` | notification; return ignored |
| `PLAYER_ON_QUEST_REWARD_ITEM` | `(Player* player, Item* item, uint32 count)` | notification; return ignored |
| `PLAYER_CAN_PLACE_AUCTION_BID` | `(Player* player, AuctionEntry* auction)` | permit; AND; default True |
| `PLAYER_ON_GROUP_ROLL_REWARD_ITEM` | `(Player* player, Item* item, uint32 count, RollVote voteType, Roll* roll)` | notification; return ignored |
| `PLAYER_ON_BEFORE_OPEN_ITEM` | `(Player* player, Item* item)` | permit; AND; default True |
| `PLAYER_ON_BEFORE_QUEST_COMPLETE` | `(Player* player, uint32 quest_id)` | permit; AND; default True |
| `PLAYER_ON_QUEST_COMPUTE_EXP` | `(Player* player, Quest const* quest, uint32& xpValue)` | notification; return ignored |
| `PLAYER_ON_BEFORE_DURABILITY_REPAIR` | `(Player* player, ObjectGuid npcGUID, ObjectGuid itemGUID, float&discountMod, uint8 guildBank)` | notification; return ignored |
| `PLAYER_ON_BEFORE_BUY_ITEM_FROM_VENDOR` | `(Player* player, ObjectGuid vendorguid, uint32 vendorslot, uint32& item, uint8 count, uint8 bag, uint8 slot)` | notification; return ignored |
| `PLAYER_ON_BEFORE_STORE_OR_EQUIP_NEW_ITEM` | `(Player* player, uint32 vendorslot, uint32& item, uint8 count, uint8 bag, uint8 slot, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore)` | notification; return ignored |
| `PLAYER_ON_AFTER_STORE_OR_EQUIP_NEW_ITEM` | `(Player* player, uint32 vendorslot, Item* item, uint8 count, uint8 bag, uint8 slot, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore)` | notification; return ignored |
| `PLAYER_ON_AFTER_UPDATE_MAX_POWER` | `(Player* player, Powers& power, float& value)` | notification; return ignored |
| `PLAYER_ON_AFTER_UPDATE_MAX_HEALTH` | `(Player* player, float& value)` | notification; return ignored |
| `PLAYER_ON_BEFORE_UPDATE_ATTACK_POWER_AND_DAMAGE` | `(Player* player, float& level, float& val2, bool ranged)` | notification; return ignored |
| `PLAYER_ON_AFTER_UPDATE_ATTACK_POWER_AND_DAMAGE` | `(Player* player, float& level, float& base_attPower, float& attPowerMod, float& attPowerMultiplier, bool ranged)` | notification; return ignored |
| `PLAYER_ON_BEFORE_INIT_TALENT_FOR_LEVEL` | `(Player* player, uint8& level, uint32& talentPointsForLevel)` | notification; return ignored |
| `PLAYER_ON_FIRST_LOGIN` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_SET_MAX_LEVEL` | `(Player* player, uint32& maxPlayerLevel)` | notification; return ignored |
| `PLAYER_CAN_JOIN_IN_BATTLEGROUND_QUEUE` | `(Player* player, ObjectGuid BattlemasterGuid, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, GroupJoinBattlegroundResult& err)` | permit; AND; default True |
| `PLAYER_SHOULD_BE_REWARDED_WITH_MONEY_INSTEAD_OF_EXP` | `(Player* player)` | handled/override; OR; default False |
| `PLAYER_ON_BEFORE_TEMP_SUMMON_INIT_STATS` | `(Player* player, TempSummon* tempSummon, uint32& duration)` | notification; return ignored |
| `PLAYER_ON_BEFORE_GUARDIAN_INIT_STATS_FOR_LEVEL` | `(Player* player, Guardian* guardian, CreatureTemplate const* cinfo, PetType& petType)` | notification; return ignored |
| `PLAYER_ON_AFTER_GUARDIAN_INIT_STATS_FOR_LEVEL` | `(Player* player, Guardian* guardian)` | notification; return ignored |
| `PLAYER_ON_BEFORE_LOAD_PET_FROM_DB` | `(Player* player, uint32& petentry, uint32& petnumber, bool& current, bool& forceLoadFromDB)` | notification; return ignored |
| `PLAYER_CAN_JOIN_IN_ARENA_QUEUE` | `(Player* player, ObjectGuid BattlemasterGuid, uint8 arenaslot, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, uint8 IsRated, GroupJoinBattlegroundResult& err)` | permit; AND; default True |
| `PLAYER_CAN_BATTLEFIELD_PORT` | `(Player* player, uint8 arenaType, BattlegroundTypeId BGTypeID, uint8 action)` | permit; AND; default True |
| `PLAYER_CAN_GROUP_INVITE` | `(Player* player, std::string& membername)` | permit; AND; default True |
| `PLAYER_CAN_GROUP_ACCEPT` | `(Player* player, Group* group)` | permit; AND; default True |
| `PLAYER_CAN_SELL_ITEM` | `(Player* player, Item* item, Creature* creature)` | permit; AND; default True |
| `PLAYER_CAN_SEND_MAIL` | `(Player* player, ObjectGuid receiverGuid, ObjectGuid mailbox, std::string& subject, std::string& body, uint32 money, uint32 COD, Item* item)` | permit; AND; default True |
| `PLAYER_PETITION_BUY` | `(Player* player, Creature* creature, uint32& charterid, uint32& cost, uint32& type)` | notification; return ignored |
| `PLAYER_PETITION_SHOW_LIST` | `(Player* player, Creature* creature, uint32& CharterEntry, uint32& CharterDispayID, uint32& CharterCost)` | notification; return ignored |
| `PLAYER_ON_REWARD_KILL_REWARDER` | `(Player* player, KillRewarder* rewarder, bool isDungeon, float& rate)` | notification; return ignored |
| `PLAYER_CAN_GIVE_MAIL_REWARD_AT_GIVE_LEVEL` | `(Player* player, uint8 level)` | permit; AND; default True |
| `PLAYER_ON_DELETE_FROM_DB` | `(CharacterDatabaseTransaction trans, uint32 guid)` | notification; return ignored |
| `PLAYER_CAN_REPOP_AT_GRAVEYARD` | `(Player* player)` | permit; AND; default True |
| `PLAYER_ON_PLAYER_IS_CLASS` | `(Player const* player, Classes playerClass, ClassContext context)` | optional decision; default None |
| `PLAYER_ON_GET_MAX_SKILL_VALUE` | `(Player* player, uint32 skill, int32& result, bool IsPure)` | notification; return ignored |
| `PLAYER_ON_PLAYER_HAS_ACTIVE_POWER_TYPE` | `(Player const* player, Powers power)` | handled/override; OR; default False |
| `PLAYER_ON_UPDATE_GATHERING_SKILL` | `(Player* player, uint32 skill_id, uint32 current, uint32 gray, uint32 green, uint32 yellow, uint32& gain)` | notification; return ignored |
| `PLAYER_ON_UPDATE_CRAFTING_SKILL` | `(Player* player, SkillLineAbilityEntry const* skill, uint32 current_level, uint32& gain)` | notification; return ignored |
| `PLAYER_ON_UPDATE_FISHING_SKILL` | `(Player* player, int32 skill, int32 zone_skill, int32 chance, int32 roll)` | permit; AND; default True |
| `PLAYER_CAN_AREA_EXPLORE_AND_OUTDOOR` | `(Player* player)` | permit; AND; default True |
| `PLAYER_ON_VICTIM_REWARD_BEFORE` | `(Player* player, Player* victim, uint32& killer_title, int32& victim_rank)` | notification; return ignored |
| `PLAYER_ON_VICTIM_REWARD_AFTER` | `(Player* player, Player* victim, uint32& killer_title, int32& victim_rank, float& honor_f)` | notification; return ignored |
| `PLAYER_ON_CUSTOM_SCALING_STAT_VALUE_BEFORE` | `(Player* player, ItemTemplate const* proto, uint8 slot, bool apply, uint32& CustomScalingStatValue)` | notification; return ignored |
| `PLAYER_ON_CUSTOM_SCALING_STAT_VALUE` | `(Player* player, ItemTemplate const* proto, uint32& statType, int32& val, uint8 itemProtoStatNumber, uint32 ScalingStatValue, ScalingStatValuesEntry const* ssv)` | notification; return ignored |
| `PLAYER_ON_APPLY_ITEM_MODS_BEFORE` | `(Player* player, uint8 slot, bool apply, uint8 itemProtoStatNumber, uint32 statType, int32& val)` | notification; return ignored |
| `PLAYER_ON_APPLY_ENCHANTMENT_ITEM_MODS_BEFORE` | `(Player* player, Item* item, EnchantmentSlot slot, bool apply, uint32 enchant_spell_id, uint32& enchant_amount)` | notification; return ignored |
| `PLAYER_ON_APPLY_WEAPON_DAMAGE` | `(Player* player, uint8 slot, ItemTemplate const* proto, float& minDamage, float& maxDamage, uint8 damageIndex)` | notification; return ignored |
| `PLAYER_CAN_ARMOR_DAMAGE_MODIFIER` | `(Player* player)` | permit; AND; default True |
| `PLAYER_ON_GET_FERAL_AP_BONUS` | `(Player* player, int32& feral_bonus, int32 dpsMod, ItemTemplate const* proto, ScalingStatValuesEntry const* ssv)` | notification; return ignored |
| `PLAYER_CAN_APPLY_WEAPON_DEPENDENT_AURA_DAMAGE_MOD` | `(Player* player, Item* item, WeaponAttackType attackType, AuraEffect const* aura, bool apply)` | permit; AND; default True |
| `PLAYER_CAN_APPLY_EQUIP_SPELL` | `(Player* player, SpellInfo const* spellInfo, Item* item, bool apply, bool form_change)` | permit; AND; default True |
| `PLAYER_CAN_APPLY_EQUIP_SPELLS_ITEM_SET` | `(Player* player, ItemSetEffect* eff)` | permit; AND; default True |
| `PLAYER_CAN_CAST_ITEM_COMBAT_SPELL` | `(Player* player, Unit* target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, Item* item, ItemTemplate const* proto)` | permit; AND; default True |
| `PLAYER_CAN_CAST_ITEM_USE_SPELL` | `(Player* player, Item* item, SpellCastTargets const& targets, uint8 cast_count, uint32 glyphIndex)` | permit; AND; default True |
| `PLAYER_ON_APPLY_AMMO_BONUSES` | `(Player* player, ItemTemplate const* proto, float& currentAmmoDPS)` | notification; return ignored |
| `PLAYER_CAN_EQUIP_ITEM` | `(Player* player, uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading)` | permit; AND; default True |
| `PLAYER_CAN_UNEQUIP_ITEM` | `(Player* player, uint16 pos, bool swap)` | permit; AND; default True |
| `PLAYER_CAN_USE_ITEM` | `(Player* player, ItemTemplate const* proto, InventoryResult& result)` | permit; AND; default True |
| `PLAYER_CAN_SAVE_EQUIP_NEW_ITEM` | `(Player* player, Item* item, uint16 pos, bool update)` | permit; AND; default True |
| `PLAYER_CAN_APPLY_ENCHANTMENT` | `(Player* player, Item* item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition)` | permit; AND; default True |
| `PLAYER_PASSED_QUEST_KILLED_MONSTER_CREDIT` | `(Player* player, Quest const* qinfo, uint32 entry, uint32 real_entry, ObjectGuid guid)` | permit; AND; default True |
| `PLAYER_CHECK_ITEM_IN_SLOT_AT_LOAD_INVENTORY` | `(Player* player, Item* item, uint8 slot, uint8& err, uint16& dest)` | permit; AND; default True |
| `PLAYER_NOT_AVOID_SATISFY` | `(Player* player, DungeonProgressionRequirements const* ar, uint32 target_map, bool report)` | permit; AND; default True |
| `PLAYER_NOT_VISIBLE_GLOBALLY_FOR` | `(Player* player, Player const* u)` | permit; AND; default True |
| `PLAYER_ON_GET_ARENA_PERSONAL_RATING` | `(Player* player, uint8 slot, uint32& result)` | notification; return ignored |
| `PLAYER_ON_GET_ARENA_TEAM_ID` | `(Player* player, uint8 slot, uint32& result)` | notification; return ignored |
| `PLAYER_ON_IS_FFA_PVP` | `(Player* player, bool& result)` | notification; return ignored |
| `PLAYER_ON_FFA_PVP_STATE_UPDATE` | `(Player* player, bool result)` | notification; return ignored |
| `PLAYER_ON_IS_PVP` | `(Player* player, bool& result)` | notification; return ignored |
| `PLAYER_ON_GET_MAX_SKILL_VALUE_FOR_LEVEL` | `(Player* player, uint16& result)` | notification; return ignored |
| `PLAYER_NOT_SET_ARENA_TEAM_INFO_FIELD` | `(Player* player, uint8 slot, ArenaTeamInfoType type, uint32 value)` | permit; AND; default True |
| `PLAYER_CAN_JOIN_LFG` | `(Player* player, uint8 roles, std::set<uint32>& dungeons, std::string const& comment)` | permit; AND; default True |
| `PLAYER_CAN_ENTER_MAP` | `(Player* player, MapEntry const* entry, InstanceTemplate const* instance, MapDifficulty const* mapDiff, bool loginCheck)` | permit; AND; default True |
| `PLAYER_CAN_INIT_TRADE` | `(Player* player, Player* target)` | permit; AND; default True |
| `PLAYER_CAN_SET_TRADE_ITEM` | `(Player* player, Item* tradedItem, uint8 tradeSlot)` | permit; AND; default True |
| `PLAYER_ON_SET_SERVER_SIDE_VISIBILITY` | `(Player* player, ServerSideVisibilityType& type, AccountTypes& sec)` | notification; return ignored |
| `PLAYER_ON_SET_SERVER_SIDE_VISIBILITY_DETECT` | `(Player* player, ServerSideVisibilityType& type, AccountTypes& sec)` | notification; return ignored |
| `PLAYER_ON_PLAYER_RESURRECT` | `(Player* player, float restore_percent, bool& applySickness)` | notification; return ignored |
| `PLAYER_ON_BEFORE_CHOOSE_GRAVEYARD` | `(Player* player, TeamId teamId, bool nearCorpse, uint32& graveyardOverride)` | notification; return ignored |
| `PLAYER_CAN_PLAYER_USE_CHAT` | `(Player* player, uint32 type, uint32 language, std::string& msg)` | permit; AND; default True |
| `PLAYER_CAN_PLAYER_USE_PRIVATE_CHAT` | `(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver)` | permit; AND; default True |
| `PLAYER_CAN_PLAYER_USE_GROUP_CHAT` | `(Player* player, uint32 type, uint32 language, std::string& msg, Group* group)` | permit; AND; default True |
| `PLAYER_CAN_PLAYER_USE_GUILD_CHAT` | `(Player* player, uint32 type, uint32 language, std::string& msg, Guild* guild)` | permit; AND; default True |
| `PLAYER_CAN_PLAYER_USE_CHANNEL_CHAT` | `(Player* player, uint32 type, uint32 language, std::string& msg, Channel* channel)` | permit; AND; default True |
| `PLAYER_ON_PLAYER_LEARN_TALENTS` | `(Player* player, uint32 talentId, uint32 talentRank, uint32 spellid)` | notification; return ignored |
| `PLAYER_ON_PLAYER_ENTER_COMBAT` | `(Player* player, Unit* enemy)` | notification; return ignored |
| `PLAYER_ON_PLAYER_LEAVE_COMBAT` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_QUEST_ABANDON` | `(Player* player, uint32 questId)` | notification; return ignored |
| `PLAYER_ON_GET_QUEST_RATE` | `(Player* player, float& result)` | notification; return ignored |
| `PLAYER_ON_CAN_PLAYER_FLY_IN_ZONE` | `(Player* player, uint32 mapId, uint32 zoneId, SpellInfo const* bySpell)` | permit; AND; default True |
| `PLAYER_ANTICHEAT_SET_CAN_FLY_BY_SERVER` | `(Player* player, bool apply)` | notification; return ignored |
| `PLAYER_ANTICHEAT_SET_UNDER_ACK_MOUNT` | `(Player* player)` | notification; return ignored |
| `PLAYER_ANTICHEAT_SET_ROOT_ACK_UPD` | `(Player* player)` | notification; return ignored |
| `PLAYER_ANTICHEAT_SET_JUMPING_BY_OPCODE` | `(Player* player, bool jump)` | notification; return ignored |
| `PLAYER_ANTICHEAT_UPDATE_MOVEMENT_INFO` | `(Player* player, MovementInfo const& movementInfo)` | notification; return ignored |
| `PLAYER_ANTICHEAT_HANDLE_DOUBLE_JUMP` | `(Player* player, Unit* mover)` | permit; AND; default True |
| `PLAYER_ANTICHEAT_CHECK_MOVEMENT_INFO` | `(Player* player, MovementInfo const& movementInfo, Unit* mover, bool jump)` | permit; AND; default True |
| `PLAYER_CAN_SEND_ERROR_ALREADY_LOOTED` | `(Player* player)` | permit; AND; default True |
| `PLAYER_ON_AFTER_CREATURE_LOOT` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_AFTER_CREATURE_LOOT_MONEY` | `(Player* player)` | notification; return ignored |
| `PLAYER_ON_CAN_UPDATE_SKILL` | `(Player* player, uint32 skillId)` | permit; AND; default True |
| `PLAYER_ON_BEFORE_UPDATE_SKILL` | `(Player* player, uint32 skillId, uint32& value, uint32 max, uint32 step)` | notification; return ignored |
| `PLAYER_ON_UPDATE_SKILL` | `(Player* player, uint32 skillId, uint32 value, uint32 max, uint32 step, uint32 newValue)` | notification; return ignored |
| `PLAYER_CAN_RESURRECT` | `(Player* player)` | permit; AND; default True |
| `PLAYER_ON_CAN_GIVE_LEVEL` | `(Player* player, uint8 newLevel)` | permit; AND; default True |
| `PLAYER_ON_SEND_LIST_INVENTORY` | `(Player* player, ObjectGuid vendorGuid, uint32& vendorEntry)` | notification; return ignored |
| `PLAYER_ON_GIVE_REPUTATION` | `(Player* player, int32 factionID, float& amount, ReputationSource repSource)` | notification; return ignored |
| `PLAYER_CAN_LEARN_TALENT` | `(Player* player, TalentEntry const* talent, uint32 rank)` | permit; AND; default True |
| `PLAYER_ON_BEFORE_SEND_LOOT` | `(Player* player, ObjectGuid lootGuid, Loot* loot)` | notification; return ignored |
| `PLAYER_CAN_PROGRESS` | `(Player* player)` | permit; AND; default True |
| `PLAYER_CAN_LEARN_PROGRESSION_SPELL` | `(Player* player, uint32 spellID)` | permit; AND; default True |
| `PLAYER_ON_AFTER_UPDATE` | `(Player* player, uint32 p_time)` | notification; return ignored |
| `PLAYER_CAN_LOAD_FROM_DB` | `(ObjectGuid guid, std::string const& name, uint8 race, uint8 classId, uint8 level, uint32 mapId, float x, float y, float z, AccountTypes security, bool isBot)` | permit; AND; default True |
| `PLAYER_ON_UNEQUIP_ITEM` | `(Player* player, Item* it)` | notification; return ignored |
| `PLAYER_CAN_STORE_PROGRESSION_ITEM` | `(Player* player, ItemTemplate const* item)` | permit; AND; default True |
| `PLAYER_CAN_CREATE_PROGRESSION_AUCTION` | `(Player* player, Item* item)` | permit; AND; default True |
| `PLAYER_ON_PLAYER_QUEST_ACCEPT` | `(Player* player, Quest const* quest)` | notification; return ignored |
| `PLAYER_CAN_ACCEPT_QUEST` | `(Player* player, Quest const* quest)` | permit; AND; default True |
| `PLAYER_CAN_PROGRESS_QUEST_OBJECTIVE` | `(Player* player, Quest const* quest)` | permit; AND; default True |
| `PLAYER_CAN_REWARD_QUEST` | `(Player* player, Quest const* quest)` | permit; AND; default True |
| `PLAYER_CAN_SET_PROGRESSION_SKILL` | `(Player* player, uint32 skillId, uint32 value, uint32 max, uint32 step)` | permit; AND; default True |
| `PLAYER_ON_SET_SKILL` | `(Player* player, uint32 skillId, uint32 value, uint32 max, uint32 step, uint32 newValue)` | notification; return ignored |
| `PLAYER_ON_GET_REPUTATION_PRICE_DISCOUNT` | `(Player const* player, Creature const* creature, float& discount) / (Player const* player, FactionTemplateEntry const* factionTemplate, float& discount)` | notification; return ignored |
| `PLAYER_ON_LEARN_TAXI_NODE` | `(Player const* player, uint32 nodeId)` | notification; return ignored |
| `PLAYER_ON_BEFORE_GET_LEVEL_FOR_XP_GAIN` | `(Player const* player, uint8& level)` | notification; return ignored |
| `PLAYER_GET_PROGRESSION_LEVEL_CEILING` | `(Player* player, uint8& ceiling)` | notification; return ignored |
| `PLAYER_GET_PROGRESSION_SKILL_CEILING` | `(Player* player, uint32 skillId, uint32& ceiling)` | notification; return ignored |
| `SERVER_ON_NETWORK_START` | `()` | notification; return ignored |
| `SERVER_ON_NETWORK_STOP` | `()` | notification; return ignored |
| `SERVER_ON_SOCKET_OPEN` | `(std::shared_ptr<WorldSocket> const& socket)` | notification; return ignored |
| `SERVER_ON_SOCKET_CLOSE` | `(std::shared_ptr<WorldSocket> const& socket)` | notification; return ignored |
| `SERVER_CAN_PACKET_SEND` | `(WorldSession* session, WorldPacket const& packet)` | permit; AND; default True |
| `SERVER_CAN_PACKET_RECEIVE` | `(WorldSession* session, WorldPacket const& packet) / (WorldSession* session, WorldPacket& packet)` | permit; AND; default True |
| `SERVER_ON_PACKET_RECEIVED` | `(WorldSession* session, WorldPacket const& packet)` | notification; return ignored |
| `UNIT_ON_HEAL` | `(Unit* healer, Unit* reciever, uint32& gain)` | notification; return ignored |
| `UNIT_ON_DAMAGE` | `(Unit* attacker, Unit* victim, uint32& damage)` | notification; return ignored |
| `UNIT_MODIFY_PERIODIC_DAMAGE_AURAS_TICK` | `(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo)` | notification; return ignored |
| `UNIT_MODIFY_MELEE_DAMAGE` | `(Unit* target, Unit* attacker, uint32& damage)` | notification; return ignored |
| `UNIT_MODIFY_SPELL_DAMAGE_TAKEN` | `(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo)` | notification; return ignored |
| `UNIT_MODIFY_HEAL_RECEIVED` | `(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo)` | notification; return ignored |
| `UNIT_ON_BEFORE_ROLL_MELEE_OUTCOME_AGAINST` | `(Unit const* attacker, Unit const* victim, WeaponAttackType attType, int32& attackerMaxSkillValueForLevel, int32& victimMaxSkillValueForLevel, int32& attackerWeaponSkill, int32& victimDefenseSkill, int32& crit_chance, int32& miss_chance, int32& dodge_chance, int32& parry_chance, int32& block_chance )` | notification; return ignored |
| `UNIT_ON_AURA_APPLY` | `(Unit* unit, Aura* aura)` | notification; return ignored |
| `UNIT_ON_AURA_REMOVE` | `(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode)` | notification; return ignored |
| `UNIT_IF_NORMAL_REACTION` | `(Unit const* unit, Unit const* target, ReputationRank& repRank)` | permit; AND; default True |
| `UNIT_CAN_SET_PHASE_MASK` | `(Unit const* unit, uint32 newPhaseMask, bool update)` | permit; AND; default True |
| `UNIT_IS_CUSTOM_BUILD_VALUES_UPDATE` | `(Unit const* unit, uint8 updateType, ByteBuffer& fieldBuffer, Player const* target, uint16 index)` | handled/override; OR; default False |
| `UNIT_SHOULD_TRACK_VALUES_UPDATE_POS_BY_INDEX` | `(Unit const* unit, uint8 updateType, uint16 index)` | handled/override; OR; default False |
| `UNIT_ON_PATCH_VALUES_UPDATE` | `(Unit const* unit, ByteBuffer& valuesUpdateBuf, BuildValuesCachePosPointers& posPointers, Player* target)` | notification; return ignored |
| `UNIT_ON_UNIT_UPDATE` | `(Unit* unit, uint32 diff)` | notification; return ignored |
| `UNIT_ON_DISPLAYID_CHANGE` | `(Unit* unit, uint32 displayId)` | notification; return ignored |
| `UNIT_ON_UNIT_ENTER_EVADE_MODE` | `(Unit* unit, uint8 evadeReason)` | notification; return ignored |
| `UNIT_ON_UNIT_ENTER_COMBAT` | `(Unit* unit, Unit* victim)` | notification; return ignored |
| `UNIT_ON_UNIT_DEATH` | `(Unit* unit, Unit* killer)` | notification; return ignored |
| `UNIT_ON_UNIT_SET_SHAPESHIFT_FORM` | `(Unit* unit, uint8 form)` | notification; return ignored |
| `UNIT_ON_UNIT_EXIT_COMBAT` | `(Unit* unit)` | notification; return ignored |
| `UNIT_DEAL_DAMAGE` | `(Unit* attacker, Unit* victim, uint32 damage, DamageEffectType damageType)` | last valid return; default damage |
| `WORLD_ON_OPEN_STATE_CHANGE` | `(bool open)` | notification; return ignored |
| `WORLD_ON_AFTER_CONFIG_LOAD` | `(bool reload)` | notification; return ignored |
| `WORLD_ON_LOAD_CUSTOM_DATABASE_TABLE` | `()` | notification; return ignored |
| `WORLD_ON_BEFORE_CONFIG_LOAD` | `(bool reload)` | notification; return ignored |
| `WORLD_ON_MOTD_CHANGE` | `(std::string& newMotd, LocaleConstant& locale)` | notification; return ignored |
| `WORLD_ON_SHUTDOWN_INITIATE` | `(ShutdownExitCode code, ShutdownMask mask)` | notification; return ignored |
| `WORLD_ON_SHUTDOWN_CANCEL` | `()` | notification; return ignored |
| `WORLD_ON_UPDATE` | `(uint32 diff)` | notification; return ignored |
| `WORLD_ON_STARTUP` | `()` | notification; return ignored |
| `WORLD_ON_SHUTDOWN` | `()` | notification; return ignored |
| `WORLD_ON_AFTER_UNLOAD_ALL_MAPS` | `()` | notification; return ignored |
| `WORLD_ON_BEFORE_FINALIZE_PLAYER_WORLD_SESSION` | `(uint32& cacheVersion)` | notification; return ignored |
| `WORLD_ON_BEFORE_WORLD_INITIALIZED` | `()` | notification; return ignored |
