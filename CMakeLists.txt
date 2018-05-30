cmake_minimum_required(VERSION 3.2)

project(bulkmt VERSION 1.0.$ENV{TRAVIS_BUILD_NUMBER})
configure_file(include/version.h.in ../include/version.h)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

enable_testing()
