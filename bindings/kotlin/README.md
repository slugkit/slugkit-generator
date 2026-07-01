# Slugkit — Kotlin / Android binding

Idiomatic Kotlin wrapper over the SlugKit generator engine, exposed through a
JNI bridge (`libslugkit_jni`) and packaged as an Android library (AAR) that
bundles the native `.so` for each ABI.

## Layout

- `src/main/cpp/slugkit_jni.cpp` — JNI bridge to the `slugkit_c` C ABI.
- `src/main/cpp/CMakeLists.txt` — builds `libslugkit_jni` (JNI + userver-free
  core + C ABI); invoked by Gradle per ABI and by the host verifier.
- `src/main/kotlin/com/slugkit/Slugkit.kt` — the public `Generator` API.
- `build.gradle.kts` — Android library module (AAR) with `externalNativeBuild`
  (CMake) for `arm64-v8a`, `armeabi-v7a`, `x86_64`.
- `scripts/verify-host.sh` + `scripts/Verify.kt` — host-JVM verification.

## Build the AAR

Requires the Android SDK + NDK and Gradle (with an `ANDROID_HOME`):

```sh
cd bindings/kotlin
gradle assembleRelease   # -> build/outputs/aar/slugkit-release.aar
```

## Usage

```kotlin
import com.slugkit.Generator

val dict: ByteArray = context.assets.open("emoji.bin").readBytes()
Generator.fromBinaryDictionary(dict).use { gen ->
    val slug = gen.generate("{emoji}", seed = "foobar", sequence = 0)

    val cap = gen.capacity("{emoji}")          // cap.value (decimal String), cap.maxLength

    val many = gen.generate("{adjective}-{noun}", seed = "s", sequence = 0, count = 100)
}
```

`Generator` is thread-safe for concurrent generation; construct once and reuse.
It is `AutoCloseable` — use `use { ... }` to release the native handle.
Generation is deterministic: the same `(pattern, seed, sequence)` always yields
the same slug.

## Verify without an emulator

The JNI bridge and the Kotlin wrapper can be verified on the host JVM (builds the
native lib for the host, compiles the wrapper with `kotlinc`, runs it against the
committed `emoji.bin`):

```sh
./scripts/verify-host.sh
# -> ALL KOTLIN JNI CHECKS PASSED
```
