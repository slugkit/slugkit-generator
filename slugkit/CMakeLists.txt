cmake_minimum_required(VERSION 3.20)
project(slugkit-generator CXX)

set(${PROJECT_NAME}_INCLUDE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(${PROJECT_NAME}_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Utils library
set(${PROJECT_NAME}_UTILS_SRC
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/numeric.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/roman.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/text.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/utils/primes.cpp
)

add_library(${PROJECT_NAME}-utils STATIC
    ${${PROJECT_NAME}_UTILS_SRC}
)
target_link_libraries(${PROJECT_NAME}-utils
    PUBLIC
        userver::core
)
target_include_directories(${PROJECT_NAME}-utils
    PUBLIC
        ${${PROJECT_NAME}_INCLUDE_ROOT}
)

# Generator library
set(${PROJECT_NAME}_SRC
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/permutations.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/dictionary.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/pattern.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/pattern_generator.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/generator.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/generator/detail/indexes.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${${PROJECT_NAME}_SRC}
)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
        ${PROJECT_NAME}-utils
)
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

set(${PROJECT_NAME}_TEST_UTILS_SRC
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/helpers.cpp
    ${${PROJECT_NAME}_SRC_ROOT}/slugkit/test_utils/helpers.hpp
)

add_library(${PROJECT_NAME}-test-utils STATIC
    ${${PROJECT_NAME}_TEST_UTILS_SRC}
)
target_include_directories(
    ${PROJECT_NAME}-test-utils
    PUBLIC
        ${${PROJECT_NAME}_INCLUDE_ROOT}
        ${${PROJECT_NAME}_SRC_ROOT}
)
target_link_libraries(
    ${PROJECT_NAME}-test-utils
    PUBLIC
        ${PROJECT_NAME}
)


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
