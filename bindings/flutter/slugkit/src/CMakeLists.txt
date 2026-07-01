# Android native build for the SlugKit Flutter FFI plugin.
# Reuses the shared slugkit/mobile build (userver-free core + C ABI) to produce libslugkit_c.so,
# which the Android Gradle Plugin packages into the app per ABI.
#
# Note: the mobile build fetches fmt + utf8proc and needs Boost headers (header-only). It requires
# CMake >= 3.24; configure the Android Gradle externalNativeBuild to use a matching CMake version.
cmake_minimum_required(VERSION 3.24)
project(slugkit_flutter C CXX)

set(SLUGKIT_MOBILE_SHARED ON CACHE BOOL "" FORCE)

# src -> generator root: bindings/flutter/slugkit/src -> ../../../..
get_filename_component(GEN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
add_subdirectory("${GEN_ROOT}/slugkit/mobile" "${CMAKE_BINARY_DIR}/slugkit-mobile")

if (ANDROID)
    # Android 15 16 KB page size support.
    target_link_options(slugkit_c PRIVATE "-Wl,-z,max-page-size=16384")
endif()
