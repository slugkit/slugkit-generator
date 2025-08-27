set(BENCH_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(${PROJECT_NAME}_BENCH_SRC
    ${BENCH_SRC_ROOT}/helpers.cpp
    ${BENCH_SRC_ROOT}/permute_benchmark.cpp
    ${BENCH_SRC_ROOT}/parse_benchmark.cpp
    ${BENCH_SRC_ROOT}/format_pattern_benchmark.cpp
    ${BENCH_SRC_ROOT}/generate_numbers_benchmarks.cpp
    ${BENCH_SRC_ROOT}/generate_from_dictionary_benchmark.cpp
    ${BENCH_SRC_ROOT}/filter_dictionary_benchmarks.cpp
    ${BENCH_SRC_ROOT}/generate_slugs_benchmarks.cpp
)
add_executable(slugkit-benchmark ${${PROJECT_NAME}_BENCH_SRC} main.cpp)
target_include_directories(
    slugkit-benchmark 
    PRIVATE 
        ${${PROJECT_NAME}_SRC_ROOT}
)
target_link_libraries(
    slugkit-benchmark 
    PRIVATE
        slugkit-generator
        userver::universal
        userver::utest
        benchmark::benchmark
)
add_google_benchmark_tests(slugkit-benchmark)
