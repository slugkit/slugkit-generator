#!/usr/bin/env bash
# Build the shared libslugkit_c for the host so `dart test` can load it via ffi.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # bindings/dart
GEN_ROOT="$(cd "$HERE/../.." && pwd)"                      # generator root
BUILD="$HERE/.build/host"

cmake -S "$GEN_ROOT/slugkit/mobile" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release -DSLUGKIT_MOBILE_SHARED=ON >/dev/null
cmake --build "$BUILD" -j >/dev/null

echo "Built shared library in $BUILD"
ls "$BUILD"/libslugkit_c.* 2>/dev/null || true
