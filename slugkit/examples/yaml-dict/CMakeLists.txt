add_executable(yaml-dict main.cpp)

target_link_libraries(yaml-dict
    PRIVATE
        CLI11::CLI11
        fmt::fmt
        slugkit-generator
)
target_include_directories(yaml-dict PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
