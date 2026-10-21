#!/usr/bin/env bash
# Quick post-deploy check of a running NMC (simulator, pod or real card).
set -euo pipefail
HOST=${1:-127.0.0.1}; HTTP=${2:-8080}; SNMP=${3:-1161}; COMMUNITY=${NMC_SNMP_COMMUNITY:-lab-ro}
curl -fsS "http://$HOST:$HTTP/healthz"
curl -fsS "http://$HOST:$HTTP/api/version" | jq .
if command -v snmpget >/dev/null; then
  snmpget -v2c -c "$COMMUNITY" "$HOST:$SNMP" 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.33.1.4.1.0
fi
echo "smoke test OK"
