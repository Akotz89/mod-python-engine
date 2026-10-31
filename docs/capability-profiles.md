# General-purpose engine

The agent-added safe-static-v1, safe-composed-v1, population-v1 and unsafe-development-v1 capability profiles and acknowledgement/exec gates were removed under the owner's 2026-10-11 instruction. They are historical implementation facts, not current runtime policy.

`Python.Enabled` controls the engine. Its enabled surface always includes native bindings, general hooks, typed ports and administrator exec. The independent optional population consumer remains controlled by `Python.Population.Enabled`; turning it on is separate from restoring the engine.

See the main README and native API/hook inventories for the supported surface and correctness constraints.
