cmake_minimum_required(VERSION 3.24)
project(slugkit_jni CXX)

# Builds libslugkit_jni.so: the JNI bridge plus the userver-free core + C ABI.
# Invoked by Gradle (externalNativeBuild) per Android ABI, and by scripts/verify-host.sh on the host.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Bring in the slugkit_c target (core + C ABI; fetches fmt/utf8proc). Path: this dir is
# bindings/kotlin/src/main/cpp -> five levels up to the generator root.
get_filename_component(GEN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
add_subdirectory("${GEN_ROOT}/slugkit/mobile" "${CMAKE_BINARY_DIR}/slugkit-mobile")

add_library(slugkit_jni SHARED slugkit_jni.cpp)
target_link_libraries(slugkit_jni PRIVATE slugkit_c)

if(ANDROID)
    # The NDK toolchain provides jni.h and the log lib.
    find_library(log-lib log)
    target_link_libraries(slugkit_jni PRIVATE ${log-lib})
else()
    # Host build (verification): locate the JDK's JNI headers.
    find_package(JNI REQUIRED)
    target_include_directories(slugkit_jni PRIVATE ${JNI_INCLUDE_DIRS})
endif()
