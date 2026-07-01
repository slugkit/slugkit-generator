# Slugkit — Swift binding

Idiomatic Swift wrapper over the SlugKit generator engine's C ABI (`slugkit_c`),
distributed as a Swift package backed by a prebuilt **XCFramework** (macOS + iOS
device + iOS simulator).

## Build

The native engine is delivered as `Slugkit.xcframework`, produced from the
userver-free C ABI by a script (it is a build artefact and is git-ignored):

```sh
cd bindings/swift
./scripts/build-xcframework.sh   # builds macOS + iOS device + iOS simulator slices
swift build
swift test
```

The script cross-compiles the core + `slugkit_c` for each Apple platform via the
CMake presets/toolchains, merges the fmt/utf8proc archives into a self-contained
static library per slice, and assembles the XCFramework.

## Usage

```swift
import Slugkit

// Load one or more compiled binary dictionaries (.bin).
let dict = try Data(contentsOf: emojiBinURL)
let generator = try Generator(binaryDictionary: dict)

let slug = try generator.generate("{emoji}", seed: "foobar", sequence: 0)

// Capacity (arbitrary precision, returned as a decimal string).
let cap = try generator.capacity(of: "{emoji}")   // cap.value, cap.maxLength

// Batch generation (no per-slug allocation on the native side).
try generator.generate("{adjective}-{noun}", seed: "s", sequence: 0, count: 100) { slug in
    print(slug)
}
```

`Generator` is thread-safe for concurrent generation; construct once and reuse.
Generation is deterministic: the same `(pattern, seed, sequence)` always yields
the same slug.

## Layout

- `Package.swift` — SwiftPM manifest (binary `CSlugkit` target + `Slugkit` wrapper).
- `Sources/Slugkit/Slugkit.swift` — the public Swift API.
- `Tests/SlugkitTests/` — verifies the wrapper against the committed `emoji.bin`.
- `scripts/build-xcframework.sh` — produces `Slugkit.xcframework`.
