#!/usr/bin/env bash
# Create a LOCAL ECDSA P-256 key pair for practising firmware signing.
# Production signing uses an AWS KMS key that never leaves the HSM (terraform/modules/kms-signing).
set -euo pipefail
mkdir -p keys
openssl ecparam -name prime256v1 -genkey -noout -out keys/dev-signing-private.pem
openssl ec -in keys/dev-signing-private.pem -pubout -out keys/dev-signing-public.pem
chmod 600 keys/dev-signing-private.pem
echo "created keys/dev-signing-*.pem (private key is git-ignored)"
