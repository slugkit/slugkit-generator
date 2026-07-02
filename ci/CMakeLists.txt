cmake_minimum_required(VERSION 3.20)
project(slugkit-generator-ci CXX)

# CI-only entry point. The generator library is normally consumed via
# add_subdirectory from the parent monorepo, which sets up userver and adds
# libs/generator/slugkit. For standalone CI we find the userver that the
# userver-build container already provides and build the library plus its
# UTEST suite in isolation. Mirrors the monorepo's userver setup.
find_package(userver REQUIRED COMPONENTS universal core)
userver_setup_environment()

set(SLUGKIT_GENERATOR_BUILD_TESTS ON CACHE BOOL "" FORCE)
enable_testing()

# The generator's own CMakeLists lives one level up in slugkit/.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../slugkit ${CMAKE_BINARY_DIR}/slugkit)
