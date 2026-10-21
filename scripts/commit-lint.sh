#!/usr/bin/env bash
# Fail a PR if any commit does not follow Conventional Commits with an NMC scope, e.g.
#   feat(snmp): add upsAlarmTable
#   fix(modbus): reject quantity 0
#   sec(auth): add lockout
set -euo pipefail
BASE=${1:-origin/main}
re='^(feat|fix|sec|perf|docs|ci|build|test|refactor|chore|revert)(\((snmp|modbus|tcpip|pcns|http|auth|security|ota|core|ci|docker|k8s|tf|deps)\))?!?: .{5,}'
bad=0
while read -r sha msg; do
  if ! grep -Eq "$re" <<<"$msg"; then echo "BAD: $sha $msg"; bad=1; fi
done < <(git log --format='%h %s' --no-merges "$BASE"..HEAD)
[[ $bad -eq 0 ]] && echo "all commit messages OK" || { echo "See docs/branching-and-versioning.md"; exit 1; }
