#!/usr/bin/env bash
# Delete ephemeral simulator namespaces whose branch no longer exists (run nightly from Jenkins).
set -euo pipefail
for ns in $(kubectl get ns -l nmc.io/ephemeral=true -o jsonpath='{.items[*].metadata.name}'); do
  branch=$(kubectl get ns "$ns" -o jsonpath='{.metadata.annotations.nmc\.io/branch}')
  if ! git ls-remote --exit-code --heads origin "$branch" >/dev/null 2>&1; then
    echo "deleting $ns (branch $branch is gone)"; kubectl delete ns "$ns" --wait=false
  fi
done
