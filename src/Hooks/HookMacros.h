#ifndef MOD_PYTHON_ENGINE_HOOK_MACROS_H
#define MOD_PYTHON_ENGINE_HOOK_MACROS_H

#include "PythonEngine.h"
#include "HookDefines.h"

/**
 * @brief Convenience Macros for Triggering Hooks
 */

#define TRIGGER_ACCOUNT_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Account::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_ACCOUNT_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Account::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_ACHIEVEMENT_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Achievement::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_ACHIEVEMENT_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Achievement::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_ARENA_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Arena::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_ARENA_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Arena::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_ARENATEAM_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::ArenaTeam::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_ARENATEAM_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::ArenaTeam::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_CREATURE_HOOK(Hook, Entry, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Creature::Hook, Entry, ##__VA_ARGS__)

#define TRIGGER_CREATURE_HOOK_RESULT(Hook, Entry, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Creature::Hook, Entry, Default, ##__VA_ARGS__)

#define TRIGGER_GAMEOBJECT_HOOK(Hook, Entry, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::GameObject::Hook, Entry, ##__VA_ARGS__)

#define TRIGGER_GAMEOBJECT_HOOK_RESULT(Hook, Entry, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::GameObject::Hook, Entry, Default, ##__VA_ARGS__)

#define TRIGGER_ITEM_HOOK(Hook, Entry, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Item::Hook, Entry, ##__VA_ARGS__)

#define TRIGGER_ITEM_HOOK_RESULT(Hook, Entry, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Item::Hook, Entry, Default, ##__VA_ARGS__)

#define TRIGGER_MAP_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Map::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_MAP_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Map::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_PLAYER_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Player::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_PLAYER_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Player::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_SERVER_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Server::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_SERVER_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Server::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_UNIT_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::Unit::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_UNIT_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::Unit::Hook, 0, Default, ##__VA_ARGS__)

#define TRIGGER_WORLD_HOOK(Hook, ...) \
    sPythonEngine->Trigger(PyEng::Hooks::World::Hook, 0, ##__VA_ARGS__)

#define TRIGGER_WORLD_HOOK_RESULT(Hook, Default, ...) \
    sPythonEngine->TriggerResult(PyEng::Hooks::World::Hook, 0, Default, ##__VA_ARGS__)

#endif // MOD_PYTHON_ENGINE_HOOK_MACROS_H
