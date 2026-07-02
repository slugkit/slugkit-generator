# slugkit — Flutter plugin

A Flutter **FFI plugin** for the SlugKit generator engine. It bundles the native
C++ engine per platform and exposes an idiomatic Dart API via `dart:ffi` — no
method channels.

This is distinct from `bindings/dart` (a plain Dart package for non-Flutter use):
the plugin adds the Android/iOS/macOS build integration so the native library is
compiled and bundled into your app automatically.

## Usage

```dart
import 'package:flutter/services.dart' show rootBundle;
import 'package:slugkit/slugkit.dart';

final emoji = (await rootBundle.load('assets/emoji.bin')).buffer.asUint8List();
final adverb = (await rootBundle.load('assets/test-adv.slugs')).buffer.asUint8List();

final gen = Generator.fromMultiple([adverb, emoji]);
final slug = gen.generate('{adverb}-{emoji}', 'foobar', 0);   // e.g. "impotently-🐝"
final cap  = gen.capacity('{adverb}-{emoji}');                // cap.value, cap.maxLength
final many = gen.generateBatch('{adverb}-{emoji}', 'foobar', 0, 20);
gen.dispose();
```

Generation is deterministic; `Generator` is thread-safe. Load one or more compiled
binary dictionaries (`.bin`) — see the SlugKit dictionary tooling.

## How the native engine is bundled

- **Android** — `src/CMakeLists.txt` reuses the `slugkit/mobile` build via the
  Android Gradle Plugin's `externalNativeBuild`, producing `libslugkit_c.so` per
  ABI. (Needs CMake ≥ 3.24 and Boost headers available to the NDK build.)
- **iOS / macOS** — the podspecs vendor a prebuilt **dynamic** `Slugkit.xcframework`
  (built by `scripts/build-xcframework.sh`, run automatically by the podspec
  `prepare_command`). Dynamic framework ⇒ the `slk_*` symbols are exported and
  loaded into the process, so `DynamicLibrary.process()` resolves them.

The xcframework is a build artefact (git-ignored); the script reproduces it.

## Example

`example/` is a runnable app (pattern + seed fields, live slug list) that loads
two dictionaries from its assets. Only the app's own code is committed — the
regenerable platform runners are not, so rehydrate them first:

```sh
cd example
flutter create --platforms=android,ios,macos .   # regenerate android/ ios/ macos/
flutter run -d macos                              # or an iOS/Android device
flutter test integration_test/app_test.dart -d macos
```

The integration test asserts the same golden slugs as the C ABI / Swift / Kotlin
/ Dart bindings — byte-identical output through the plugin. Verified on macOS.
