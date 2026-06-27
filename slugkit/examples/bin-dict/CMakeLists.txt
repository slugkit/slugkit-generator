add_executable(bin-dict main.cpp)

target_link_libraries(bin-dict
    PRIVATE
        Boost::program_options
        slugkit-generator
)
target_include_directories(bin-dict PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
