#!/usr/bin/env bash
# Build one card variant.
#   scripts/build.sh GVS              native build + unit tests (host)
#   scripts/build.sh GVS --cross      cross-compile for the card CPU
#   scripts/build.sh GVS --coverage   native build with gcov + gcovr reports
set -euo pipefail
VARIANT=${1:?usage: build.sh <GVS|GVL|GVXL|GVXL-NAM> [--cross|--coverage|--sanitize]}
MODE=${2:-}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
VERSION=${NMC_VERSION:-$("$ROOT/scripts/version.sh")}
declare -A ARCH=( [GVS]=armhf [GVL]=aarch64 [GVXL]=aarch64 [GVXL-NAM]=aarch64 )
args=(-S "$ROOT/firmware" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNMC_VARIANT="$VARIANT" -DNMC_VERSION="$VERSION")

case "$MODE" in
  --cross)    BUILD="$ROOT/build/$VARIANT-${ARCH[$VARIANT]}"
              args+=(-DCMAKE_TOOLCHAIN_FILE="$ROOT/firmware/cmake/toolchains/${ARCH[$VARIANT]}.cmake" -DNMC_BUILD_TESTS=OFF) ;;
  --coverage) BUILD="$ROOT/build/$VARIANT-cov"; args+=(-DNMC_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug) ;;
  --sanitize) BUILD="$ROOT/build/$VARIANT-asan"; args+=(-DNMC_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug) ;;
  "")         BUILD="$ROOT/build/$VARIANT-host" ;;
  *)          echo "unknown mode $MODE" >&2; exit 1 ;;
esac

mkdir -p "$ROOT/reports"
cmake -B "$BUILD" "${args[@]}"
cmake --build "$BUILD"
cmake --install "$BUILD" --prefix "$BUILD/install"

if [[ "$MODE" != "--cross" ]]; then
  (cd "$BUILD" && ctest --output-on-failure --output-junit "$ROOT/reports/unit-$VARIANT.xml")
fi
if [[ "$MODE" == "--coverage" ]]; then
  mkdir -p "$ROOT/reports"
  gcovr -r "$ROOT/firmware" "$BUILD" --exclude '.*tests/.*' \
        --xml "$ROOT/reports/coverage-$VARIANT.xml" --html-details "$ROOT/reports/coverage-$VARIANT.html" \
        --sonarqube "$ROOT/reports/coverage-sonar-$VARIANT.xml" --print-summary --fail-under-line 70
fi
file "$BUILD/nmc-fw"
echo "built $VARIANT $VERSION -> $BUILD/install"
