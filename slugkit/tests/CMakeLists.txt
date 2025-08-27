set(TEST_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(${PROJECT_NAME}_TEST_SRC
    ${TEST_SRC_ROOT}/numerics_test.cpp
    ${TEST_SRC_ROOT}/roman_test.cpp
    ${TEST_SRC_ROOT}/pattern_test.cpp
    ${TEST_SRC_ROOT}/permutation_test.cpp
    ${TEST_SRC_ROOT}/dictionary_test.cpp
    ${TEST_SRC_ROOT}/generator_test.cpp
    ${TEST_SRC_ROOT}/mix_case_test.cpp
)

add_executable(test_${PROJECT_NAME} ${${PROJECT_NAME}_TEST_SRC})
target_link_libraries(
    test_${PROJECT_NAME}
    PRIVATE 
        ${PROJECT_NAME} 
        userver::utest
)
target_include_directories(
    test_${PROJECT_NAME}
    PRIVATE
        ${${PROJECT_NAME}_INCLUDE_ROOT}
        ${${PROJECT_NAME}_SRC_ROOT}
)
add_google_tests(test_${PROJECT_NAME})
