# Embedded capability profiles

The embedded engine is optional and deferred. `Python.Enabled = 0` remains the mandatory production
default, and neither profile grants the engine authority over Playerbots, PopulationDirector,
progression, durable state, database transactions, release, or rollback.

| Capability | `safe-static-v1` (default if enabled) | `unsafe-development-v1` |
| --- | --- | --- |
| Immutable game enums/constants | exported | exported |
| Hook registration | not exported; defense-in-depth rejection | 49 truthfully forwarded names only |
| Unwired declared hooks | unsupported | unsupported |
| Borrowed native-object wrappers | not exported | exposed; callback-lifetime-only contract |
| Mutable-reference wrappers | not exported | exposed |
| Realm-state mutators | not exported | exposed |
| Authoritative boolean vetoes | not exposed | callbacks run but native fixed decisions remain authoritative |
| Combat/per-tick/hot-path callbacks | forbidden | exposed, synchronous, unbounded, not accepted |
| Administrative `.python exec` | not registered | separate `Python.AllowExecCommand` gate |
| Production acceptance | no; engine remains deferred | never |

The unsafe profile requires the exact profile name, `Python.AllowUnsafeDevelopmentProfile = 1`,
and `Python.UnsafeDevelopmentAcknowledgement = "CPI-223-UNSAFE-DEVELOPMENT-ONLY"`. Missing,
misspelled, or unknown values fail initialization. Administrative exec requires a fourth explicit
switch and is absent from the command table otherwise.

## Forbidden authority dependencies

Embedded Python may not own or veto:

- progression, XP, quests, items, loot, money, or economy;
- login/session admission;
- Playerbots lifecycle, strategy, grouping, combat, movement, or travel;
- PopulationDirector policy, durable population membership, or scheduling;
- durable state or database transactions;
- release, deployment, promotion, or rollback;
- any per-tick or latency-sensitive production path.

The default safe profile implements that boundary mechanically by exporting no registration API,
native wrappers, mutable references, or mutators. It is still in-process Python and is not an
operating-system sandbox; scripts can have external side effects through the ordinary Python
standard library. Consequently, even `safe-static-v1` is not accepted for production merely because
its AzerothCore-native API is narrow.

## Reload and failure semantics

The engine stages a complete candidate namespace and hook registry under the exclusive hook lock
and GIL. Publication swaps both objects only after every deterministic candidate file loads.
Candidate failure preserves the last accepted namespace and registry. If no candidate has ever been
accepted, failure leaves the registry empty.

The engine does not and cannot roll back arbitrary script side effects, imported-module state,
filesystem/network activity, or unsafe native calls. A preserved registry is therefore a precise
engine-owned transaction, not a claim that Python execution itself is reversible.

## Native-object and dispatch semantics

The safe profile cannot create native wrappers. The unsafe profile's wrappers are borrowed and valid
only during the synchronous callback that received them; retaining one is unsupported. Each dispatch
acquires the GIL explicitly. Exceptions are logged and do not change fixed native boolean return
values. A callback can still hang or crash the process, and no source-level timeout can safely
preempt arbitrary in-process Python. These are explicit residual risks and runtime acceptance gates,
not solved properties.

Shutdown disables dispatch, takes the exclusive hook lock, and restores the saved initialization
thread state only when shutdown is running on that same native thread. It releases Python-owned
references before finalization. A thread mismatch refuses unsafe restoration/finalization and keeps
the engine fail-closed rather than guessing about CPython thread ownership.
