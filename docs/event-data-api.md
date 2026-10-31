# Native event data

Native event arguments expose the following classes alongside Player, Creature,
Unit, Map, Item, Quest, Spell, Aura, Group and Guild. Borrowed arguments and
returned native objects are valid only during their callback, on that callback's
thread. Copy scalar fields or packet bytes if they are needed later. Retained
native references raise `ReferenceError`; background use raises `RuntimeError`.

Record fields below use their native spelling. Database/DBC template records are
inspection inputs; changes to event decisions use the hook's native return or
mutable arguments. The module adds no database synchronization or persistence
behind these fields.

Packet/network callbacks can inspect their packet and session data. Live game
objects and mutable gameplay records require their native gameplay callback or
the world thread. Use `azerothcore.Schedule(callback)` to enqueue work on a later
world update, copy GUIDs and scalar values into the scheduled work, and reacquire
a player with `azerothcore.GetPlayer(guid)`. Do not capture borrowed Player/Map
objects for a later callback. This follows core thread ownership; packet editing
remains available directly in packet callbacks.

| Class | Useful fields and methods |
| --- | --- |
| ByteBuffer | Size, GetBytes, AppendBytes, GetReadPosition, SetReadPosition; ReadUInt8/16/32/64 and ReadFloat; GetUInt8/16/32; PutUInt8/16/32; AppendUInt8/16/32 |
| WorldPacket | ByteBuffer operations, GetOpcode, SetOpcode |
| AuraEffect | GetId, GetEffIndex, GetAmount, SetAmount, GetMiscValue, GetCaster, GetBase, GetSpellInfo |
| ArenaTeam | GetId, GetType, GetName, GetCaptain, GetRating |
| Battleground | GetName, GetMapId, GetInstanceID, GetPlayersSize, GetStatus, GetWinner |
| Channel | GetName, GetChannelId, GetNumPlayers, GetFlags, IsLFG |
| InstanceScript | GetMap, GetBossState, SetBossState, GetData, SetData |
| AchievementEntry | ID, requiredFaction, mapID, categoryId, points, flags, count, refAchievement |
| AchievementCriteriaEntry | ID, referredAchievement, requiredType, flags, timeLimit, asset, count |
| CriteriaProgress | counter, date, changed |
| AchievementMgr | GetPlayer, HasAchieved, GetCriteriaProgress |
| AchievementGlobalMgr | IsRealmCompleted, GetAchievement |
| CreatureTemplate | Entry, Name, minlevel, maxlevel, faction, npcflag, lootid |
| FactionTemplateEntry | ID, faction, factionFlags, ourMask, friendlyMask, hostileMask, IsFriendlyTo, IsHostileTo |
| MapEntry | MapID, map_type, Flags, expansionID, maxPlayers, IsDungeon, IsRaid |
| MapDifficulty | resetTime, maxPlayers, hasErrorMessage |
| InstanceTemplate | Parent, ScriptId, AllowMount |
| DungeonProgressionRequirements | levelMin, levelMax, reqItemLevel, GetQuests, GetItems, GetAchievements |
| ProgressionRequirement | id, faction, note, priority, checkLeaderOnly |
| AuctionEntry | Id, item_guid, item_template, itemCount, owner, bidder, startbid, bid, buyout, expire_time, deposit |
| LootItem | itemid, itemIndex, randomSuffix, randomPropertyId, count, is_looted, needs_quest; count is writable |
| Loot | gold, unlootedCount, lootOwnerGUID, SetGold, ItemCount, GetItem, empty |
| VendorItem | item, maxcount, incrtime, ExtendedCost |
| Roll | itemGUID, itemid, itemCount, totalPlayersRolling, totalNeed, totalGreed, totalPass |
| MovementInfo | guid, flags, flags2, time, pitch, fallTime, GetPositionX/Y/Z, GetOrientation |
| SkillLineAbilityEntry | ID, SkillLine, Spell, RaceMask, ClassMask, MinSkillLineRank |
| TalentEntry | TalentID, TalentTab, Row, Col, DependsOn, DependsOnRank, GetRankSpell |
| ItemSetEffect | setid, item_count, GetSpell |
| ScalingStatValuesEntry | Id, Level, spellPower, GetArmorMod, GetDPSMod, GetStatMultiplier |
| KillRewarder | GetVictim, GetKiller |

Packet getters take byte offsets; typed Read methods advance the native read
cursor. Packet byte snapshots are Python `bytes`. AppendBytes accepts a
contiguous Python buffer, including bytes and bytearray. Ranges and cursor
positions are checked before accessing native memory, including offset overflow.
Use packet modification only at a hook with a mutable packet argument.

Guardian and TempSummon expose Creature/Unit APIs; InstanceMap and MapInstanced
expose Map APIs through the converter's native base-type fallback. Generic
NativeReference remains for internal reference-cache bookkeeping and database
transaction handles; it does not pretend to provide gameplay fields or a query
API. Inspect database data through the native gameplay object passed to the hook.

`tests/test_event_data.cpp` checks the actual packet adapters against CPython and
the native ByteBuffer implementation. Other native classes require the embedded
realm linkage; compilation of this export file proves signature compatibility,
while actual hook calls and lifetime behavior are separately exercised by the
dispatcher, borrowed-binding and realm acceptance checks.
