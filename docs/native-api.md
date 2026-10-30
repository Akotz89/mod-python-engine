# Native Python API

The enabled engine exports the complete upstream native class surface and all registration/administration functions. There are no capability profiles, unsafe acknowledgements, hook-category denials, or exec opt-ins. Existing account/administrator permissions still govern `.python` commands.

`azerothcore.Register(name, callback[, entry])` registers global or entry-specific callbacks during load, admin exec, or a callback. Void hooks ignore Python return values. Decision hooks consume values according to their native semantics; see the generated hook inventory for each hook's native signature and default. Python exceptions are contained per callback and preserve the native default decision.

## Native object and reference lifetime

Native callback objects keep their Boost.Python type, class inheritance, and methods. Their memory is owned by the server. All methods, properties, and returned native objects validate the originating callback/thread before touching memory. Borrowed objects expire when that callback returns; methods raise `ReferenceError` afterward and `RuntimeError` from another thread. Native removal/destruction invalidates tracked wrappers. Native map/game callbacks retain their original native game context; packet callbacks running on network threads can edit their packet and access session metadata, but cannot dereference game-owned objects. Use `Schedule(callback)` with copied IDs/values to run that work on the world thread. The scheduled callback resolves a fresh handle through `GetPlayer(guid)` or `GetMap(map_id)`; keeping an expired borrowed object inside a closure does not extend its lifetime. Store `GetObjectGUID()` (the full owning `ObjectGuid`) or copied fields for later work. Owning `ObjectGuid`, `Position`, and `WorldLocation` values remain ordinary Python-accessible values.

Mutable scalar/enum parameters provide `.get()` and `.set(value)`. `StringRef` and `MutableValue` cover native strings and containers/pointer output references. They copy the incoming value, validate replacements, and commit to the native reference when dispatch ends; retaining these objects after dispatch raises `ReferenceError`. Container `.get()` returns a copy; call `.set(updated_value)` to commit edits. Native enum references use their underlying integer values and typed copyback rather than aliasing storage. Array-indexed power and server-visibility enums validate the native slot bounds; floating references reject NaN/Inf. Failed validation leaves the native value unchanged.

Lists/sets/vectors/maps are copied recursively into Python lists/dicts; time points become epoch milliseconds. Exported pointer types become their native class wrapper; null pointers become `None`. Other upstream argument types become an explicitly opaque `NativeReference` with `GetTypeName()`, `IsValid()`, and `__repr__`. These handles allow callback delivery and correctly typed pointer output references, but do not claim unimplemented native class methods. They cannot be dereferenced from Python or kept alive beyond dispatch. This is an upstream API gap, not a capability-policy denial.

## Exported native classes

