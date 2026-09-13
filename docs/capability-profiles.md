# Embedded capability profiles

The embedded engine is optional and deferred. `Python.Enabled = 0` remains the mandatory production
default, and no profile grants the engine authority over Playerbots, PopulationDirector,
progression, durable state, database transactions, release, or rollback.

| Capability | `safe-static-v1` (default if enabled) | `safe-composed-v1` | `population-v1` | `unsafe-development-v1` |
| --- | --- | --- | --- | --- |
| Immutable game enums/constants | exported | exported | exported | exported |
| Hook registration | not exported; defense-in-depth rejection | not exported | not exported | 49 truthfully forwarded names only |
| Unwired declared hooks | unsupported | unsupported | unsupported | unsupported |
| Borrowed native-object wrappers | not exported | not exported | not exported | exposed; callback-lifetime-only contract |
| Mutable-reference wrappers | not exported | not exported | not exported | exposed |
| Realm-state mutators | not exported | not exported | not exported | exposed |
| Typed safe events | not exported | exported; non-veto callbacks | exported; non-veto callbacks | not exported |
| Typed party capability | not exported | exported; value snapshots/provider results | exported; value snapshots/provider results | not exported |
| Typed population capability | not exported | exported; tick-fenced | exported; tick-fenced | not exported |
| Population tick | not exported | optional entry-script callback | required entry-script callback | not exported |
| Authoritative boolean vetoes | not exposed | not exposed | not exposed | callbacks run but native fixed decisions remain authoritative |
| Combat/per-tick/hot-path callbacks | forbidden | forbidden | population tick only; bounded post-call check | exposed, synchronous, unbounded, not accepted |
| Administrative `.python exec` | not registered | not registered | not registered | separate `Python.AllowExecCommand` gate |
| Production acceptance | no; engine remains deferred | no; runtime gates remain | no; runtime gates remain | never |

The safe-composed profile requires only `Python.CapabilityProfile = "safe-composed-v1"`; the engine
remains disabled unless `Python.Enabled = 1`. The population profile additionally requires a valid
relative `Python.Population.EntryScript` that publishes `population_tick`.

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
native wrappers, mutable references, or mutators. The typed safe profiles add only value snapshots
and native provider calls for the event/party seam; native code remains authoritative for every
operation. They are still in-process Python and are not an
operating-system sandbox; scripts can have external side effects through the ordinary Python
standard library. Consequently, even `safe-static-v1` is not accepted for production merely because
its AzerothCore-native API is narrow.

## Reload and failure semantics

The engine stages a complete candidate namespace, hook registry, safe-event registry, and optional
population callback under the exclusive hook lock and GIL. Publication swaps the candidate only
after every deterministic candidate file loads. Candidate failure preserves the last accepted
objects. If no candidate has ever been accepted, failure leaves the registries empty.

Safe event callbacks are synchronous and receive a dictionary. They must return `None`; exceptions
and post-call budget overruns disable further safe-event dispatch. `party.vendor_open` and
`party.mailbox_open` are typed event names currently reserved by the native party seam. Party
operations are exposed as value-returning functions: `party_read_current`, `party_read_inventory`,
`party_equip`, `party_trade`, `party_sell_vendor`, `party_request_item_use`, and
`party_send_mail_batch`. Provider code owns validation, mutation, persistence, and result codes.

The engine does not and cannot roll back arbitrary script side effects, imported-module state,
filesystem/network activity, or unsafe native calls. A preserved registry is therefore a precise
engine-owned transaction, not a claim that Python execution itself is reversible.

## Native-object and dispatch semantics

The safe profiles cannot create native wrappers. The unsafe profile's wrappers are borrowed and valid
only during the synchronous callback that received them; retaining one is unsupported. Each dispatch
acquires the GIL explicitly. Exceptions are logged and do not change fixed native boolean return
values. A callback can still hang or crash the process, and no source-level timeout can safely
preempt arbitrary in-process Python. These are explicit residual risks and runtime acceptance gates,
not solved properties.

Shutdown disables dispatch, takes the exclusive hook lock, and restores the saved initialization
thread state only when shutdown is running on that same native thread. It releases Python-owned
references before finalization. A thread mismatch refuses unsafe restoration/finalization and keeps
the engine fail-closed rather than guessing about CPython thread ownership.
