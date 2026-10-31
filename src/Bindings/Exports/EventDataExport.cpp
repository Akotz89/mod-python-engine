#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "PythonConverter.h"
#include "WorldPacket.h"
#include "SpellAuraEffects.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "Channel.h"
#include "InstanceScript.h"
#include "AchievementMgr.h"
#include "CreatureData.h"
#include "DBCStructure.h"
#include "AuctionHouseMgr.h"
#include "LootMgr.h"
#include "Group.h"
#include "Player.h"
#include "Item.h"
#include "KillRewarder.h"
#include <limits>
#include <stdexcept>

namespace bp = boost::python;
using namespace PyEng::Bridge;
using BorrowedReturn = BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>;
using CopiedString = BorrowedPolicy<bp::return_value_policy<bp::copy_const_reference>>;
using BorrowedValue = BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>;

namespace PyEng::EventData
{
    void CheckBufferRange(ByteBuffer const& buffer, std::size_t offset, std::size_t count)
    {
        if (offset > buffer.size() || count > buffer.size() - offset)
        {
            PyErr_SetString(PyExc_IndexError, "packet range exceeds buffer size");
            bp::throw_error_already_set();
        }
    }

    bp::object BufferBytes(ByteBuffer const& buffer)
    {
        if (buffer.size() > static_cast<std::size_t>(PY_SSIZE_T_MAX))
            throw std::length_error("packet exceeds Python bytes size");
        return bp::object(bp::handle<>(PyBytes_FromStringAndSize(
            buffer.empty() ? "" : reinterpret_cast<char const*>(buffer.contents()),
            static_cast<Py_ssize_t>(buffer.size()))));
    }

    void AppendBytes(ByteBuffer& buffer, bp::object bytes)
    {
        Py_buffer view;
        if (PyObject_GetBuffer(bytes.ptr(), &view, PyBUF_SIMPLE) < 0)
            bp::throw_error_already_set();
        try
        {
            if (view.len > 0)
                buffer.append(static_cast<uint8 const*>(view.buf), static_cast<std::size_t>(view.len));
        }
        catch (...)
        {
            PyBuffer_Release(&view);
            throw;
        }
        PyBuffer_Release(&view);
    }

    template<typename T> T Read(ByteBuffer& buffer)
    {
        CheckBufferRange(buffer, buffer.rpos(), sizeof(T));
        return buffer.read<T>();
    }

    template<typename T> T Get(ByteBuffer const& buffer, std::size_t offset)
    {
        CheckBufferRange(buffer, offset, sizeof(T));
        return buffer.read<T>(offset);
    }

    template<typename T> void Put(ByteBuffer& buffer, std::size_t offset, T value)
    {
        CheckBufferRange(buffer, offset, sizeof(T));
        buffer.put<T>(offset, value);
    }

    void SetReadPosition(ByteBuffer& buffer, std::size_t position)
    {
        CheckBufferRange(buffer, position, 0);
        buffer.rpos(position);
    }

    template<typename T> bp::class_<T, boost::noncopyable> Record(char const* name, bool gameplayOwned = false)
    {
        bp::class_<T, boost::noncopyable> result(name, bp::no_init);
        RegisterBorrowedType<T>(result, gameplayOwned);
        return result;
    }

    template<typename T, typename Value>
    void ReadField(bp::class_<T, boost::noncopyable>& record, char const* name, Value T::*member)
    {
        record.add_property(name, bp::make_getter(member, BorrowedValue()));
    }
}

