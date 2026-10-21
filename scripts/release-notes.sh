#!/usr/bin/env bash
# Generate release notes from Conventional Commits between the previous tag and HEAD.
set -euo pipefail
PREV=$(git describe --tags --abbrev=0 HEAD^ 2>/dev/null || git rev-list --max-parents=0 HEAD)
echo "## NMC firmware $(cat VERSION)"; echo
for t in feat fix sec perf; do
  case $t in feat) h="Features";; fix) h="Bug fixes";; sec) h="Security";; perf) h="Performance";; esac
  lines=$(git log --format='- %s (%h)' "$PREV"..HEAD | grep -E "^- $t(\(|:)" || true)
  [[ -n "$lines" ]] && { echo "### $h"; echo "$lines"; echo; }
done
echo "### Variants"; echo "GVS, GVL, GVXL, GVXL-NAM"
