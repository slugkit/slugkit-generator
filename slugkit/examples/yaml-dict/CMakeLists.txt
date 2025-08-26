add_executable(yaml-dict main.cpp)

target_link_libraries(yaml-dict 
    PRIVATE 
        Boost::program_options
        slugkit-generator
)
target_include_directories(yaml-dict PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
