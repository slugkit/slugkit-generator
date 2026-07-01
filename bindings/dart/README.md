# Slugkit — Dart / Flutter binding

Idiomatic Dart wrapper over the SlugKit generator engine's C ABI (`slugkit_c`),
using `dart:ffi` to bind the native library directly (no platform channel).

## Layout

- `lib/slugkit.dart` — the public `Generator` API.
- `lib/src/ffi_bindings.dart` — `dart:ffi` lookups over `slugkit_c`.
- `test/slugkit_test.dart` — verifies the binding against the committed `emoji.bin`.
- `scripts/build-native.sh` — builds the shared `libslugkit_c` for the host.

## Usage

```dart
import 'dart:io';
import 'package:slugkit/slugkit.dart';

final dict = File('emoji.bin').readAsBytesSync();
final gen = Generator.fromBytes(dict);

final slug = gen.generate('{emoji}', 'foobar', 0);

// Several dictionaries (e.g. an adjective + a noun dictionary for {adjective}-{noun}):
final multi = Generator.fromMultiple([adjectiveBin, nounBin]);

final cap = gen.capacity('{emoji}');           // cap.value (decimal String), cap.maxLength

final many = gen.generateBatch('{adjective}-{noun}', 's', 0, 100);

gen.dispose();
```

Generation is deterministic: the same `(pattern, seed, sequence)` always yields
the same slug. `Generator` is backed by a native handle — call `dispose()` when
done.

### Loading the native library

`Generator.fromBytes` opens `libslugkit_c` by its platform default name; pass
`libraryPath:` to load an explicit file. In a Flutter app the shared library is
bundled per platform (built from `slugkit/mobile` with `-DSLUGKIT_MOBILE_SHARED=ON`,
reusing the iOS/Android artefacts).

## Test (host)

```sh
cd bindings/dart
./scripts/build-native.sh   # builds .build/host/libslugkit_c.dylib|.so
dart pub get
dart test                   # 6/6 pass against emoji.bin
```
