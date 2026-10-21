# Runbook: simulator pod CrashLoopBackOff in nmc-ci-* namespace
1. `kubectl -n <ns> logs deploy/nmc-gvs --previous`.
2. `bind port ... failed` → port already used / below 1024 while non-root: ports must be ≥ 1024 in pods.
3. `no entropy source available` → container has no `/dev/urandom` (should never happen; check runtime class).
4. Missing secret `nmc-sim-credentials` → `nmcDeploySim` step failed earlier; re-run.
