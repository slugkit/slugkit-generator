# iOS device (arm64) toolchain for the slugkit generator.
# Usage: cmake -S slugkit/mobile -B build/ios -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios-device.cmake
# Relies on CMake's built-in iOS support (>= 3.14) driving the Xcode iPhoneOS SDK.

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT iphoneos CACHE STRING "iOS device SDK")
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "iOS device architecture")
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum iOS version")

# Static library output; no code signing needed for a library artifact.
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
