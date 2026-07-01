#!/usr/bin/env bash
# Build a DYNAMIC Slugkit.xcframework (macOS + iOS device + iOS simulator) for the Flutter plugin,
# and place a copy next to each Apple podspec (ios/ and macos/). Dynamic frameworks keep the slk_*
# symbols exported (no dead-strip when embedded), so DynamicLibrary.process() resolves them.
#
# Run automatically by the podspec prepare_command, or manually before `flutter build`.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"          # bindings/flutter/slugkit
GEN_ROOT="$(cd "$HERE/../../.." && pwd)"                          # generator root
MOBILE="$GEN_ROOT/slugkit/mobile"
BUILD="$HERE/.build/xcframework"
OUT="$BUILD/Slugkit.xcframework"

# Idempotent: skip the (slow) rebuild if both podspec copies already exist. Set FORCE=1 to rebuild.
if [ "${FORCE:-0}" != "1" ] && [ -d "$HERE/ios/Slugkit.xcframework" ] && [ -d "$HERE/macos/Slugkit.xcframework" ]; then
    echo "==> Slugkit.xcframework already present (set FORCE=1 to rebuild); skipping"
    exit 0
fi

common=(-DCMAKE_BUILD_TYPE=Release -DSLUGKIT_MOBILE_SHARED=ON -DSLUGKIT_MOBILE_FRAMEWORK=ON)

echo "==> Building dynamic frameworks"
cmake -S "$MOBILE" -B "$BUILD/macos" "${common[@]}" -DCMAKE_OSX_ARCHITECTURES=arm64 >/dev/null
cmake --build "$BUILD/macos" -j >/dev/null

cmake -S "$MOBILE" -B "$BUILD/ios" "${common[@]}" \
    -DCMAKE_TOOLCHAIN_FILE="$GEN_ROOT/cmake/toolchains/ios-device.cmake" >/dev/null
cmake --build "$BUILD/ios" -j >/dev/null

cmake -S "$MOBILE" -B "$BUILD/ios-sim" "${common[@]}" \
    -DCMAKE_TOOLCHAIN_FILE="$GEN_ROOT/cmake/toolchains/ios-simulator.cmake" >/dev/null
cmake --build "$BUILD/ios-sim" -j >/dev/null

echo "==> Assembling $OUT"
rm -rf "$OUT"
xcodebuild -create-xcframework \
    -framework "$BUILD/macos/Slugkit.framework" \
    -framework "$BUILD/ios/Slugkit.framework" \
    -framework "$BUILD/ios-sim/Slugkit.framework" \
    -output "$OUT" >/dev/null

echo "==> Placing copies next to the podspecs"
for platform in ios macos; do
    rm -rf "$HERE/$platform/Slugkit.xcframework"
    cp -R "$OUT" "$HERE/$platform/Slugkit.xcframework"
done
echo "==> Done"
