cmake_minimum_required(VERSION 3.20)
project(slugkit-host-spike C CXX)

# Phase 0 de-risk spike: build the generator core WITHOUT userver, on the macOS host
# (clang/libc++). Deliberately does NOT define SLUGKIT_USE_USERVER (so the compat shims use their
# standalone branches) and does NOT define SLUGKIT_GENERATOR_WITH_SERIALIZATION (numeric I/O is
# gated out and not needed on mobile/host).

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Root of the generator subtree (one level up from this host/ dir).
get_filename_component(SLUGKIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

find_package(fmt REQUIRED)
find_package(Boost REQUIRED)

# utf8proc: prefer a CMake/pkg config package, fall back to the Homebrew location.
find_package(utf8proc QUIET)
if(NOT utf8proc_FOUND)
    find_path(UTF8PROC_INCLUDE_DIR utf8proc.h
        HINTS /opt/homebrew/opt/utf8proc/include /usr/local/opt/utf8proc/include)
    find_library(UTF8PROC_LIBRARY utf8proc
        HINTS /opt/homebrew/opt/utf8proc/lib /usr/local/opt/utf8proc/lib)
    if(NOT UTF8PROC_INCLUDE_DIR OR NOT UTF8PROC_LIBRARY)
        message(FATAL_ERROR "utf8proc not found")
    endif()
    add_library(utf8proc::utf8proc UNKNOWN IMPORTED)
    set_target_properties(utf8proc::utf8proc PROPERTIES
        IMPORTED_LOCATION "${UTF8PROC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${UTF8PROC_INCLUDE_DIR}")
endif()

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

add_library(slugkit-generator-host STATIC ${CORE_SOURCES})
target_include_directories(slugkit-generator-host PUBLIC
    ${SLUGKIT_DIR}/include
    ${SLUGKIT_DIR}/src
)
target_link_libraries(slugkit-generator-host PUBLIC
    fmt::fmt
    utf8proc::utf8proc
    Boost::headers
)

enable_testing()
find_package(GTest REQUIRED)

add_executable(golden_test ${CMAKE_CURRENT_SOURCE_DIR}/golden_test.cpp)
target_link_libraries(golden_test PRIVATE
    slugkit-generator-host
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME golden_test COMMAND golden_test)

# C ABI smoke test -- compiled as C to prove slugkit_c.h is valid C, linked against the C++ core.
add_executable(c_abi_test ${CMAKE_CURRENT_SOURCE_DIR}/c_abi_test.c)
set_target_properties(c_abi_test PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(c_abi_test PRIVATE
    SLK_EMOJI_BIN_PATH="${SLUGKIT_DIR}/tests/data/emoji.bin"
    SLK_ADVERB_BIN_PATH="${SLUGKIT_DIR}/tests/data/test-adv.slugs")
target_link_libraries(c_abi_test PRIVATE slugkit-generator-host)
add_test(NAME c_abi_test COMMAND c_abi_test)
