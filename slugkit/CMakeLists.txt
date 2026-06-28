cmake_minimum_required(VERSION 3.20)
project(slugkit-generator CXX)

set(${PROJECT_NAME}_INCLUDE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(${PROJECT_NAME}_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Build the generator against userver (vocabulary types, formats serialisation).
# OFF produces a standalone build via the slugkit/compat shims (see slugkit/host
# for a self-contained example). This drives pimpl storage sizes in PUBLIC headers,
# so it MUST be propagated PUBLIC to every consumer.
option(SLUGKIT_USE_USERVER "Build the generator against userver" ON)

if (SLUGKIT_USE_USERVER)
    # Serialisation (BigInt Parse/Serialize, formats IO) is only available with userver.
    set(${PROJECT_NAME}_COMPILE_DEFS SLUGKIT_USE_USERVER SLUGKIT_GENERATOR_WITH_SERIALIZATION)
else()
    set(${PROJECT_NAME}_COMPILE_DEFS)
endif()

# utf8proc backs case conversion in utils/text.cpp for BOTH builds (replaced boost::locale/ICU).
find_package(utf8proc CONFIG QUIET)
if (NOT TARGET utf8proc::utf8proc)
    find_path(UTF8PROC_INCLUDE_DIR utf8proc.h REQUIRED)
    find_library(UTF8PROC_LIBRARY NAMES utf8proc REQUIRED)
    add_library(utf8proc::utf8proc UNKNOWN IMPORTED)
    set_target_properties(utf8proc::utf8proc PROPERTIES
        IMPORTED_LOCATION "${UTF8PROC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${UTF8PROC_INCLUDE_DIR}")
endif()

# Utils library
set(${PROJECT_NAME}_UTILS_SRC
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/numeric.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/roman.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/text.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/primes.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/memory_mapped_file.cpp
)

add_library(${PROJECT_NAME}-utils STATIC
    ${${PROJECT_NAME}_UTILS_SRC}
)
target_link_libraries(${PROJECT_NAME}-utils
    PUBLIC
        utf8proc::utf8proc
)
if (SLUGKIT_USE_USERVER)
    target_link_libraries(${PROJECT_NAME}-utils PUBLIC userver::core)
endif()
target_compile_definitions(${PROJECT_NAME}-utils PUBLIC ${${PROJECT_NAME}_COMPILE_DEFS})
target_include_directories(${PROJECT_NAME}-utils
    PUBLIC
        ${${PROJECT_NAME}_INCLUDE_ROOT}
)

# Generator library
set(${PROJECT_NAME}_SRC
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/permutations.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/dictionary.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/binary_dictionary.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/pattern.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/placeholders.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/pattern_generator.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/generator.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/detail/pattern_parser.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/detail/indexes.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/filter/filtered_ids.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${${PROJECT_NAME}_SRC}
)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
        ${PROJECT_NAME}-utils
)
target_compile_definitions(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_COMPILE_DEFS})
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${${PROJECT_NAME}_INCLUDE_ROOT}
)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${${PROJECT_NAME}_SRC_ROOT}
)


if (SLUGKIT_GENERATOR_BUILD_TESTS OR SLUGKIT_GENERATOR_BUILD_BENCHMARKS OR SLUGKIT_GENERATOR_BUILD_EXAMPLES)
    set(
        ${PROJECT_NAME}-test-dictionary-data
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/test-adv.slugs
    )
    userver_embed_file(
        ${PROJECT_NAME}-dictionary_test_data
        NAME dictionary_test_data
        FILEPATH ${${PROJECT_NAME}-test-dictionary-data}
        )
    userver_embed_file(
        ${PROJECT_NAME}-emoji_test_data
        NAME emoji_test_data
        FILEPATH ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/emoji.bin
        )
    set(DICTIONARY_TEST_FILE "${${PROJECT_NAME}-test-dictionary-data}")
    configure_file(
        ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/data_config.in.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/slugkit/include/slugkit/test_utils/data_config.hpp
        @ONLY
    )
    set(${PROJECT_NAME}_TEST_UTILS_SRC
        ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/helpers.cpp
        ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/helpers.hpp
        ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/test_dictionary.cpp
        ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/test_dictionary.hpp
    )

    add_library(${PROJECT_NAME}-test-utils STATIC
        ${${PROJECT_NAME}_TEST_UTILS_SRC}
    )
    target_include_directories(
        ${PROJECT_NAME}-test-utils
        PUBLIC
            ${${PROJECT_NAME}_INCLUDE_ROOT}
            ${${PROJECT_NAME}_SRC_ROOT}
            ${CMAKE_CURRENT_BINARY_DIR}/slugkit/include
    )
    target_link_libraries(
        ${PROJECT_NAME}-test-utils
        PUBLIC
            ${PROJECT_NAME}
            # test_dictionary.cpp includes the generated embed headers; linking the embed targets
            # provides their include dirs and orders their generation before this library compiles.
            ${PROJECT_NAME}-dictionary_test_data
            ${PROJECT_NAME}-emoji_test_data
    )
endif()


if(SLUGKIT_GENERATOR_BUILD_TESTS)
    add_subdirectory(tests)
else()
    message(STATUS "Skipping tests, set SLUGKIT_GENERATOR_BUILD_TESTS to ON to build them")
endif()

if(SLUGKIT_GENERATOR_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
else()
    message(STATUS "Skipping benchmarks, set SLUGKIT_GENERATOR_BUILD_BENCHMARKS to ON to build them")
endif()

if(SLUGKIT_GENERATOR_BUILD_EXAMPLES)
    add_subdirectory(examples)
else()
    message(STATUS "Skipping examples, set SLUGKIT_GENERATOR_BUILD_EXAMPLES to ON to build them")
endif()
