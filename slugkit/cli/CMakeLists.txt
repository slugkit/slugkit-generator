add_executable(slugkit main.cpp)

target_link_libraries(slugkit
    PRIVATE
        slugkit-generator
        CLI11::CLI11
        fmt::fmt
)
