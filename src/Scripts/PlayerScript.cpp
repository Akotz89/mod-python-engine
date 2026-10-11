#include "HookMacros.h"
#include "PythonBorrowed.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Spell.h"
#include "Item.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "AchievementMgr.h"
#include "WorldSession.h"
#include "WorldSocket.h"
#include "Map.h"
#include "Guild.h"
#include "Group.h"
#include "Channel.h"
#include "Pet.h"
#include "ObjectMgr.h"
#include "LootMgr.h"
#include "SpellAuraEffects.h"

class PythonPlayerScript : public PlayerScript
{
public:
    PythonPlayerScript() : PlayerScript("PythonPlayerScript") {}

    void OnPlayerJustDied(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_JUST_DIED, player);
    }

    void OnPlayerCalculateTalentsPoints(Player const* player, uint32& talentPointsForLevel) override
    {
        TRIGGER_PLAYER_HOOK(ON_CALCULATE_TALENTS_POINTS, player, std::ref(talentPointsForLevel));
    }

    void OnPlayerReleasedGhost(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_RELEASED_GHOST, player);
    }

    void OnPlayerSendInitialPacketsBeforeAddToMap(Player* player, WorldPacket& data) override
    {
        TRIGGER_PLAYER_HOOK(ON_SEND_INITIAL_PACKETS_BEFORE_ADD_TO_MAP, player, std::ref(data));
    }

    void OnPlayerBattlegroundDesertion(Player* player, BattlegroundDesertionType const desertionType) override
    {
        TRIGGER_PLAYER_HOOK(ON_BATTLEGROUND_DESERTION, player, desertionType);
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest_id) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_COMPLETE_QUEST, player, quest_id);
    }

    void OnPlayerPVPKill(Player* killer, Player* killed) override
    {
        TRIGGER_PLAYER_HOOK(ON_PVP_KILL, killer, killed);
    }

    void OnPlayerPVPFlagChange(Player* player, bool state) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_PVP_FLAG_CHANGE, player, state);
    }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        TRIGGER_PLAYER_HOOK(ON_CREATURE_KILL, killer, killed);
    }

    void OnPlayerCreatureKilledByPet(Player* PetOwner, Creature* killed) override
    {
        TRIGGER_PLAYER_HOOK(ON_CREATURE_KILLED_BY_PET, PetOwner, killed);
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_KILLED_BY_CREATURE, killer, killed);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        TRIGGER_PLAYER_HOOK(ON_LEVEL_CHANGED, player, oldlevel);
    }

    void OnPlayerFreeTalentPointsChanged(Player* player, uint32 points) override
    {
        TRIGGER_PLAYER_HOOK(ON_FREE_TALENT_POINTS_CHANGED, player, points);
    }

    void OnPlayerTalentsReset(Player* player, bool noCost) override
    {
        TRIGGER_PLAYER_HOOK(ON_TALENTS_RESET, player, noCost);
    }

    void OnPlayerAfterSpecSlotChanged(Player* player, uint8 newSlot) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_SPEC_SLOT_CHANGED, player, newSlot);
    }

    void OnPlayerBeforeUpdate(Player* player, uint32 p_time) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_UPDATE, player, p_time);
    }

    void OnPlayerUpdate(Player* player, uint32 p_time) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE, player, p_time);
    }

    void OnPlayerMoneyChanged(Player* player, int32& amount) override
    {
        TRIGGER_PLAYER_HOOK(ON_MONEY_CHANGED, player, std::ref(amount));
    }

    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_LOOT_MONEY, player, loot);
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        TRIGGER_PLAYER_HOOK(ON_GIVE_EXP, player, std::ref(amount), victim, xpSource);
    }

    bool OnPlayerReputationChange(Player* player, uint32 factionID, int32& standing, bool incremental) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_REPUTATION_CHANGE, true, player, factionID, std::ref(standing), incremental);
    }

    void OnPlayerReputationRankChange(Player* player, uint32 factionID, ReputationRank newRank, ReputationRank olRank, bool increased) override
    {
        TRIGGER_PLAYER_HOOK(ON_REPUTATION_RANK_CHANGE, player, factionID, newRank, olRank, increased);
    }

    void OnPlayerLearnSpell(Player* player, uint32 spellID) override
    {
        TRIGGER_PLAYER_HOOK(ON_LEARN_SPELL, player, spellID);
    }

    void OnPlayerForgotSpell(Player* player, uint32 spellID) override
    {
        TRIGGER_PLAYER_HOOK(ON_FORGOT_SPELL, player, spellID);
    }

    void OnPlayerDuelRequest(Player* target, Player* challenger) override
    {
        TRIGGER_PLAYER_HOOK(ON_DUEL_REQUEST, target, challenger);
    }

    void OnPlayerDuelStart(Player* player1, Player* player2) override
    {
        TRIGGER_PLAYER_HOOK(ON_DUEL_START, player1, player2);
    }

    void OnPlayerDuelEnd(Player* winner, Player* loser, DuelCompleteType type) override
    {
        TRIGGER_PLAYER_HOOK(ON_DUEL_END, winner, loser, type);
    }

    void OnPlayerBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_SEND_CHAT_MESSAGE, player, std::ref(type), std::ref(lang), std::ref(msg));
    }

    void OnPlayerEmote(Player* player, uint32 emote) override
    {
        TRIGGER_PLAYER_HOOK(ON_EMOTE, player, emote);
    }

    void OnPlayerTextEmote(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid guid) override
    {
        TRIGGER_PLAYER_HOOK(ON_TEXT_EMOTE, player, textEmote, emoteNum, guid);
    }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        TRIGGER_PLAYER_HOOK(ON_SPELL_CAST, player, spell, skipCheck);
    }

    void OnPlayerLoadFromDB(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_LOAD_FROM_DB, player);
    }

    void OnPlayerLogin(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_LOGIN, player);
    }

    void OnPlayerBeforeLogout(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_LOGOUT, player);
    }

    void OnPlayerLogout(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_LOGOUT, player);
        if (Py_IsInitialized())
        {
            PyEng::Bridge::GILGuard gil;
            PyEng::Bridge::InvalidateBorrowedPointer(player);
        }
    }

    void OnPlayerCreate(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_CREATE, player);
    }

    void OnPlayerDelete(ObjectGuid guid, uint32 accountId) override
    {
        TRIGGER_PLAYER_HOOK(ON_DELETE, guid, accountId);
    }

    void OnPlayerFailedDelete(ObjectGuid guid, uint32 accountId) override
    {
        TRIGGER_PLAYER_HOOK(ON_FAILED_DELETE, guid, accountId);
    }

    void OnPlayerSave(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_SAVE, player);
    }

    void OnPlayerBindToInstance(Player* player, Difficulty difficulty, uint32 mapId, bool permanent) override
    {
        TRIGGER_PLAYER_HOOK(ON_BIND_TO_INSTANCE, player, difficulty, mapId, permanent);
    }

    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE_ZONE, player, newZone, newArea);
    }

    void OnPlayerUpdateArea(Player* player, uint32 oldArea, uint32 newArea) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE_AREA, player, oldArea, newArea);
    }

    void OnPlayerMapChanged(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_MAP_CHANGED, player);
    }

    bool OnPlayerBeforeTeleport(Player* player, uint32 mapid, float x, float y, float z, float orientation, uint32 options, Unit* target) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_BEFORE_TELEPORT, true, player, mapid, x, y, z, orientation, options, target);
    }

    void OnPlayerUpdateFaction(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE_FACTION, player);
    }

    void OnPlayerAddToBattleground(Player* player, Battleground* bg) override
    {
        TRIGGER_PLAYER_HOOK(ON_ADD_TO_BATTLEGROUND, player, bg);
    }

    void OnPlayerQueueRandomDungeon(Player* player, uint32 & rDungeonId) override
    {
        TRIGGER_PLAYER_HOOK(ON_QUEUE_RANDOM_DUNGEON, player, std::ref(rDungeonId));
    }

    void OnPlayerRemoveFromBattleground(Player* player, Battleground* bg) override
    {
        TRIGGER_PLAYER_HOOK(ON_REMOVE_FROM_BATTLEGROUND, player, bg);
    }

    void OnPlayerAchievementComplete(Player* player, AchievementEntry const* achievement) override
    {
        TRIGGER_PLAYER_HOOK(ON_ACHI_COMPLETE, player, achievement);
    }

    bool OnPlayerBeforeAchievementComplete(Player* player, AchievementEntry const* achievement) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_BEFORE_ACHI_COMPLETE, true, player, achievement);
    }

    void OnPlayerCriteriaProgress(Player* player, AchievementCriteriaEntry const* criteria) override
    {
        TRIGGER_PLAYER_HOOK(ON_CRITERIA_PROGRESS, player, criteria);
    }

    bool OnPlayerBeforeCriteriaProgress(Player* player, AchievementCriteriaEntry const* criteria) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_BEFORE_CRITERIA_PROGRESS, true, player, criteria);
    }

    void OnPlayerAchievementSave(CharacterDatabaseTransaction trans, Player* player, uint16 achId, CompletedAchievementData achiData) override
    {
        TRIGGER_PLAYER_HOOK(ON_ACHI_SAVE, trans, player, achId, achiData);
    }

    void OnPlayerCriteriaSave(CharacterDatabaseTransaction trans, Player* player, uint16 achId, CriteriaProgress criteriaData) override
    {
        TRIGGER_PLAYER_HOOK(ON_CRITERIA_SAVE, trans, player, achId, criteriaData);
    }

    void OnPlayerGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override
    {
        TRIGGER_PLAYER_HOOK(ON_GOSSIP_SELECT, player, menu_id, sender, action);
    }

    void OnPlayerGossipSelectCode(Player* player, uint32 menu_id, uint32 sender, uint32 action, char const* code) override
    {
        TRIGGER_PLAYER_HOOK(ON_GOSSIP_SELECT_CODE, player, menu_id, sender, action, code);
    }

    void OnPlayerBeingCharmed(Player* player, Unit* charmer, uint32 oldFactionId, uint32 newFactionId) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEING_CHARMED, player, charmer, oldFactionId, newFactionId);
    }

    void OnPlayerAfterSetVisibleItemSlot(Player* player, uint8 slot, Item* item) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_SET_VISIBLE_ITEM_SLOT, player, slot, item);
    }

    void OnPlayerAfterMoveItemFromInventory(Player* player, Item* it, uint8 bag, uint8 slot, bool update) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_MOVE_ITEM_FROM_INVENTORY, player, it, bag, slot, update);
    }

    void OnPlayerEquip(Player* player, Item* it, uint8 bag, uint8 slot, bool update) override
    {
        TRIGGER_PLAYER_HOOK(ON_EQUIP, player, it, bag, slot, update);
    }

    void OnPlayerJoinBG(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_JOIN_BG, player);
    }

    void OnPlayerJoinArena(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_JOIN_ARENA, player);
    }

    void OnPlayerGetMaxPersonalArenaRatingRequirement(Player const* player, uint32 minSlot, uint32& maxArenaRating) const override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_MAX_PERSONAL_ARENA_RATING_REQUIREMENT, player, minSlot, std::ref(maxArenaRating));
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid lootguid) override
    {
        TRIGGER_PLAYER_HOOK(ON_LOOT_ITEM, player, item, count, lootguid);
    }

    void OnPlayerBeforeFillQuestLootItem(Player* player, LootItem& item) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_FILL_QUEST_LOOT_ITEM, player, std::ref(item));
    }

    void OnPlayerStoreNewItem(Player* player, Item* item, uint32 count) override
    {
        TRIGGER_PLAYER_HOOK(ON_STORE_NEW_ITEM, player, item, count);
    }

    void OnPlayerCreateItem(Player* player, Item* item, uint32 count) override
    {
        TRIGGER_PLAYER_HOOK(ON_CREATE_ITEM, player, item, count);
    }

    void OnPlayerQuestRewardItem(Player* player, Item* item, uint32 count) override
    {
        TRIGGER_PLAYER_HOOK(ON_QUEST_REWARD_ITEM, player, item, count);
    }

    bool OnPlayerCanPlaceAuctionBid(Player* player, AuctionEntry* auction) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PLACE_AUCTION_BID, true, player, auction);
    }

    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 count, RollVote voteType, Roll* roll) override
    {
        TRIGGER_PLAYER_HOOK(ON_GROUP_ROLL_REWARD_ITEM, player, item, count, voteType, roll);
    }

    bool OnPlayerBeforeOpenItem(Player* player, Item* item) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_BEFORE_OPEN_ITEM, true, player, item);
    }

    bool OnPlayerBeforeQuestComplete(Player* player, uint32 quest_id) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_BEFORE_QUEST_COMPLETE, true, player, quest_id);
    }

    void OnPlayerQuestComputeXP(Player* player, Quest const* quest, uint32& xpValue) override
    {
        TRIGGER_PLAYER_HOOK(ON_QUEST_COMPUTE_EXP, player, quest, std::ref(xpValue));
    }

    void OnPlayerBeforeDurabilityRepair(Player* player, ObjectGuid npcGUID, ObjectGuid itemGUID, float&discountMod, uint8 guildBank) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_DURABILITY_REPAIR, player, npcGUID, itemGUID, std::ref(discountMod), guildBank);
    }

    void OnPlayerBeforeBuyItemFromVendor(Player* player, ObjectGuid vendorguid, uint32 vendorslot, uint32& item, uint8 count, uint8 bag, uint8 slot) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_BUY_ITEM_FROM_VENDOR, player, vendorguid, vendorslot, std::ref(item), count, bag, slot);
    }

    void OnPlayerBeforeStoreOrEquipNewItem(Player* player, uint32 vendorslot, uint32& item, uint8 count, uint8 bag, uint8 slot, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_STORE_OR_EQUIP_NEW_ITEM, player, vendorslot, std::ref(item), count, bag, slot, pProto, pVendor, crItem, bStore);
    }

    void OnPlayerAfterStoreOrEquipNewItem(Player* player, uint32 vendorslot, Item* item, uint8 count, uint8 bag, uint8 slot, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_STORE_OR_EQUIP_NEW_ITEM, player, vendorslot, item, count, bag, slot, pProto, pVendor, crItem, bStore);
    }

    void OnPlayerAfterUpdateMaxPower(Player* player, Powers& power, float& value) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_UPDATE_MAX_POWER, player, std::ref(power), std::ref(value));
    }

    void OnPlayerAfterUpdateMaxHealth(Player* player, float& value) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_UPDATE_MAX_HEALTH, player, std::ref(value));
    }

    void OnPlayerBeforeUpdateAttackPowerAndDamage(Player* player, float& level, float& val2, bool ranged) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_UPDATE_ATTACK_POWER_AND_DAMAGE, player, std::ref(level), std::ref(val2), ranged);
    }

    void OnPlayerAfterUpdateAttackPowerAndDamage(Player* player, float& level, float& base_attPower, float& attPowerMod, float& attPowerMultiplier, bool ranged) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_UPDATE_ATTACK_POWER_AND_DAMAGE, player, std::ref(level), std::ref(base_attPower), std::ref(attPowerMod), std::ref(attPowerMultiplier), ranged);
    }

    void OnPlayerBeforeInitTalentForLevel(Player* player, uint8& level, uint32& talentPointsForLevel) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_INIT_TALENT_FOR_LEVEL, player, std::ref(level), std::ref(talentPointsForLevel));
    }

    void OnPlayerFirstLogin(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_FIRST_LOGIN, player);
    }

    void OnPlayerSetMaxLevel(Player* player, uint32& maxPlayerLevel) override
    {
        TRIGGER_PLAYER_HOOK(ON_SET_MAX_LEVEL, player, std::ref(maxPlayerLevel));
    }

    bool OnPlayerCanJoinInBattlegroundQueue(Player* player, ObjectGuid BattlemasterGuid, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, GroupJoinBattlegroundResult& err) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_JOIN_IN_BATTLEGROUND_QUEUE, true, player, BattlemasterGuid, BGTypeID, joinAsGroup, std::ref(err));
    }

    bool OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(SHOULD_BE_REWARDED_WITH_MONEY_INSTEAD_OF_EXP, false, player);
    }

    void OnPlayerBeforeTempSummonInitStats(Player* player, TempSummon* tempSummon, uint32& duration) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_TEMP_SUMMON_INIT_STATS, player, tempSummon, std::ref(duration));
    }

    void OnPlayerBeforeGuardianInitStatsForLevel(Player* player, Guardian* guardian, CreatureTemplate const* cinfo, PetType& petType) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_GUARDIAN_INIT_STATS_FOR_LEVEL, player, guardian, cinfo, std::ref(petType));
    }

    void OnPlayerAfterGuardianInitStatsForLevel(Player* player, Guardian* guardian) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_GUARDIAN_INIT_STATS_FOR_LEVEL, player, guardian);
    }

    void OnPlayerBeforeLoadPetFromDB(Player* player, uint32& petentry, uint32& petnumber, bool& current, bool& forceLoadFromDB) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_LOAD_PET_FROM_DB, player, std::ref(petentry), std::ref(petnumber), std::ref(current), std::ref(forceLoadFromDB));
    }

    bool OnPlayerCanJoinInArenaQueue(Player* player, ObjectGuid BattlemasterGuid, uint8 arenaslot, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, uint8 IsRated, GroupJoinBattlegroundResult& err) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_JOIN_IN_ARENA_QUEUE, true, player, BattlemasterGuid, arenaslot, BGTypeID, joinAsGroup, IsRated, std::ref(err));
    }

    bool OnPlayerCanBattleFieldPort(Player* player, uint8 arenaType, BattlegroundTypeId BGTypeID, uint8 action) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_BATTLEFIELD_PORT, true, player, arenaType, BGTypeID, action);
    }

    bool OnPlayerCanGroupInvite(Player* player, std::string& membername) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_GROUP_INVITE, true, player, std::ref(membername));
    }

    bool OnPlayerCanGroupAccept(Player* player, Group* group) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_GROUP_ACCEPT, true, player, group);
    }

    bool OnPlayerCanSellItem(Player* player, Item* item, Creature* creature) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_SELL_ITEM, true, player, item, creature);
    }

    bool OnPlayerCanSendMail(Player* player, ObjectGuid receiverGuid, ObjectGuid mailbox, std::string& subject, std::string& body, uint32 money, uint32 COD, Item* item) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_SEND_MAIL, true, player, receiverGuid, mailbox, std::ref(subject), std::ref(body), money, COD, item);
    }

    void OnPlayerPetitionBuy(Player* player, Creature* creature, uint32& charterid, uint32& cost, uint32& type) override
    {
        TRIGGER_PLAYER_HOOK(PETITION_BUY, player, creature, std::ref(charterid), std::ref(cost), std::ref(type));
    }

    void OnPlayerPetitionShowList(Player* player, Creature* creature, uint32& CharterEntry, uint32& CharterDispayID, uint32& CharterCost) override
    {
        TRIGGER_PLAYER_HOOK(PETITION_SHOW_LIST, player, creature, std::ref(CharterEntry), std::ref(CharterDispayID), std::ref(CharterCost));
    }

    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* rewarder, bool isDungeon, float& rate) override
    {
        TRIGGER_PLAYER_HOOK(ON_REWARD_KILL_REWARDER, player, rewarder, isDungeon, std::ref(rate));
    }

    bool OnPlayerCanGiveMailRewardAtGiveLevel(Player* player, uint8 level) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_GIVE_MAIL_REWARD_AT_GIVE_LEVEL, true, player, level);
    }

    void OnPlayerDeleteFromDB(CharacterDatabaseTransaction trans, uint32 guid) override
    {
        TRIGGER_PLAYER_HOOK(ON_DELETE_FROM_DB, trans, guid);
    }

    bool OnPlayerCanRepopAtGraveyard(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_REPOP_AT_GRAVEYARD, true, player);
    }

    Optional<bool> OnPlayerIsClass(Player const* player, Classes playerClass, ClassContext context) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_PLAYER_IS_CLASS, Optional<bool>{}, player, playerClass, context);
    }

    void OnPlayerGetMaxSkillValue(Player* player, uint32 skill, int32& result, bool IsPure) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_MAX_SKILL_VALUE, player, skill, std::ref(result), IsPure);
    }

    bool OnPlayerHasActivePowerType(Player const* player, Powers power) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_PLAYER_HAS_ACTIVE_POWER_TYPE, false, player, power);
    }

    void OnPlayerUpdateGatheringSkill(Player* player, uint32 skill_id, uint32 current, uint32 gray, uint32 green, uint32 yellow, uint32& gain) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE_GATHERING_SKILL, player, skill_id, current, gray, green, yellow, std::ref(gain));
    }

    void OnPlayerUpdateCraftingSkill(Player* player, SkillLineAbilityEntry const* skill, uint32 current_level, uint32& gain) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE_CRAFTING_SKILL, player, skill, current_level, std::ref(gain));
    }

    bool OnPlayerUpdateFishingSkill(Player* player, int32 skill, int32 zone_skill, int32 chance, int32 roll) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_UPDATE_FISHING_SKILL, true, player, skill, zone_skill, chance, roll);
    }

    bool OnPlayerCanAreaExploreAndOutdoor(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_AREA_EXPLORE_AND_OUTDOOR, true, player);
    }

    void OnPlayerVictimRewardBefore(Player* player, Player* victim, uint32& killer_title, int32& victim_rank) override
    {
        TRIGGER_PLAYER_HOOK(ON_VICTIM_REWARD_BEFORE, player, victim, std::ref(killer_title), std::ref(victim_rank));
    }

    void OnPlayerVictimRewardAfter(Player* player, Player* victim, uint32& killer_title, int32& victim_rank, float& honor_f) override
    {
        TRIGGER_PLAYER_HOOK(ON_VICTIM_REWARD_AFTER, player, victim, std::ref(killer_title), std::ref(victim_rank), std::ref(honor_f));
    }

    void OnPlayerCustomScalingStatValueBefore(Player* player, ItemTemplate const* proto, uint8 slot, bool apply, uint32& CustomScalingStatValue) override
    {
        TRIGGER_PLAYER_HOOK(ON_CUSTOM_SCALING_STAT_VALUE_BEFORE, player, proto, slot, apply, std::ref(CustomScalingStatValue));
    }

    void OnPlayerCustomScalingStatValue(Player* player, ItemTemplate const* proto, uint32& statType, int32& val, uint8 itemProtoStatNumber, uint32 ScalingStatValue, ScalingStatValuesEntry const* ssv) override
    {
        TRIGGER_PLAYER_HOOK(ON_CUSTOM_SCALING_STAT_VALUE, player, proto, std::ref(statType), std::ref(val), itemProtoStatNumber, ScalingStatValue, ssv);
    }

    void OnPlayerApplyItemModsBefore(Player* player, uint8 slot, bool apply, uint8 itemProtoStatNumber, uint32 statType, int32& val) override
    {
        TRIGGER_PLAYER_HOOK(ON_APPLY_ITEM_MODS_BEFORE, player, slot, apply, itemProtoStatNumber, statType, std::ref(val));
    }

    void OnPlayerApplyEnchantmentItemModsBefore(Player* player, Item* item, EnchantmentSlot slot, bool apply, uint32 enchant_spell_id, uint32& enchant_amount) override
    {
        TRIGGER_PLAYER_HOOK(ON_APPLY_ENCHANTMENT_ITEM_MODS_BEFORE, player, item, slot, apply, enchant_spell_id, std::ref(enchant_amount));
    }

    void OnPlayerApplyWeaponDamage(Player* player, uint8 slot, ItemTemplate const* proto, float& minDamage, float& maxDamage, uint8 damageIndex) override
    {
        TRIGGER_PLAYER_HOOK(ON_APPLY_WEAPON_DAMAGE, player, slot, proto, std::ref(minDamage), std::ref(maxDamage), damageIndex);
    }

    bool OnPlayerCanArmorDamageModifier(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_ARMOR_DAMAGE_MODIFIER, true, player);
    }

    void OnPlayerGetFeralApBonus(Player* player, int32& feral_bonus, int32 dpsMod, ItemTemplate const* proto, ScalingStatValuesEntry const* ssv) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_FERAL_AP_BONUS, player, std::ref(feral_bonus), dpsMod, proto, ssv);
    }

    bool OnPlayerCanApplyWeaponDependentAuraDamageMod(Player* player, Item* item, WeaponAttackType attackType, AuraEffect const* aura, bool apply) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_APPLY_WEAPON_DEPENDENT_AURA_DAMAGE_MOD, true, player, item, attackType, aura, apply);
    }

    bool OnPlayerCanApplyEquipSpell(Player* player, SpellInfo const* spellInfo, Item* item, bool apply, bool form_change) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_APPLY_EQUIP_SPELL, true, player, spellInfo, item, apply, form_change);
    }

    bool OnPlayerCanApplyEquipSpellsItemSet(Player* player, ItemSetEffect* eff) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_APPLY_EQUIP_SPELLS_ITEM_SET, true, player, eff);
    }

    bool OnPlayerCanCastItemCombatSpell(Player* player, Unit* target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, Item* item, ItemTemplate const* proto) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_CAST_ITEM_COMBAT_SPELL, true, player, target, attType, procVictim, procEx, item, proto);
    }

    bool OnPlayerCanCastItemUseSpell(Player* player, Item* item, SpellCastTargets const& targets, uint8 cast_count, uint32 glyphIndex) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_CAST_ITEM_USE_SPELL, true, player, item, targets, cast_count, glyphIndex);
    }

    void OnPlayerApplyAmmoBonuses(Player* player, ItemTemplate const* proto, float& currentAmmoDPS) override
    {
        TRIGGER_PLAYER_HOOK(ON_APPLY_AMMO_BONUSES, player, proto, std::ref(currentAmmoDPS));
    }

    bool OnPlayerCanEquipItem(Player* player, uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_EQUIP_ITEM, true, player, slot, std::ref(dest), pItem, swap, not_loading);
    }

    bool OnPlayerCanUnequipItem(Player* player, uint16 pos, bool swap) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_UNEQUIP_ITEM, true, player, pos, swap);
    }

    bool OnPlayerCanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& result) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_USE_ITEM, true, player, proto, std::ref(result));
    }

    bool OnPlayerCanSaveEquipNewItem(Player* player, Item* item, uint16 pos, bool update) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_SAVE_EQUIP_NEW_ITEM, true, player, item, pos, update);
    }

    bool OnPlayerCanApplyEnchantment(Player* player, Item* item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_APPLY_ENCHANTMENT, true, player, item, slot, apply, apply_dur, ignore_condition);
    }

    bool OnPlayerPassedQuestKilledMonsterCredit(Player* player, Quest const* qinfo, uint32 entry, uint32 real_entry, ObjectGuid guid) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(PASSED_QUEST_KILLED_MONSTER_CREDIT, true, player, qinfo, entry, real_entry, guid);
    }

    bool OnPlayerCheckItemInSlotAtLoadInventory(Player* player, Item* item, uint8 slot, uint8& err, uint16& dest) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CHECK_ITEM_IN_SLOT_AT_LOAD_INVENTORY, true, player, item, slot, std::ref(err), std::ref(dest));
    }

    bool OnPlayerNotAvoidSatisfy(Player* player, DungeonProgressionRequirements const* ar, uint32 target_map, bool report) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(NOT_AVOID_SATISFY, true, player, ar, target_map, report);
    }

    bool OnPlayerNotVisibleGloballyFor(Player* player, Player const* u) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(NOT_VISIBLE_GLOBALLY_FOR, true, player, u);
    }

    void OnPlayerGetArenaPersonalRating(Player* player, uint8 slot, uint32& result) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_ARENA_PERSONAL_RATING, player, slot, std::ref(result));
        TRIGGER_PLAYER_HOOK(GET_CUSTOM_ARENA_PERSONAL_RATING, player, slot, std::ref(result));
    }

    void OnPlayerGetArenaTeamId(Player* player, uint8 slot, uint32& result) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_ARENA_TEAM_ID, player, slot, std::ref(result));
        TRIGGER_PLAYER_HOOK(GET_CUSTOM_GET_ARENA_TEAM_ID, player, slot, std::ref(result));
    }

    void OnPlayerIsFFAPvP(Player* player, bool& result) override
    {
        TRIGGER_PLAYER_HOOK(ON_IS_FFA_PVP, player, std::ref(result));
    }

    void OnPlayerFfaPvpStateUpdate(Player* player, bool result) override
    {
        TRIGGER_PLAYER_HOOK(ON_FFA_PVP_STATE_UPDATE, player, result);
    }

    void OnPlayerIsPvP(Player* player, bool& result) override
    {
        TRIGGER_PLAYER_HOOK(ON_IS_PVP, player, std::ref(result));
    }

    void OnPlayerGetMaxSkillValueForLevel(Player* player, uint16& result) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_MAX_SKILL_VALUE_FOR_LEVEL, player, std::ref(result));
    }

    bool OnPlayerNotSetArenaTeamInfoField(Player* player, uint8 slot, ArenaTeamInfoType type, uint32 value) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(NOT_SET_ARENA_TEAM_INFO_FIELD, true, player, slot, type, value);
    }

    bool OnPlayerCanJoinLfg(Player* player, uint8 roles, std::set<uint32>& dungeons, std::string const& comment) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_JOIN_LFG, true, player, roles, std::ref(dungeons), comment);
    }

    bool OnPlayerCanEnterMap(Player* player, MapEntry const* entry, InstanceTemplate const* instance, MapDifficulty const* mapDiff, bool loginCheck) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_ENTER_MAP, true, player, entry, instance, mapDiff, loginCheck);
    }

    bool OnPlayerCanInitTrade(Player* player, Player* target) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_INIT_TRADE, true, player, target);
    }

    bool OnPlayerCanSetTradeItem(Player* player, Item* tradedItem, uint8 tradeSlot) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_SET_TRADE_ITEM, true, player, tradedItem, tradeSlot);
    }

    void OnPlayerSetServerSideVisibility(Player* player, ServerSideVisibilityType& type, AccountTypes& sec) override
    {
        TRIGGER_PLAYER_HOOK(ON_SET_SERVER_SIDE_VISIBILITY, player, std::ref(type), std::ref(sec));
    }

    void OnPlayerSetServerSideVisibilityDetect(Player* player, ServerSideVisibilityType& type, AccountTypes& sec) override
    {
        TRIGGER_PLAYER_HOOK(ON_SET_SERVER_SIDE_VISIBILITY_DETECT, player, std::ref(type), std::ref(sec));
    }

    void OnPlayerResurrect(Player* player, float restore_percent, bool& applySickness) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_RESURRECT, player, restore_percent, std::ref(applySickness));
    }

    void OnPlayerBeforeChooseGraveyard(Player* player, TeamId teamId, bool nearCorpse, uint32& graveyardOverride) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_CHOOSE_GRAVEYARD, player, teamId, nearCorpse, std::ref(graveyardOverride));
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PLAYER_USE_CHAT, true, player, type, language, std::ref(msg));
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PLAYER_USE_PRIVATE_CHAT, true, player, type, language, std::ref(msg), receiver);
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Group* group) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PLAYER_USE_GROUP_CHAT, true, player, type, language, std::ref(msg), group);
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Guild* guild) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PLAYER_USE_GUILD_CHAT, true, player, type, language, std::ref(msg), guild);
    }

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Channel* channel) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PLAYER_USE_CHANNEL_CHAT, true, player, type, language, std::ref(msg), channel);
    }

    void OnPlayerLearnTalents(Player* player, uint32 talentId, uint32 talentRank, uint32 spellid) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_LEARN_TALENTS, player, talentId, talentRank, spellid);
    }

    void OnPlayerEnterCombat(Player* player, Unit* enemy) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_ENTER_COMBAT, player, enemy);
    }

    void OnPlayerLeaveCombat(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_LEAVE_COMBAT, player);
    }

    void OnPlayerQuestAbandon(Player* player, uint32 questId) override
    {
        TRIGGER_PLAYER_HOOK(ON_QUEST_ABANDON, player, questId);
    }

    void OnPlayerGetQuestRate(Player* player, float& result) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_QUEST_RATE, player, std::ref(result));
    }

    bool OnPlayerCanFlyInZone(Player* player, uint32 mapId, uint32 zoneId, SpellInfo const* bySpell) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_CAN_PLAYER_FLY_IN_ZONE, true, player, mapId, zoneId, bySpell);
    }

    void AnticheatSetCanFlybyServer(Player* player, bool apply) override
    {
        TRIGGER_PLAYER_HOOK(ANTICHEAT_SET_CAN_FLY_BY_SERVER, player, apply);
    }

    void AnticheatSetUnderACKmount(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ANTICHEAT_SET_UNDER_ACK_MOUNT, player);
    }

    void AnticheatSetRootACKUpd(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ANTICHEAT_SET_ROOT_ACK_UPD, player);
    }

    void AnticheatSetJumpingbyOpcode(Player* player, bool jump) override
    {
        TRIGGER_PLAYER_HOOK(ANTICHEAT_SET_JUMPING_BY_OPCODE, player, jump);
    }

    void AnticheatUpdateMovementInfo(Player* player, MovementInfo const& movementInfo) override
    {
        TRIGGER_PLAYER_HOOK(ANTICHEAT_UPDATE_MOVEMENT_INFO, player, movementInfo);
    }

    bool AnticheatHandleDoubleJump(Player* player, Unit* mover) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ANTICHEAT_HANDLE_DOUBLE_JUMP, true, player, mover);
    }

    bool AnticheatCheckMovementInfo(Player* player, MovementInfo const& movementInfo, Unit* mover, bool jump) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ANTICHEAT_CHECK_MOVEMENT_INFO, true, player, movementInfo, mover, jump);
    }

    bool OnPlayerCanSendErrorAlreadyLooted(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_SEND_ERROR_ALREADY_LOOTED, true, player);
    }

    void OnPlayerAfterCreatureLoot(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_CREATURE_LOOT, player);
    }

    void OnPlayerAfterCreatureLootMoney(Player* player) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_CREATURE_LOOT_MONEY, player);
    }

    bool OnPlayerCanUpdateSkill(Player* player, uint32 skillId) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_CAN_UPDATE_SKILL, true, player, skillId);
    }

    void OnPlayerBeforeUpdateSkill(Player* player, uint32 skillId, uint32& value, uint32 max, uint32 step) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_UPDATE_SKILL, player, skillId, std::ref(value), max, step);
    }

    void OnPlayerUpdateSkill(Player* player, uint32 skillId, uint32 value, uint32 max, uint32 step, uint32 newValue) override
    {
        TRIGGER_PLAYER_HOOK(ON_UPDATE_SKILL, player, skillId, value, max, step, newValue);
    }

    bool OnPlayerCanResurrect(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_RESURRECT, true, player);
    }

    bool OnPlayerCanGiveLevel(Player* player, uint8 newLevel) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(ON_CAN_GIVE_LEVEL, true, player, newLevel);
    }

    void OnPlayerSendListInventory(Player* player, ObjectGuid vendorGuid, uint32& vendorEntry) override
    {
        TRIGGER_PLAYER_HOOK(ON_SEND_LIST_INVENTORY, player, vendorGuid, std::ref(vendorEntry));
    }

    void OnPlayerGiveReputation(Player* player, int32 factionID, float& amount, ReputationSource repSource) override
    {
        TRIGGER_PLAYER_HOOK(ON_GIVE_REPUTATION, player, factionID, std::ref(amount), repSource);
    }

    bool OnPlayerCanLearnTalent(Player* player, TalentEntry const* talent, uint32 rank) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_LEARN_TALENT, true, player, talent, rank);
    }

    void OnPlayerBeforeSendLoot(Player* player, ObjectGuid lootGuid, Loot* loot) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_SEND_LOOT, player, lootGuid, loot);
    }

    bool OnPlayerCanProgress(Player* player) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PROGRESS, true, player);
    }

    bool OnPlayerCanLearnProgressionSpell(Player* player, uint32 spellID) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_LEARN_PROGRESSION_SPELL, true, player, spellID);
    }

    void OnPlayerAfterUpdate(Player* player, uint32 p_time) override
    {
        TRIGGER_PLAYER_HOOK(ON_AFTER_UPDATE, player, p_time);
    }

    bool OnPlayerCanLoadFromDB(ObjectGuid guid, std::string const& name, uint8 race, uint8 classId, uint8 level, uint32 mapId, float x, float y, float z, AccountTypes security, bool isBot) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_LOAD_FROM_DB, true, guid, name, race, classId, level, mapId, x, y, z, security, isBot);
    }

    void OnPlayerUnequip(Player* player, Item* it) override
    {
        TRIGGER_PLAYER_HOOK(ON_UNEQUIP_ITEM, player, it);
    }

    bool OnPlayerCanStoreProgressionItem(Player* player, ItemTemplate const* item) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_STORE_PROGRESSION_ITEM, true, player, item);
    }

    bool OnPlayerCanCreateProgressionAuction(Player* player, Item* item) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_CREATE_PROGRESSION_AUCTION, true, player, item);
    }

    void OnPlayerQuestAccept(Player* player, Quest const* quest) override
    {
        TRIGGER_PLAYER_HOOK(ON_PLAYER_QUEST_ACCEPT, player, quest);
    }

    bool OnPlayerCanAcceptQuest(Player* player, Quest const* quest) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_ACCEPT_QUEST, true, player, quest);
    }

    bool OnPlayerCanProgressQuestObjective(Player* player, Quest const* quest) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_PROGRESS_QUEST_OBJECTIVE, true, player, quest);
    }

    bool OnPlayerCanRewardQuest(Player* player, Quest const* quest) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_REWARD_QUEST, true, player, quest);
    }

    bool OnPlayerCanSetProgressionSkill(Player* player, uint32 skillId, uint32 value, uint32 max, uint32 step) override
    {
        return TRIGGER_PLAYER_HOOK_RESULT(CAN_SET_PROGRESSION_SKILL, true, player, skillId, value, max, step);
    }

    void OnPlayerSetSkill(Player* player, uint32 skillId, uint32 value, uint32 max, uint32 step, uint32 newValue) override
    {
        TRIGGER_PLAYER_HOOK(ON_SET_SKILL, player, skillId, value, max, step, newValue);
    }

    void OnPlayerGetReputationPriceDiscount(Player const* player, Creature const* creature, float& discount) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_REPUTATION_PRICE_DISCOUNT, player, creature, std::ref(discount));
    }

    void OnPlayerGetReputationPriceDiscount(Player const* player, FactionTemplateEntry const* factionTemplate, float& discount) override
    {
        TRIGGER_PLAYER_HOOK(ON_GET_REPUTATION_PRICE_DISCOUNT, player, factionTemplate, std::ref(discount));
    }

    void OnPlayerLearnTaxiNode(Player const* player, uint32 nodeId) override
    {
        TRIGGER_PLAYER_HOOK(ON_LEARN_TAXI_NODE, player, nodeId);
    }

    void OnPlayerBeforeGetLevelForXPGain(Player const* player, uint8& level) override
    {
        TRIGGER_PLAYER_HOOK(ON_BEFORE_GET_LEVEL_FOR_XP_GAIN, player, std::ref(level));
    }

    void OnPlayerGetProgressionLevelCeiling(Player* player, uint8& ceiling) override
    {
        TRIGGER_PLAYER_HOOK(GET_PROGRESSION_LEVEL_CEILING, player, std::ref(ceiling));
    }

    void OnPlayerGetProgressionSkillCeiling(Player* player, uint32 skillId, uint32& ceiling) override
    {
        TRIGGER_PLAYER_HOOK(GET_PROGRESSION_SKILL_CEILING, player, skillId, std::ref(ceiling));
    }
};

void AddSC_PythonPlayerScripts()
{
    new PythonPlayerScript();
}
