#!/usr/bin/env bash
# Start all 4 simulated NMC cards with Docker Compose and run the integration suite against each.
set -euo pipefail
cd "$(dirname "$0")/../docker"
docker compose --profile build build toolchain
docker compose up -d --build
for v in GVS GVL GVXL GVXL-NAM; do
  host="nmc-$(echo "$v" | tr 'A-Z' 'a-z')"
  echo "=== integration tests: $v ($host)"
  docker compose --profile test run --rm --use-aliases -e NMC_HOST="$host" -e NMC_VARIANT="$v" itest
done
