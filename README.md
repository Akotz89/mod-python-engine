# Python Scripting Engine

Embed CPython in AzerothCore to write custom server gameplay in Python, use native game objects and events, and reload scripts without recompiling the server. This maintained fork derives from [privatecore/mod-python-engine](https://github.com/privatecore/mod-python-engine) and retains its AGPL-3.0 license.

## Configure and use

Build with matching CPython development headers and Boost.Python. Copy `conf/mod_python_engine.conf.dist` to the server module config directory and set:

```ini
Python.Enabled = 1
Python.ScriptsPath = "/path/to/python_scripts"
```

All regular `.py` files below that directory load in lexical order. Relative roots resolve against the server working directory. The full hook API and native exports are available whenever the engine is enabled; there are no capability profiles, magic acknowledgements or separate exec opt-ins.

```python
import azerothcore as ac

def welcome(player):
    player.SendSysMessage(f"Welcome, {player.GetName()}!")

ac.Register("PLAYER_ON_LOGIN", welcome)

def greet(player, creature):
    creature.Say("Welcome, traveler.", 0)
    return True

ac.Register("CREATURE_ON_GOSSIP_HELLO", greet, 99999)
```

An entry ID of zero receives every event in its category; an NPC/item/gameobject entry selects that template. Global callbacks run before entry callbacks. `Register` also works from running callbacks or administrator `python exec`; callback snapshots keep registration and recursive native events safe.

Administrators and the server console use `python status`, `python reload` and `python exec <code>`, with AzerothCore's existing administrator command permission. Reload replaces the callback registry and namespace only after every script succeeds. Edited imports below the script root are refreshed too; a rejected candidate restores the prior imported-module registry. Pending scheduled callbacks belong to the script generation: successful reload publishes the new staged queue; a rejected reload retains the previous queue. External/native effects performed while loading are not transactions and cannot be rolled back.

## Native decisions and references

Void events ignore callback results. For native boolean permit hooks, `False` vetoes; handled hooks combine `True` results. `None` keeps the native default. Nonboolean results use the last valid non-`None` value. Exceptions and invalid result conversions are logged without escaping into the core. Mutable arguments expose `.get()` and `.set(value)` and commit back to the original native reference when the callback finishes.

Network callbacks operate on packet/session metadata. Use `ac.Schedule(callback)` to run gameplay work on the next world tick; capture GUIDs/copied data and reacquire an online player with `ac.GetPlayer(guid)` inside that callback. Map callbacks keep their native map execution context. A network thread cannot directly touch world/map-owned gameplay objects.

Native object and mutable-reference wrappers belong to the callback where they were received. Retaining a wrapper is allowed, but calling it after that callback returns raises `ReferenceError`; use GUIDs or copied values across callbacks. Using it from another thread raises `RuntimeError`. Owning value objects such as Position and ObjectGuid remain ordinary values. Native entity removal invalidates its borrowed wrappers.

See [hook inventory](docs/hook-inventory.md) for exact event forwarding and [API details](docs/native-api.md) for binding coverage and signatures. `GetMap(mapId)` and `Map.LoadGrid(x, y)` provide native base-map access on the server thread, useful for server-side scripts without a player session.

## Existing consumers

The typed party/event and population ports remain available alongside general scripting. `Python.Population.Enabled = 0` disables only that optional population consumer; enabling the engine does not admit bots or activate its scripts. Native provider permissions, thread ownership and persisted-state validation still apply.

This is trusted Python in the server process. Exceptions are contained, but a hung callback cannot be forcibly preempted. Scripts can use the normal Python standard library; administrators are responsible for the code they install.
