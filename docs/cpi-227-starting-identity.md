# CPI-227 frozen starting identity

- Upstream: `https://github.com/privatecore/mod-python-engine.git`
- Exact upstream base commit: `700a4f75a2e550f17022665d7b5b009c24e46a40`
- Exact upstream base tree: `73453619b827984f75e8871247f9b45e38bece85`
- Accepted prior hardening patch SHA-256: `36994d26a8998f38bbaa39a806859675dfc31afe1618298912f1028dac6ed779`
- Accepted prior result tree: `96266817107d8633059bc4687d8e2808e70e271f`
- Accepted prior isolated commit: `121516f8c5cb173e9727dabfa4e221efb39278da`
- Accepted prior evidence SHA-256: `d3383993fcbdc9fef3e27b96967b048df8c59a1ba4f6b80c828a4067af3a6f9e`
- Fresh CPI-227 branch: `codex/cpi-227-safe-static-hardening`
- Fresh isolated checkout: `tmp/cpi-227-python-engine-hardening-20260902`

At checkout creation, `HEAD` was the accepted prior isolated commit, `HEAD^{tree}` was the accepted
prior result tree, `HEAD^` was the exact upstream base, `git status --short --branch` was clean, and
`git fsck --strict --no-progress` passed.

This identity is source/static evidence only. It does not authorize or prove a C++ build, package,
Python/Boost ABI, embedded interpreter execution, worldserver startup, runtime configuration,
database behavior, service behavior, latency, memory, hang/crash containment, rollback, gameplay,
deployment, or promotion.
