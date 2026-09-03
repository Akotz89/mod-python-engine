# Python Scripting Engine

This repository embeds CPython and Boost.Python in AzerothCore. Embedded execution is disabled by
default and is not a process sandbox. A script can still use ordinary Python modules and perform
process-external side effects; the capability profile limits the AzerothCore-native API only.

## Default posture

```ini
Python.Enabled = 0
Python.ScriptsPath = ""
Python.CapabilityProfile = "safe-static-v1"
Python.AllowUnsafeDevelopmentProfile = 0
Python.UnsafeDevelopmentAcknowledgement = ""
Python.AllowExecCommand = 0
```

`safe-static-v1` is the only default profile. If the engine is deliberately enabled, it exports
immutable game constants but does not export callback registration, borrowed native-object
wrappers, mutable reference wrappers, realm-state mutators, boolean veto callbacks, per-tick or
combat callbacks, or administrative exec.

`Python.ScriptsPath` has no relative fallback. An enabled engine requires an existing absolute
root. The loader canonicalizes it, rejects `..`, rejects symlink path components and entries,
rejects non-regular `.py` entries, verifies containment, and loads regular lowercase `.py` files
in deterministic lexical order. No current-working-directory resolution is performed.

## Development-only legacy surface

The separately named `unsafe-development-v1` profile preserves the legacy native bindings and the
49 hook names that are actually forwarded by the current C++ adapters. It requires all of these
settings at initialization:

```ini
Python.Enabled = 1
Python.CapabilityProfile = "unsafe-development-v1"
Python.AllowUnsafeDevelopmentProfile = 1
Python.UnsafeDevelopmentAcknowledgement = "CPI-223-UNSAFE-DEVELOPMENT-ONLY"
```

Administrative exec additionally requires `Python.AllowExecCommand = 1`. The `.python exec`
command is not registered otherwise.

This profile is unsafe, development-only, and outside production acceptance. It exposes borrowed
native objects and state-mutating functions. Callbacks are synchronous, hold the GIL, cannot be
preempted safely if they block, and may run on latency-sensitive native paths. Native objects may
not be retained after the callback returns. These restrictions are documented contracts, not an
in-process security boundary or a proof that legacy scripts obey them.

## Truthful hook support

Hook support is generated from two source facts: `DEFINE_GAME_HOOK` declarations and native
`TRIGGER_*_HOOK` forwarders. See [the generated summary](docs/hook-inventory.md) and
[the detailed inventory](docs/hook-inventory.csv). At this checkpoint:

- 271 hook names are declared;
- 49 hook names/IDs have native forwarders and are available only in the unsafe profile;
- those 49 are disabled by the safe profile;
- 222 declarations have no native forwarder and are unsupported in every profile.

Registration of unsupported names is rejected. The project does not claim that a declaration is a
working callback, and it does not wire additional hooks merely to increase the count.

## Reload contract

Reload discovers and validates the complete file set, then executes candidates in a fresh namespace
while hook registrations target a private candidate registry. The engine publishes the candidate
namespace and registry together only after every file loads successfully. A candidate failure
preserves the last accepted namespace and registry; the first failed activation therefore remains
empty/fail-closed.

The transaction covers only the engine-owned namespace and hook registry. Arbitrary Python side
effects, imported-module state in `sys.modules`, filesystem/network effects, native calls made by
the unsafe profile, and external systems cannot be rolled back. This is intentionally not described
as a fully transactional Python runtime.

## Lifetime, GIL, and shutdown contract

- Hook lock ordering is native hook lock before GIL acquisition.
- Candidate and active Python references are created, swapped, cleared, and destroyed while the
  GIL is owned.
- Callback dispatch owns the GIL explicitly and converts Python exceptions to logged failures; a
  Python return value never becomes an authoritative native boolean decision.
- The safe profile exposes no native-object wrappers or callbacks, eliminating dangling wrapper
  use in that profile.
- The unsafe profile's native wrappers are borrowed for the synchronous callback only. Retention is
  unsupported and remains unproved at runtime.
- Shutdown disables dispatch, takes the exclusive hook lock, restores the initialization thread
  state on the same native thread, releases Python-owned references, and only then finalizes
  CPython. A thread mismatch refuses unsafe finalization.

## Verification boundary

`python3 tools/generate_hook_inventory.py --check` and
`python3 -m unittest discover -s tests -v` perform source/static and pure-Python checks only. They do
not compile C++, embed CPython, establish ABI compatibility, run worldserver, or prove latency,
memory, hang, crash, restart, rollback, or gameplay behavior.

See [the capability matrix](docs/capability-profiles.md) for the exact authority boundary.

## License

This module is released under the [GNU AGPL v3](LICENSE).
