# yaml-dict loads dictionaries through userver's YAML formats, so it only builds in the userver
# build. The userver-free path is: compile YAML -> .bin with `compile-dict`, then use the `slugkit`
# CLI (SLUGKIT_GENERATOR_BUILD_CLI).
if(SLUGKIT_USE_USERVER)
    add_subdirectory(yaml-dict)
else()
    message(STATUS "Skipping yaml-dict example (requires userver for YAML loading)")
endif()
