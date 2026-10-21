#!/usr/bin/env bash
# Sign every dist/*.nmcfw with ECDSA P-256 / SHA-256, producing a detached DER signature (.sig).
#   security/sign-firmware.sh local keys/dev-signing-private.pem     (practice)
#   security/sign-firmware.sh kms   alias/nmc-fw-signing-dev         (Jenkins, AWS KMS HSM)
set -euo pipefail
MODE=${1:?local|kms}; KEY=${2:?key file or KMS alias}
shopt -s nullglob
files=(dist/*.nmcfw); [[ ${#files[@]} -gt 0 ]] || { echo "no firmware in dist/" >&2; exit 1; }
for f in "${files[@]}"; do
  case "$MODE" in
    local)
      openssl dgst -sha256 -sign "$KEY" -out "$f.sig" "$f" ;;
    kms)
      # KMS signs the DIGEST so multi-MB images are not sent to the API
      openssl dgst -sha256 -binary "$f" > "$f.digest"
      aws kms sign --key-id "$KEY" --message "fileb://$f.digest" --message-type DIGEST \
          --signing-algorithm ECDSA_SHA_256 --query Signature --output text | base64 -d > "$f.sig"
      rm -f "$f.digest"
      [[ -f dist/signing-public.pem ]] || aws kms get-public-key --key-id "$KEY" --query PublicKey --output text \
          | base64 -d | openssl pkey -pubin -inform DER -out dist/signing-public.pem ;;
    *) echo "unknown mode $MODE" >&2; exit 1 ;;
  esac
  echo "signed $f"
done
