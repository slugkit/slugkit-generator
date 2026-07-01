#!/usr/bin/env bash
# Build Slugkit.xcframework (macOS + iOS device + iOS simulator) from the userver-free C ABI.
# The xcframework is a build artifact (gitignored); the Swift package's binaryTarget points at it.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # bindings/swift
GEN_ROOT="$(cd "$HERE/../.." && pwd)"                      # generator root (contains slugkit/, cmake/)
MOBILE="$GEN_ROOT/slugkit/mobile"
BUILD="$HERE/.build/cmake"
OUT="$HERE/Slugkit.xcframework"

echo "==> Building static libraries (macOS, iOS device, iOS simulator)"
cmake -S "$MOBILE" -B "$BUILD/macos" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 >/dev/null
cmake --build "$BUILD/macos" -j >/dev/null

cmake -S "$MOBILE" -B "$BUILD/ios-device" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$GEN_ROOT/cmake/toolchains/ios-device.cmake" >/dev/null
cmake --build "$BUILD/ios-device" -j >/dev/null

cmake -S "$MOBILE" -B "$BUILD/ios-sim" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$GEN_ROOT/cmake/toolchains/ios-simulator.cmake" >/dev/null
cmake --build "$BUILD/ios-sim" -j >/dev/null

echo "==> Merging dependency archives per slice (self-contained static libs)"
# A CMake static library does not bundle its dependencies, so libslugkit_c.a alone has unresolved
# fmt/utf8proc symbols. Combine the three archives per slice into one with libtool -static.
merge_slice() {
    local slice="$1"
    libtool -static -no_warning_for_no_symbols -o "$BUILD/$slice/libslugkit_combined.a" \
        "$BUILD/$slice/libslugkit_c.a" \
        "$BUILD/$slice/_deps/fmt-build/libfmt.a" \
        "$BUILD/$slice/_deps/utf8proc-build/libutf8proc.a"
}
merge_slice macos
merge_slice ios-device
merge_slice ios-sim

echo "==> Staging headers + module map"
HDR="$BUILD/headers"
rm -rf "$HDR"; mkdir -p "$HDR"
cp "$GEN_ROOT/slugkit/include/slugkit/c/slugkit_c.h" "$HDR/"
cat > "$HDR/module.modulemap" <<'MODMAP'
module CSlugkit {
    header "slugkit_c.h"
    export *
}
MODMAP

echo "==> Assembling $OUT"
rm -rf "$OUT"
xcodebuild -create-xcframework \
    -library "$BUILD/macos/libslugkit_combined.a"      -headers "$HDR" \
    -library "$BUILD/ios-device/libslugkit_combined.a" -headers "$HDR" \
    -library "$BUILD/ios-sim/libslugkit_combined.a"    -headers "$HDR" \
    -output "$OUT" >/dev/null

echo "==> Done: $OUT"
