# Android (NDK) toolchain wrapper for the slugkit generator.
# Resolves the NDK location, then chains to the NDK's own android.toolchain.cmake.
#
# Usage:
#   export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/<version>
#   cmake -S slugkit/mobile -B build/android-arm64 \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/android.cmake \
#         -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24
#
# ANDROID_ABI: arm64-v8a | armeabi-v7a | x86_64 | x86  (default arm64-v8a)

if(NOT ANDROID_NDK)
    foreach(_var ANDROID_NDK_HOME ANDROID_NDK_ROOT NDK_ROOT)
        if(DEFINED ENV{${_var}} AND EXISTS "$ENV{${_var}}")
            set(ANDROID_NDK "$ENV{${_var}}")
            break()
        endif()
    endforeach()
endif()

if(NOT ANDROID_NDK OR NOT EXISTS "${ANDROID_NDK}")
    message(FATAL_ERROR
        "Android NDK not found. Set ANDROID_NDK_HOME (or pass -DANDROID_NDK=/path/to/ndk).")
endif()

if(NOT ANDROID_ABI)
    set(ANDROID_ABI "arm64-v8a")
endif()
if(NOT ANDROID_PLATFORM)
    set(ANDROID_PLATFORM "android-24")
endif()

# Modern C++/STL and static libc++ so the .so is self-contained.
set(ANDROID_STL "c++_static")

include("${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
