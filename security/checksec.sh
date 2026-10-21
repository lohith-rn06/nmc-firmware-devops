#!/usr/bin/env bash
# Verify binary hardening of the built firmware (works for x86, ARM and AArch64 ELF).
# Fails the build if PIE, full RELRO, NX stack or stack canaries are missing.
set -euo pipefail
rc=0
for bin in "$@"; do
  echo "== $bin ($(file -b "$bin" | cut -d, -f1-2))"
  hdr=$(readelf -h "$bin"); dyn=$(readelf -d "$bin" 2>/dev/null || true)
  seg=$(readelf -lW "$bin"); sym=$(readelf -sW --dyn-syms "$bin" 2>/dev/null || true)
  check() { if eval "$2"; then echo "  [PASS] $1"; else echo "  [FAIL] $1"; rc=1; fi; }
  check "PIE"            'grep -q "Type:.*DYN" <<<"$hdr"'
  check "Full RELRO"     'grep -q GNU_RELRO <<<"$seg" && grep -Eq "BIND_NOW|FLAGS.*NOW" <<<"$dyn"'
  check "NX stack"       '! grep -A1 GNU_STACK <<<"$seg" | grep -q RWE'
  check "Stack canary"   'grep -q __stack_chk_fail <<<"$sym"'
done
exit $rc
