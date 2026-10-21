#!/usr/bin/env bash
# Verify a firmware image: signature (authenticity) + header/sha256 (integrity) + variant.
#   security/verify-firmware.sh dist/nmc-GVS-1.4.0.nmcfw keys/dev-signing-public.pem
set -euo pipefail
IMG=${1:?image}; PUB=${2:?public key pem}
openssl dgst -sha256 -verify "$PUB" -signature "$IMG.sig" "$IMG"
python3 - "$IMG" <<'PY'
import hashlib, struct, sys
data = open(sys.argv[1], "rb").read()
magic, ver, _, variant, version, plen, digest = struct.unpack("<4sB3s16s32sI32s", data[:92])
assert magic == b"NMCF", "bad magic"
payload = data[128:128 + plen]
assert hashlib.sha256(payload).digest() == digest, "payload sha256 mismatch"
v_name = variant.rstrip(b"\0").decode()
v_ver = version.rstrip(b"\0").decode()
print(f"OK variant={v_name} version={v_ver} payload={plen} bytes")
PY
