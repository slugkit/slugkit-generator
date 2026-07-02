cmake_minimum_required(VERSION 3.20)
project(slugkit-generator-ci CXX)

# CI-only entry point. The generator library is normally consumed via
# add_subdirectory from the parent monorepo, which sets up userver and adds
# libs/generator/slugkit. For standalone CI we find the userver that the
# userver-build container already provides and build the library plus its
# UTEST suite in isolation. Mirrors the monorepo's userver setup.
# userver's exported targets reference fmt::fmt, so its imported target must exist before
# userver's package config is loaded. (The monorepo pulls fmt in transitively; standalone we
# find it explicitly. Both fmt and userver are installed in the prebuilt image.)
find_package(fmt REQUIRED)
find_package(userver REQUIRED COMPONENTS universal core)
userver_setup_environment()

set(SLUGKIT_GENERATOR_BUILD_TESTS ON CACHE BOOL "" FORCE)
enable_testing()

# The generator's own CMakeLists lives one level up in slugkit/.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../slugkit ${CMAKE_BINARY_DIR}/slugkit)
