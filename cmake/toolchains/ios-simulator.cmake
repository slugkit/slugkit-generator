# iOS simulator (arm64 + x86_64) toolchain for the slugkit generator.
# Usage: cmake -S slugkit/mobile -B build/ios-sim -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios-simulator.cmake

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT iphonesimulator CACHE STRING "iOS simulator SDK")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "iOS simulator architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum iOS version")

set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
