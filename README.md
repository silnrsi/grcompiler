# SILGRAPHITE COMPILER

The SIL Graphite compiler builds a Graphite enabled font from a smart font
description, written in GDL (Graphite Description Language) and a TrueType
font by adding extra TrueType tables to it which the Graphite engine can 
interpret.

## BUILDING

The grcompiler can currently be built with several build systems.  The primary
cross-platform build system is CMake.  We require CMake 3.0 or above.

### CMake

#### For all platforms

1. Create your build directory 
    ```
    mkdir build
    cd build
    ```

2. Generate project files for your build system
    You may need to specify the CMAKE_BUILD_TYPE as some Windows generators require it.
    ```
    cmake -DCMAKE_BUILD_TYPE=Release ..
    ```
    CMake will automatically detect your build system and generate a project for
    that. You may wish to specify a build system other than the automatically
    detected one, for example, if you have multiple versions of Visual Studio
    installed or other toolchain such as MinGW you wish build under. To do this
    pass the `-G <generator name>` option to the initial cmake configuration call,
    for example for Visual Studio 8:  
    ```
    cmake -G "Visual Studio 12 2013" -DCMAKE_BUILD_TYPE=Release ..
    ```

    or for MinGW:  
    ```
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE:STRING=Release ..
    ```
    TIP: You can get a list of generators CMakes supports with `cmake --help`.

3. Build grcompiler binaries
    ```
    cmake --build .
    ```
    When building using the Visual Studio generator you will need to append
    `--config Debug` or `--config Release` for your debug or release builds,
    respectively, to the above command. Depending on your chosen generator the
    next step varies, for MS Visual Studio projects you will need to run:   
    `cmake --build . --target RUN_TESTS`  
    but for all other generators:  
    `cmake --build . --target test`  
    will be sufficient.

4. Installation
    ```
    cmake --build . --target install
    ```
    The step will copy files into a GNU style heirachy rooted at
    `CMAKE_INSTALL_PREFIX` which can be added the the project generation
    command using `-DCMAKE_INSTALL_PREFIX=<path to target dir>`.  This command
    only really makes sense on a Unix system or if build as a dependency
    as part of another Windows project.

5. Rebuilds
    You can clean the project with:
    ```
    cmake --build . --target clean
    ```
    Or just delete the build directory and start again.


#### Linux specific details

On amd64 architecture if you wish build and test 32 bit binaries this is
possible using the following cmake invocation:
```
CFLAGS=-m32 CXXFLAGS=-m32 cmake ..
cmake --build .
cmake --build test
```
You will need g++-multilib support.

It is possible to use clang to build and test grcompiler and gdlpp. Use this
build command:
```
CC=clang CXX=clang++ cmake ..
cmake --build .
```
You will need libc++ and libc++-abi packages.

#### Windows specific details

Visual Studio 2017 added support for handling CMake projects, you can find
more information and instructions at 
[CMake projects in Visual Studio](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019).

### Older build systems

#### For Unix style system with autotools

The grcompiler project can use GNU autobuild tools to manage building and 
installation, please see the INSTALL file for more details.


#### For Win32 older build tools

To build release binaries:  
    ```
    nmake -f makefile.mak
    ```

To build debug binaries:  
    ```
    nmake CFG=DEBUG -f makefile.mak
    ```

Cleaning up, to remove all .obj files without removing the binaries:  
    ```
    nmake -f makefile.mak clean
    ```

To remove the binaries as well:  
    ```
    nmake -f makefile.mak realclean
    ```

this deletes the libraries as well.


## DEPENDENCIES
### ICU

#### Linux

You should use your distributions package manager to install icu-dev package.

- CMake:
    It will automatically find the system dev package installed by you package
    manager. Setting the CMake variable ICU_ROOT to an abolute path will
    override this process and use the built copy located at the provided path.
    This can be set by passing `-DICU_ROOT=<path to built ICU files>` to the
    `cmake` configuration invocation.
- autotools:
    This will auto detect the icu installation via the use of pkg-config
    To overide the detection you need to provide compiler and linker flags via
    the ICU_CFLAGS and ICU_LIBS environment variables.


#### Windows

The Graphite compiler requires library modules from ICU. You will need to
download the ICU source code from the following web site:  
http://www.icu-project.org/download/
    
There is a VisualStudio file in the source\allinone directory that can be
used to build the binaries. The "common" project is the one to build.

- CMake:  
    The CMakeLists.txt will automatically fetch the icu4c.v140 nuget package
    for you and also a copy of nuget if it's not installed.  

- Nmake:  
    The only modules that are needed at this time are icuuc.lib and icuuc63.dll
    (icuucd.lib and icuuc63d.dll for the debug version). The .lib files
    are apparently stubs that call the DLLs. The following source header
    files are needed as well:

    - uchar.h (#included in main.h)
    - pwin32.h
    - uconfig.h
    - umachine.h
    - urename.h
    - utf8.h
    - utf16.h
    - utf.h
    - utf_old.h
    - utypes.h
    - uversion.h

    The makefile expects to find these in icu/source/common.