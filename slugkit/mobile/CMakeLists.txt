cmake_minimum_required(VERSION 3.24)
project(slugkit-mobile CXX C)

# Cross-compilable, userver-free build of the generator core + the slugkit_c C ABI, packaged as a
# library for mobile/embedded targets (iOS, Android, wasm, host). Select a target by passing a
# toolchain / preset (see cmake/toolchains and CMakePresets.json at the generator root).
#
# Dependencies are made self-contained so the build works on any target:
#   - fmt and utf8proc are fetched and compiled from source (correct target ABI);
#   - boost is header-only here (after dropping boost::locale/ICU), so only an include dir is needed.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build a shared library (e.g. Android .so) instead of a static archive.
option(SLUGKIT_MOBILE_SHARED "Build slugkit_c as a shared library" OFF)

get_filename_component(SLUGKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# --- Boost (header-only) -------------------------------------------------------------------------
# Point at any Boost include tree; header-only components (multiprecision, functional/hash,
# algorithm, range) are platform-independent, so a host Boost checkout is fine for cross builds.
set(SLUGKIT_BOOST_INCLUDE_DIR "" CACHE PATH "Path to Boost headers (contains boost/)")
if(NOT SLUGKIT_BOOST_INCLUDE_DIR)
    # Separate find variable: find_path no-ops if its result var is already a set cache entry.
    # NO_CMAKE_FIND_ROOT_PATH: boost here is header-only and host-independent, so allow finding a
    # host boost checkout even when a cross toolchain (Android/iOS/wasm) restricts finds to its sysroot.
    find_path(SLUGKIT_BOOST_INCLUDE_DIR_FOUND boost/multiprecision/cpp_int.hpp
        HINTS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
        NO_CMAKE_FIND_ROOT_PATH)
    if(SLUGKIT_BOOST_INCLUDE_DIR_FOUND)
        set(SLUGKIT_BOOST_INCLUDE_DIR "${SLUGKIT_BOOST_INCLUDE_DIR_FOUND}")
    endif()
endif()
if(NOT SLUGKIT_BOOST_INCLUDE_DIR)
    message(FATAL_ERROR "Boost headers not found; set -DSLUGKIT_BOOST_INCLUDE_DIR=/path/to/boost/include")
endif()
message(STATUS "slugkit-mobile: Boost headers at ${SLUGKIT_BOOST_INCLUDE_DIR}")

# --- fmt + utf8proc (compiled from source for the target) ----------------------------------------
include(FetchContent)

set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.1.4
    GIT_SHALLOW TRUE)

FetchContent_Declare(utf8proc
    GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
    GIT_TAG v2.9.0
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(fmt utf8proc)

# --- Core sources (same set as the host spike) ---------------------------------------------------
set(CORE_SOURCES
    ${SLUGKIT_DIR}/src/slugkit/utils/numeric.cpp
    ${SLUGKIT_DIR}/src/slugkit/utils/roman.cpp
    ${SLUGKIT_DIR}/src/slugkit/utils/text.cpp
    ${SLUGKIT_DIR}/src/slugkit/utils/primes.cpp
    ${SLUGKIT_DIR}/src/slugkit/utils/memory_mapped_file.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/permutations.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/dictionary.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/binary_dictionary.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/pattern.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/placeholders.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/pattern_generator.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/generator.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/detail/pattern_parser.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/detail/indexes.cpp
    ${SLUGKIT_DIR}/src/slugkit/generator/filter/filtered_ids.cpp
    ${SLUGKIT_DIR}/src/slugkit/c/slugkit_c.cpp
)

if(SLUGKIT_MOBILE_SHARED)
    add_library(slugkit_c SHARED ${CORE_SOURCES})
else()
    add_library(slugkit_c STATIC ${CORE_SOURCES})
endif()

# No SLUGKIT_USE_USERVER / SLUGKIT_GENERATOR_WITH_SERIALIZATION -> standalone compat branches.
target_include_directories(slugkit_c
    PUBLIC
        ${SLUGKIT_DIR}/include
    PRIVATE
        ${SLUGKIT_DIR}/src
        ${SLUGKIT_BOOST_INCLUDE_DIR})
target_link_libraries(slugkit_c PRIVATE fmt::fmt utf8proc)

# Keep the C ABI symbols visible in the shared library even under -fvisibility=hidden.
set_target_properties(slugkit_c PROPERTIES
    C_VISIBILITY_PRESET default
    CXX_VISIBILITY_PRESET default)