#ifndef PYTHON_ENGINE_PACKET_ADAPTER_TEST
void export_event_data_classes()
{
    using namespace PyEng::EventData;
    auto buffer = Record<ByteBuffer>("ByteBuffer");
    buffer.def("Size", &ByteBuffer::size, BorrowedPolicy<>());
    buffer.def("GetBytes", &BufferBytes, BorrowedPolicy<>());
    buffer.def("AppendBytes", &AppendBytes, BorrowedPolicy<>());
    buffer.def("GetReadPosition", static_cast<std::size_t (ByteBuffer::*)() const>(&ByteBuffer::rpos), BorrowedPolicy<>());
    buffer.def("SetReadPosition", &SetReadPosition, BorrowedPolicy<>());
    buffer.def("ReadUInt8", &Read<uint8>, BorrowedPolicy<>());
    buffer.def("ReadUInt16", &Read<uint16>, BorrowedPolicy<>());
    buffer.def("ReadUInt32", &Read<uint32>, BorrowedPolicy<>());
    buffer.def("ReadUInt64", &Read<uint64>, BorrowedPolicy<>());
    buffer.def("ReadFloat", &Read<float>, BorrowedPolicy<>());
    buffer.def("GetUInt8", &Get<uint8>, BorrowedPolicy<>());
    buffer.def("GetUInt16", &Get<uint16>, BorrowedPolicy<>());
    buffer.def("GetUInt32", &Get<uint32>, BorrowedPolicy<>());
    buffer.def("PutUInt8", &Put<uint8>, BorrowedPolicy<>());
    buffer.def("PutUInt16", &Put<uint16>, BorrowedPolicy<>());
    buffer.def("PutUInt32", &Put<uint32>, BorrowedPolicy<>());
    buffer.def("AppendUInt8", +[](ByteBuffer& value, uint8 data) { value.append<uint8>(data); }, BorrowedPolicy<>());
    buffer.def("AppendUInt16", +[](ByteBuffer& value, uint16 data) { value.append<uint16>(data); }, BorrowedPolicy<>());
    buffer.def("AppendUInt32", +[](ByteBuffer& value, uint32 data) { value.append<uint32>(data); }, BorrowedPolicy<>());

    bp::class_<WorldPacket, bp::bases<ByteBuffer>, boost::noncopyable> packet("WorldPacket", bp::no_init);
    RegisterBorrowedType<WorldPacket>(packet);
    packet.def("GetOpcode", &WorldPacket::GetOpcode, BorrowedPolicy<>());
    packet.def("SetOpcode", &WorldPacket::SetOpcode, BorrowedPolicy<>());

    auto effect = Record<AuraEffect>("AuraEffect", true);
    effect.def("GetId", &AuraEffect::GetId, BorrowedPolicy<>());
    effect.def("GetEffIndex", &AuraEffect::GetEffIndex, BorrowedPolicy<>());
    effect.def("GetAmount", &AuraEffect::GetAmount, BorrowedPolicy<>());
    effect.def("SetAmount", &AuraEffect::SetAmount, BorrowedPolicy<>());
    effect.def("GetMiscValue", &AuraEffect::GetMiscValue, BorrowedPolicy<>());
    effect.def("GetCaster", &AuraEffect::GetCaster, BorrowedReturn());
    effect.def("GetBase", &AuraEffect::GetBase, BorrowedReturn());
    effect.def("GetSpellInfo", &AuraEffect::GetSpellInfo, BorrowedReturn());

    auto arena = Record<ArenaTeam>("ArenaTeam", true);
    arena.def("GetId", &ArenaTeam::GetId, BorrowedPolicy<>());
    arena.def("GetType", &ArenaTeam::GetType, BorrowedPolicy<>());
    arena.def("GetName", &ArenaTeam::GetName, CopiedString());
    arena.def("GetCaptain", &ArenaTeam::GetCaptain, BorrowedPolicy<>());
    arena.def("GetRating", &ArenaTeam::GetRating, BorrowedPolicy<>());

    auto battleground = Record<Battleground>("Battleground", true);
    battleground.def("GetName", &Battleground::GetName, BorrowedPolicy<>());
    battleground.def("GetMapId", &Battleground::GetMapId, BorrowedPolicy<>());
    battleground.def("GetInstanceID", &Battleground::GetInstanceID, BorrowedPolicy<>());
    battleground.def("GetPlayersSize", &Battleground::GetPlayersSize, BorrowedPolicy<>());
    battleground.def("GetStatus", +[](Battleground const& value) { return static_cast<uint32>(value.GetStatus()); }, BorrowedPolicy<>());
    battleground.def("GetWinner", +[](Battleground const& value) { return static_cast<uint32>(value.GetWinner()); }, BorrowedPolicy<>());

    auto channel = Record<Channel>("Channel", true);
    channel.def("GetName", &Channel::GetName, CopiedString());
    channel.def("GetChannelId", &Channel::GetChannelId, BorrowedPolicy<>());
    channel.def("GetNumPlayers", &Channel::GetNumPlayers, BorrowedPolicy<>());
    channel.def("GetFlags", &Channel::GetFlags, BorrowedPolicy<>());
    channel.def("IsLFG", &Channel::IsLFG, BorrowedPolicy<>());

    auto instance = Record<InstanceScript>("InstanceScript", true);
    instance.def("GetMap", +[](InstanceScript const& value) { return value.instance; }, BorrowedReturn());
    instance.def("GetBossState", +[](InstanceScript const& value, uint32 id) { return static_cast<uint32>(value.GetBossState(id)); }, BorrowedPolicy<>());
    instance.def("SetBossState", +[](InstanceScript& value, uint32 id, uint32 state)
    {
        if (state > TO_BE_DECIDED) throw std::invalid_argument("invalid encounter state");
        return value.SetBossState(id, static_cast<EncounterState>(state));
    }, BorrowedPolicy<>());
    instance.def("GetData", +[](InstanceScript const& value, uint32 id) { return value.GetData(id); }, BorrowedPolicy<>());
    instance.def("SetData", +[](InstanceScript& value, uint32 id, uint32 data) { value.SetData(id, data); }, BorrowedPolicy<>());

    auto achievement = Record<AchievementEntry>("AchievementEntry");
    ReadField(achievement, "ID", &AchievementEntry::ID);
    ReadField(achievement, "requiredFaction", &AchievementEntry::requiredFaction);
    ReadField(achievement, "mapID", &AchievementEntry::mapID);
    ReadField(achievement, "categoryId", &AchievementEntry::categoryId);
    ReadField(achievement, "points", &AchievementEntry::points);
    ReadField(achievement, "flags", &AchievementEntry::flags);
    ReadField(achievement, "count", &AchievementEntry::count);
    ReadField(achievement, "refAchievement", &AchievementEntry::refAchievement);
    auto criteria = Record<AchievementCriteriaEntry>("AchievementCriteriaEntry");
    ReadField(criteria, "ID", &AchievementCriteriaEntry::ID);
    ReadField(criteria, "referredAchievement", &AchievementCriteriaEntry::referredAchievement);
    ReadField(criteria, "requiredType", &AchievementCriteriaEntry::requiredType);
    ReadField(criteria, "flags", &AchievementCriteriaEntry::flags);
    ReadField(criteria, "timeLimit", &AchievementCriteriaEntry::timeLimit);
    criteria.add_property("asset", bp::make_function(+[](AchievementCriteriaEntry const& value) { return value.raw.field3; }, BorrowedPolicy<>()));
    criteria.add_property("count", bp::make_function(+[](AchievementCriteriaEntry const& value) { return value.raw.count; }, BorrowedPolicy<>()));
    auto progress = Record<CriteriaProgress>("CriteriaProgress", true);
    ReadField(progress, "counter", &CriteriaProgress::counter);
    ReadField(progress, "date", &CriteriaProgress::date);
    ReadField(progress, "changed", &CriteriaProgress::changed);
    auto manager = Record<AchievementMgr>("AchievementMgr", true);
    manager.def("GetPlayer", &AchievementMgr::GetPlayer, BorrowedReturn());
    manager.def("HasAchieved", &AchievementMgr::HasAchieved, BorrowedPolicy<>());
    manager.def("GetCriteriaProgress", &AchievementMgr::GetCriteriaProgress, BorrowedReturn());
    auto globalManager = Record<AchievementGlobalMgr>("AchievementGlobalMgr", true);
    globalManager.def("IsRealmCompleted", &AchievementGlobalMgr::IsRealmCompleted, BorrowedPolicy<>());
    globalManager.def("GetAchievement", &AchievementGlobalMgr::GetAchievement, BorrowedReturn());

    auto creature = Record<CreatureTemplate>("CreatureTemplate");
    ReadField(creature, "Entry", &CreatureTemplate::Entry);
    ReadField(creature, "Name", &CreatureTemplate::Name);
    ReadField(creature, "minlevel", &CreatureTemplate::minlevel);
    ReadField(creature, "maxlevel", &CreatureTemplate::maxlevel);
    ReadField(creature, "faction", &CreatureTemplate::faction);
    ReadField(creature, "npcflag", &CreatureTemplate::npcflag);
    ReadField(creature, "lootid", &CreatureTemplate::lootid);
    auto faction = Record<FactionTemplateEntry>("FactionTemplateEntry");
    ReadField(faction, "ID", &FactionTemplateEntry::ID);
    ReadField(faction, "faction", &FactionTemplateEntry::faction);
    ReadField(faction, "factionFlags", &FactionTemplateEntry::factionFlags);
    ReadField(faction, "ourMask", &FactionTemplateEntry::ourMask);
    ReadField(faction, "friendlyMask", &FactionTemplateEntry::friendlyMask);
    ReadField(faction, "hostileMask", &FactionTemplateEntry::hostileMask);
    faction.def("IsFriendlyTo", &FactionTemplateEntry::IsFriendlyTo, BorrowedPolicy<>());
    faction.def("IsHostileTo", &FactionTemplateEntry::IsHostileTo, BorrowedPolicy<>());

    auto map = Record<MapEntry>("MapEntry");
    ReadField(map, "MapID", &MapEntry::MapID);
    ReadField(map, "map_type", &MapEntry::map_type);
    ReadField(map, "Flags", &MapEntry::Flags);
    ReadField(map, "expansionID", &MapEntry::expansionID);
    ReadField(map, "maxPlayers", &MapEntry::maxPlayers);
    map.def("IsDungeon", &MapEntry::IsDungeon, BorrowedPolicy<>());
    map.def("IsRaid", &MapEntry::IsRaid, BorrowedPolicy<>());
    auto difficulty = Record<MapDifficulty>("MapDifficulty");
    ReadField(difficulty, "resetTime", &MapDifficulty::resetTime);
    ReadField(difficulty, "maxPlayers", &MapDifficulty::maxPlayers);
    ReadField(difficulty, "hasErrorMessage", &MapDifficulty::hasErrorMessage);
    auto instanceTemplate = Record<InstanceTemplate>("InstanceTemplate");
    ReadField(instanceTemplate, "Parent", &InstanceTemplate::Parent);
    ReadField(instanceTemplate, "ScriptId", &InstanceTemplate::ScriptId);
    ReadField(instanceTemplate, "AllowMount", &InstanceTemplate::AllowMount);
    auto requirements = Record<DungeonProgressionRequirements>("DungeonProgressionRequirements");
    ReadField(requirements, "levelMin", &DungeonProgressionRequirements::levelMin);
    ReadField(requirements, "levelMax", &DungeonProgressionRequirements::levelMax);
    ReadField(requirements, "reqItemLevel", &DungeonProgressionRequirements::reqItemLevel);
    auto requirement = Record<ProgressionRequirement>("ProgressionRequirement");
    ReadField(requirement, "id", &ProgressionRequirement::id);
    ReadField(requirement, "note", &ProgressionRequirement::note);
    ReadField(requirement, "priority", &ProgressionRequirement::priority);
    ReadField(requirement, "checkLeaderOnly", &ProgressionRequirement::checkLeaderOnly);
    requirement.add_property("faction", bp::make_function(+[](ProgressionRequirement const& value) { return static_cast<uint32>(value.faction); }, BorrowedPolicy<>()));
    requirements.def("GetQuests", +[](DungeonProgressionRequirements const& value) { return Converter::ToPython(value.quests); }, BorrowedPolicy<>());
    requirements.def("GetItems", +[](DungeonProgressionRequirements const& value) { return Converter::ToPython(value.items); }, BorrowedPolicy<>());
    requirements.def("GetAchievements", +[](DungeonProgressionRequirements const& value) { return Converter::ToPython(value.achievements); }, BorrowedPolicy<>());

    auto auction = Record<AuctionEntry>("AuctionEntry", true);
    ReadField(auction, "Id", &AuctionEntry::Id);
    ReadField(auction, "item_guid", &AuctionEntry::item_guid);
    ReadField(auction, "item_template", &AuctionEntry::item_template);
    ReadField(auction, "itemCount", &AuctionEntry::itemCount);
    ReadField(auction, "owner", &AuctionEntry::owner);
    ReadField(auction, "bidder", &AuctionEntry::bidder);
    ReadField(auction, "startbid", &AuctionEntry::startbid);
    ReadField(auction, "bid", &AuctionEntry::bid);
    ReadField(auction, "buyout", &AuctionEntry::buyout);
    ReadField(auction, "expire_time", &AuctionEntry::expire_time);
    ReadField(auction, "deposit", &AuctionEntry::deposit);

    auto lootItem = Record<LootItem>("LootItem", true);
    ReadField(lootItem, "itemid", &LootItem::itemid);
    ReadField(lootItem, "itemIndex", &LootItem::itemIndex);
    ReadField(lootItem, "randomSuffix", &LootItem::randomSuffix);
    ReadField(lootItem, "randomPropertyId", &LootItem::randomPropertyId);
    lootItem.add_property("count", bp::make_function(+[](LootItem const& value) { return uint32(value.count); }, BorrowedPolicy<>()), bp::make_function(+[](LootItem& value, uint32 count)
    {
        if (count > std::numeric_limits<uint8>::max()) throw std::out_of_range("loot count exceeds uint8");
        value.count = static_cast<uint8>(count);
    }, BorrowedPolicy<>()));
    lootItem.add_property("is_looted", bp::make_function(+[](LootItem const& value) { return bool(value.is_looted); }, BorrowedPolicy<>()));
    lootItem.add_property("needs_quest", bp::make_function(+[](LootItem const& value) { return bool(value.needs_quest); }, BorrowedPolicy<>()));
    auto loot = Record<Loot>("Loot", true);
    ReadField(loot, "gold", &Loot::gold);
    ReadField(loot, "unlootedCount", &Loot::unlootedCount);
    ReadField(loot, "lootOwnerGUID", &Loot::lootOwnerGUID);
    loot.def("SetGold", +[](Loot& value, uint32 gold) { value.gold = gold; }, BorrowedPolicy<>());
    loot.def("ItemCount", +[](Loot const& value) { return value.items.size(); }, BorrowedPolicy<>());
    loot.def("GetItem", +[](Loot& value, std::size_t index) -> LootItem*
    {
        if (index >= value.items.size()) throw std::out_of_range("loot item index");
        return &value.items[index];
    }, BorrowedReturn());
    loot.def("empty", &Loot::empty, BorrowedPolicy<>());
    auto vendor = Record<VendorItem>("VendorItem");
    ReadField(vendor, "item", &VendorItem::item);
    ReadField(vendor, "maxcount", &VendorItem::maxcount);
    ReadField(vendor, "incrtime", &VendorItem::incrtime);
    ReadField(vendor, "ExtendedCost", &VendorItem::ExtendedCost);
    auto roll = Record<Roll>("Roll", true);
    ReadField(roll, "itemGUID", &Roll::itemGUID);
    ReadField(roll, "itemid", &Roll::itemid);
    ReadField(roll, "itemCount", &Roll::itemCount);
    ReadField(roll, "totalPlayersRolling", &Roll::totalPlayersRolling);
    ReadField(roll, "totalNeed", &Roll::totalNeed);
    ReadField(roll, "totalGreed", &Roll::totalGreed);
    ReadField(roll, "totalPass", &Roll::totalPass);

    auto movement = Record<MovementInfo>("MovementInfo", true);
    ReadField(movement, "guid", &MovementInfo::guid);
    ReadField(movement, "flags", &MovementInfo::flags);
    ReadField(movement, "flags2", &MovementInfo::flags2);
    ReadField(movement, "time", &MovementInfo::time);
    ReadField(movement, "pitch", &MovementInfo::pitch);
    ReadField(movement, "fallTime", &MovementInfo::fallTime);
    movement.def("GetPositionX", +[](MovementInfo const& value) { return value.pos.GetPositionX(); }, BorrowedPolicy<>());
    movement.def("GetPositionY", +[](MovementInfo const& value) { return value.pos.GetPositionY(); }, BorrowedPolicy<>());
    movement.def("GetPositionZ", +[](MovementInfo const& value) { return value.pos.GetPositionZ(); }, BorrowedPolicy<>());
    movement.def("GetOrientation", +[](MovementInfo const& value) { return value.pos.GetOrientation(); }, BorrowedPolicy<>());
    auto skill = Record<SkillLineAbilityEntry>("SkillLineAbilityEntry");
    ReadField(skill, "ID", &SkillLineAbilityEntry::ID);
    ReadField(skill, "SkillLine", &SkillLineAbilityEntry::SkillLine);
    ReadField(skill, "Spell", &SkillLineAbilityEntry::Spell);
    ReadField(skill, "RaceMask", &SkillLineAbilityEntry::RaceMask);
    ReadField(skill, "ClassMask", &SkillLineAbilityEntry::ClassMask);
    ReadField(skill, "MinSkillLineRank", &SkillLineAbilityEntry::MinSkillLineRank);
    auto talent = Record<TalentEntry>("TalentEntry");
    ReadField(talent, "TalentID", &TalentEntry::TalentID);
    ReadField(talent, "TalentTab", &TalentEntry::TalentTab);
    ReadField(talent, "Row", &TalentEntry::Row);
    ReadField(talent, "Col", &TalentEntry::Col);
    ReadField(talent, "DependsOn", &TalentEntry::DependsOn);
    ReadField(talent, "DependsOnRank", &TalentEntry::DependsOnRank);
    talent.def("GetRankSpell", +[](TalentEntry const& value, std::size_t rank)
    {
        if (rank >= value.RankID.size()) throw std::out_of_range("talent rank index");
        return value.RankID[rank];
    }, BorrowedPolicy<>());
    auto itemSet = Record<ItemSetEffect>("ItemSetEffect");
    ReadField(itemSet, "setid", &ItemSetEffect::setid);
    ReadField(itemSet, "item_count", &ItemSetEffect::item_count);
    itemSet.def("GetSpell", +[](ItemSetEffect const& value, std::size_t index) -> SpellInfo const*
    {
        if (index >= 8) throw std::out_of_range("item-set spell index");
        return value.spells[index];
    }, BorrowedReturn());
    auto scaling = Record<ScalingStatValuesEntry>("ScalingStatValuesEntry");
    ReadField(scaling, "Id", &ScalingStatValuesEntry::Id);
    ReadField(scaling, "Level", &ScalingStatValuesEntry::Level);
    ReadField(scaling, "spellPower", &ScalingStatValuesEntry::spellPower);
    scaling.def("GetArmorMod", &ScalingStatValuesEntry::getArmorMod, BorrowedPolicy<>());
    scaling.def("GetDPSMod", &ScalingStatValuesEntry::getDPSMod, BorrowedPolicy<>());
    scaling.def("GetStatMultiplier", &ScalingStatValuesEntry::getssdMultiplier, BorrowedPolicy<>());
    auto rewarder = Record<KillRewarder>("KillRewarder", true);
    rewarder.def("GetVictim", &KillRewarder::GetVictim, BorrowedReturn());
    rewarder.def("GetKiller", &KillRewarder::GetKiller, BorrowedReturn());
}
#endif