| Class | Methods and read-only properties |
| --- | --- |
| `Quest` | `GetQuestId`, `GetQuestLevel`, `GetTitle` |
| `Corpse` | `GetOwnerGUID` |
| `Spell` | `GetCaster`, `GetSpellInfo` |
| `Aura` | `GetId`, `GetCaster`, `GetSpellInfo` |
| `AuraApplication` | `GetTarget` |
| `WorldSession` | `GetAccountId`, `GetSecurity`, `GetPlayer` |
| `Group` | `GetLeaderGUID`, `GetMembersCount` |
| `Guild` | `GetId`, `GetName`, `GetLeaderGUID` |
| `Creature` | `GetEntry`, `Despawn`, `Respawn`, `Say`, `Yell`, `GetGUID` |
| `GameObject` | `GetEntry`, `GetName`, `Respawn`, `Delete`, `Use`, `GetGoState`, `SetGoState`, `IsSpawned`, `GetGUID` |
| `ItemTemplate` | `Id`, `Class`, `SubClass`, `Name`, `Quality`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `MaxCount`, `Stackable` |
| `Item` | `GetTemplate`, `GetEntry`, `GetCount`, `GetSlot`, `GetBagSlot` |
| `Map` | `GetId`, `GetMapName`, `Instanceable`, `IsDungeon`, `IsNonRaidDungeon`, `IsRaid`, `IsRaidOrHeroicDungeon`, `IsHeroic`, `Is25ManRaid`, `IsBattleground`, `IsBattleArena`, `IsBattlegroundOrArena`, `IsWorldMap`, `GetCreature`, `GetGameObject`, `LoadGrid`, `GetHeight`, `GetAreaId`, `GetZoneId` |
| `Object` | `GetObjectGUID`, `GetEntry`, `GetTypeId`, `isType`, `IsInWorld`, `IsItem`, `IsUnit`, `IsPlayer`, `IsCreature`, `IsGameObject`, `IsDynamicObject`, `IsCorpse`, `GetUInt32Value`, `GetFloatValue`, `SetUInt32Value`, `SetFloatValue`, `HasFlag`, `SetFlag`, `RemoveFlag` |
| `WorldObject` | `GetName`, `GetInstanceId`, `GetAreaId`, `GetZoneId`, `GetMap` |
| `ObjectGuid` | `GetRawValue`, `GetCounter`, `IsEmpty`, `Clear`, `IsPlayer`, `IsCreature`, `IsGameObject`, `IsItem`, `IsPet`, `IsVehicle`, `IsUnit`, `ToString`, `__str__`, `__repr__`, `__hash__` |
| `Player` | `GetGUID`, `GetAccountId`, `GiveXP`, `GiveLevel`, `SetXP`, `Say`, `Yell`, `SendNotification`, `SendSysMessage`, `HasItem`, `AddItem`, `DestroyItem`, `RemoveItem`, `GetMoney`, `SetMoney`, `ModifyMoney`, `TeleportTo`, `HasQuest`, `CompleteQuest` |
| `Position` | `GetPositionX`, `GetPositionY`, `GetPositionZ`, `GetOrientation`, `GetExactDist2d`, `GetExactDist`, `GetExactDist2dSq`, `GetExactDistSq`, `GetExactDist2dTo`, `GetExactDistTo`, `GetExactDist2dSqTo`, `GetExactDistSqTo`, `GetAngle`, `IsPositionValid` |
| `WorldLocation` | `GetMapId`, `SetMapId` |
| `SpellInfo` | `Id`, `SpellName`, `Rank`, `SchoolMask`, `DmgClass`, `Dispel`, `Mechanic`, `ManaCost`, `ManaCostPercentage`, `PowerType`, `RecoveryTime`, `CategoryRecoveryTime`, `StackAmount`, `IsPassive`, `IsChanneled`, `IsPositive`, `GetDuration`, `GetMaxRange`, `CalcCastTime` |
| `SpellCastTargets` | `GetUnitTarget`, `GetObjectTarget`, `GetCorpseTarget`, `GetItemTarget`, `GetUnitTargetGUID`, `GetObjectTargetGUID`, `GetCorpseTargetGUID`, `GetItemTargetGUID` |
| `Unit` | `GetGUID`, `GetName`, `GetGender`, `GetRace`, `GetClass`, `GetLevel`, `GetHealth`, `GetMaxHealth`, `SetHealth`, `GetPower`, `GetMaxPower`, `IsInCombat`, `IsPolymorphed`, `isFrozen`, `IsInFlight`, `IsLevitating`, `IsWalking`, `IsMoving`, `IsSwimming`, `IsFlying`, `IsFalling`, `IsRooted`, `IsStopped`, `IsAlive`, `IsDead` |

## Functions and commands

- `GetPlayer(guid)` accepts an owning `ObjectGuid` or raw integer player GUID, validates its player identity, and returns the connected native player or `None`. It requires the world thread.
- `Schedule(callback)` queues a callable for the next world update; pass copied IDs/values and resolve native handles inside it. Candidate reload work is staged with script publication, and rejected reloads do not publish queued work.
- `GetMap(map_id)` resolves a validated existing map definition to its native base map; `Map.LoadGrid(x, y)` loads a validated grid through the core. These operations require the world thread and an active native callback scope.
- `Register` and `RegisterSafeEvent` register native and existing typed-event callbacks. Existing population and party bridge functions remain exported; provider presence and native thread/lifecycle correctness are checked. Population activation remains a separate configuration selection.
- `.python status`, `.python reload`, and `.python exec <code>` retain `SEC_ADMINISTRATOR` and console access. Exec supports the same complete module API, including registration. Output truncation affects display only.

## Restored native semantics

`Player.CompleteQuest(id)` calls `Player::CompleteQuest`, which updates native quest state; the upstream binding previously only sent a completion packet. `GiveXP` exposes the native defaults for victim/group rate/LFG reward. `Creature.Despawn` accepts optional integer delay milliseconds and forced respawn seconds, avoiding unbound chrono argument types. `Map.GetHeight` and `SpellInfo.GetMaxRange`/`CalcCastTime` expose their native defaults. Object field access validates indices before entering the core. `SpellInfo.SpellName` and `Rank` return copied locale tuples.

## Verification boundary

`tests/test_borrowed_bindings.cpp` embeds the real converter/call policies and validates methods/properties/returned pointers, scope and native removal expiry, nested dispatch, thread ownership, scalar/string/container/enum copyback and opaque-reference expiry. Source syntax checks compile all native export/function/command units against the retained core headers. These prove binding mechanics; the coordinator's separate embedded dispatcher, build and live runtime evidence prove event delivery/deployment.
