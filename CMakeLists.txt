cmake_minimum_required(VERSION 3.11)
include(TestBigEndian)
include(GNUInstallDirs)
project(grcompiler)
include(CTest)

enable_language(CXX C)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 11)

enable_testing()

if (NOT CMAKE_BUILD_TYPE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE Release)
endif()

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DWIN32 -D_WINDOWS)

    if (NOT DEFINED ICU_ROOT)
        find_program(NUGET nuget)
        if(NOT NUGET)
            file(DOWNLOAD https://dist.nuget.org/win-x86-commandline/latest/nuget.exe ${CMAKE_BINARY_DIR}/nuget.exe)
            find_program(NUGET nuget PATHS ${CMAKE_BINARY_DIR})
            if (NOT NUGET)
                message(FATAL_ERROR "CMake could not find the nuget command line tool. Please install it or provide ICU_ROOT.")
            endif()
        endif()
        
        # Copy the Nuget config file from source location to the CMake build directory.
        configure_file(packages.config.in packages.config COPYONLY)
        # Run Nuget using the .config file to install any missing dependencies to the build directory.
        execute_process(
            COMMAND ${NUGET} restore packages.config -SolutionDirectory ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
endif()

add_subdirectory(doc)
add_subdirectory(preprocessor)
add_subdirectory(compiler)
add_subdirectory(test)

set(version 5.1)