#!/usr/bin/env bash
# Package a cross-compiled variant into dist/nmc-<variant>-<version>.nmcfw (+ manifest, sha256)
set -euo pipefail
VARIANT=${1:?variant}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
VERSION=${NMC_VERSION:-$("$ROOT/scripts/version.sh")}
declare -A ARCH=( [GVS]=armhf [GVL]=aarch64 [GVXL]=aarch64 [GVXL-NAM]=aarch64 )
SRC="$ROOT/build/$VARIANT-${ARCH[$VARIANT]}/install"
[[ -d "$SRC" ]] || { echo "run scripts/build.sh $VARIANT --cross first" >&2; exit 1; }
OUT="$ROOT/dist/nmc-$VARIANT-${VERSION%%+*}.nmcfw"
python3 "$ROOT/tools/fwpack.py" --variant "$VARIANT" --version "$VERSION" --input "$SRC" --output "$OUT"
(cd "$ROOT/dist" && sha256sum "$(basename "$OUT")" > "$(basename "$OUT").sha256")
echo "$OUT"
