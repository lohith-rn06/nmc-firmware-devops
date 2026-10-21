#!/usr/bin/env bash
# Build and run fuzzers with clang + libFuzzer + ASan/UBSan.
# Usage: tests/fuzz/build-fuzzers.sh [seconds-per-target]
set -euo pipefail
cd "$(dirname "$0")/../.."
SECS=${1:-60}
OUT=build/fuzz; mkdir -p "$OUT"
CFLAGS="-g -O1 -fsanitize=fuzzer,address,undefined -D_GNU_SOURCE -Iinclude -DNMC_FEATURE_SNMP=1 -DNMC_FEATURE_MODBUS=1"
COMMON="src/core/config.c src/core/log.c src/core/ups_model.c src/net/tcpip.c src/security/sha256.c src/security/auth.c src/security/audit_log.c"
clang $CFLAGS tests/fuzz/fuzz_snmp.c src/snmp/*.c $COMMON -lm -lpthread -o $OUT/fuzz_snmp
clang $CFLAGS tests/fuzz/fuzz_modbus.c src/modbus/*.c $COMMON -lm -lpthread -o $OUT/fuzz_modbus
for t in snmp modbus; do
  mkdir -p $OUT/corpus-$t && cp tests/fuzz/corpus/$t/* $OUT/corpus-$t/
  $OUT/fuzz_$t -max_total_time=$SECS -print_final_stats=1 $OUT/corpus-$t 2>&1 | tail -5
done
