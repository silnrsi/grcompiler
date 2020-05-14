cmake_minimum_required(VERSION 3.10)
project(grcompiler)
enable_language(CXX C)
enable_testing()

include(TestBigEndian)
include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 11)
include(GNUInstallDirs)



if (NOT CMAKE_BUILD_TYPE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE Release)
endif()

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DWIN32 -D_WINDOWS)

    if (NOT DEFINED ICU_ROOT)
        set(ICU_URL https://github.com/unicode-org/icu/releases/download/release-66-1/icu4c-66_1-Win32-MSVC2017.zip)
    endif()
endif()

add_subdirectory(doc)
add_subdirectory(preprocessor)
add_subdirectory(compiler)
add_subdirectory(test)

set(version 5.1)