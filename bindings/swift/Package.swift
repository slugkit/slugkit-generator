// swift-tools-version:5.9
import PackageDescription

// Slugkit Swift package.
//
// The native engine is delivered as a prebuilt XCFramework (macOS + iOS device + iOS simulator),
// produced by scripts/build-xcframework.sh. Run that script before `swift build`/`swift test`.
let package = Package(
    name: "Slugkit",
    platforms: [.macOS(.v11), .iOS(.v13)],
    products: [
        .library(name: "Slugkit", targets: ["Slugkit"]),
    ],
    targets: [
        // Prebuilt C ABI (libslugkit_c + core) with a module map exposing slugkit_c.h as `CSlugkit`.
        .binaryTarget(name: "CSlugkit", path: "Slugkit.xcframework"),
        // Idiomatic Swift wrapper over the C ABI. The native lib is C++ (fmt, engine), so link libc++.
        .target(
            name: "Slugkit",
            dependencies: ["CSlugkit"],
            linkerSettings: [.linkedLibrary("c++")]
        ),
        .testTarget(name: "SlugkitTests", dependencies: ["Slugkit"]),
    ]
)
