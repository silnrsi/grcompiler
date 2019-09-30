cmake_minimum_required(VERSION 3.0)
include(TestBigEndian)
include(GNUInstallDirs)
project(grcompiler)
enable_language(CXX C)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 11)

enable_testing()

if (NOT CMAKE_BUILD_TYPE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE Release)
endif()

add_subdirectory(doc)
add_subdirectory(preprocessor)
add_subdirectory(compiler)
add_subdirectory(test)

set(version 5.1)