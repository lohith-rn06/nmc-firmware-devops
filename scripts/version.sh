#!/usr/bin/env bash
# Compute the build version (SemVer 2.0):
#   tag v1.4.0            -> 1.4.0
#   main / release/*      -> 1.4.0-rc.<build>+<sha>
#   any other branch      -> 1.4.0-dev.<build>+<sha>
set -euo pipefail
BASE=$(cat "$(dirname "$0")/../VERSION")
SHA=$(git rev-parse --short HEAD 2>/dev/null || echo nogit)
BUILD=${BUILD_NUMBER:-0}
BRANCH=${BRANCH_NAME:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo local)}
if [[ -n "${TAG_NAME:-}" ]]; then
  [[ "${TAG_NAME#v}" == "$BASE" ]] || { echo "tag $TAG_NAME does not match VERSION $BASE" >&2; exit 1; }
  echo "$BASE"
elif [[ "$BRANCH" == "main" || "$BRANCH" == release/* ]]; then
  echo "${BASE}-rc.${BUILD}+${SHA}"
else
  echo "${BASE}-dev.${BUILD}+${SHA}"
fi
