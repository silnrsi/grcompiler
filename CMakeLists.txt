cmake_minimum_required(VERSION 3.11)
project(grcompiler)
enable_language(CXX C)
enable_testing()

include(TestBigEndian)
include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 14)
include(GNUInstallDirs)

test_big_endian(BIGENDIAN_SYSTEM)

if (NOT CMAKE_BUILD_TYPE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE Release)
endif()

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DWIN32 -D_WINDOWS)

    if (NOT DEFINED ICU_ROOT AND NOT DEFINED FETCHCONTENT_SOURCE_DIR_ICU)
        string(REGEX REPLACE "^x64$" "Win64" _icu_platform_name ${CMAKE_VS_PLATFORM_NAME})
        set(ICU_URL https://github.com/unicode-org/icu/releases/download/release-74-2/icu4c-74_2-${_icu_platform_name}-MSVC2019.zip)
    endif()
endif()

add_subdirectory(doc)
add_subdirectory(preprocessor)
add_subdirectory(compiler)
add_subdirectory(test)
