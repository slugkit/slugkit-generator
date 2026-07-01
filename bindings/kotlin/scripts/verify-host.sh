#!/usr/bin/env bash
# Host-JVM verification of the JNI bridge + Kotlin wrapper (no Android device/emulator needed).
# Builds libslugkit_jni for the host, compiles the Kotlin wrapper + Verify.kt, and runs them.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # bindings/kotlin
GEN_ROOT="$(cd "$HERE/../.." && pwd)"                      # generator root
BUILD="$HERE/.build/host-cmake"
KOUT="$HERE/.build/kotlin"

echo "==> Building libslugkit_jni for the host"
cmake -S "$HERE/src/main/cpp" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$BUILD" -j >/dev/null

LIB="$(find "$BUILD" -name 'libslugkit_jni.*' \( -name '*.dylib' -o -name '*.so' \) | head -1)"
LIBDIR="$(dirname "$LIB")"
echo "    built $LIB"

echo "==> Compiling Kotlin wrapper + verifier"
mkdir -p "$KOUT"
kotlinc "$HERE/src/main/kotlin/com/slugkit/Slugkit.kt" "$HERE/scripts/Verify.kt" \
    -include-runtime -d "$KOUT/verify.jar" 2>/dev/null

echo "==> Running verification"
java -Djava.library.path="$LIBDIR" -cp "$KOUT/verify.jar" com.slugkit.VerifyKt \
    "$GEN_ROOT/slugkit/tests/data/emoji.bin" \
    "$GEN_ROOT/slugkit/tests/data/test-adv.slugs"
