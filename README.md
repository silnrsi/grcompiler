# SILGRAPHITE COMPILER

The SIL Graphite compiler builds a Graphite enabled font from a smart font
description, written in GDL (Graphite Description Language) and a TrueType
font by adding extra TrueType tables to it which the Graphite engine can
interpret.

This project contains three executables: grcompiler, gdlpp, and
GrcRegressionTest. This project also depends on the International Components
for Unicode (ICU) library. ANTLR is used to generate the GDL parser, though
most developers can use the provided generated files. (See the
compiler/Grammar/Antlr folder.) LZ4 is also used to compress some TrueType
tables.

- gdlpp is a preprocessor for the GDL language that grcompiler invokes during
compilation. On Linux, it should be on the user's path. On Windows, the full
path to it should be specified using the GDLPP environment variable
(TIP: Do not put quotes around the path to gdlpp.exe. DOS-style short path
naming may be needed.) or placed in the same folder as grcompiler.

- GrcRegressionTest is used to regression test grcompiler against a set of
reference GDL files and fonts. The regression tests are typically ran using 
project files (see below).

### GDLPP #include details

WARNING: File inclusion is relative to the current working directory when
grcompiler (or gdlpp) is ran. (It is NOT relative to the folder containing the
including file.)

It's possible to specify a folder that gdlpp will use to find included files
(such as stddef.gdh). Set the GDLPP_PREFS environment variable to 
`-I<folder path>`. There must be no space after `I` and no quote marks. Use
DOS-style short path naming if needed.

## BUILDING

The grcompiler can currently be built with several build systems. The primary
cross-platform build system is CMake.  We require CMake 3.11 or above. It will
build all three executables and can run the regression tests.

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
    detected one -- for example, if you have multiple versions of Visual Studio
    installed or another toolchain, such as MinGW, you wish to build under. To do this
    pass the `-G <generator name>` option to the initial cmake configuration call.
    On Windows it's often desirable to build 32-bit executables. To do this, pass
    the `-A Win32` option.
    For example for Visual Studio 2019:  
    ```
    cmake -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release -A Win32 ..
    ```

    or for MinGW:  
    ```
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE:STRING=Release ..
    ```
    TIPS: You can get a list of generators CMakes supports with `cmake --help`.  
    If you want to run `cmake` with different options, you should do so in an
    empty folder.  
    See below if you want to use a specific version of ICU.

3. Build grcompiler binaries
    ```
    cmake --build .
    ```
    When building using the Visual Studio generator you will need to append
    `--config Debug` or `--config Release` for your debug or release builds,
    respectively, to the above command.

    Depending on your chosen generator the next step varies.  
    For MS Visual Studio projects you will need to run:  
    `cmake --build . --target RUN_TESTS --config Release`  
    but for all other generators:  
    `cmake --build . --target test`  
    will be sufficient.

4. Installation
    ```
    cmake --build . --target install
    ```
    The step will copy files into a GNU style hierarchy rooted at
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

Visual Studio 2017 added support for handling CMake projects. You can find 
more information and instructions at 
[CMake projects in Visual Studio](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019).

TIPS:  
- After generating the project files and optionally doing inital building and 
testing (above), the easiest way for Visual Studio developers to proceed is to 
open the `grcompiler.sln` file that cmake generates.

- The Solution Explorer will contain projects for `grcompiler`, `gdlpp`, and 
`GrcRegressionTest`, as well as several other projects, some of which are just 
empty standard test targets that cmake generates.

- Do NOT switch the Solution Explorer to Folder View as that will cause cmake 
project generation to be redone using the Ninja generator (see below).

- Right clicking on the grcompiler project and setting it as the Startup
Project is advisable.

- Right clicking on ALL_BUILD and Building will build all three executables.

- Right clicking on RUN_TESTS and Building will run the regression tests.

If development with cmake is preferred, in Visual Studio 2019,`File-Open-CMake...`
can be used to open the CMakeCache.txt file. Using `File-Open Folder...' is not
advisable, since the Ninja project generator will be used. It seems buggy.
FWIW, it's harder to specify the arguments to use for debugging with cmake 
development compared to using the sln file.

### Older build systems

#### For Unix style system with autotools

The grcompiler project can use GNU autobuild tools to manage building and
installation, please see the INSTALL file for more details.

#### For Win32 older build tools

The CMake approach above is strongly encouraged.

The choice of 32- or 64-bit build tools and targets is made by building
from the appropriate Visual Studio command prompt.

To build grcomiler release binaries, from the `compiler` folder:  
    ```
    nmake -f makefile.mak
    ```

To build grcompiler debug binaries:  
    ```
    nmake CFG=DEBUG -f makefile.mak
    ```

WARNING: A debug build for ICU from source will be needed too (see below).

Cleaning up, to remove all .obj files without removing the binaries:  
    ```
    nmake -f makefile.mak clean
    ```

To remove the binaries as well:  
    ```
    nmake -f makefile.mak realclean
    ```
This deletes the libraries as well.  

To build gdlpp, from the `preprocessor` folder:  
    ```
    nmake -f gdlpp.mak
    ```

To build GrcRegressionTest and run regression tests, from the 
`test/GrcRegressionTest` folder:  
    ```
    nmake -f Makefile.vc
    cd fonts
    nmake -f regtest.mak
    ```

To use Visual Studio, setup a new makefile project and add commands
for building, testing, and debugging using the makefiles indicated above.

## DEPENDENCIES
### ICU

The grcompiler executable has a hard build dependency on ICU.  This 
dependency may be satisfied via a system supplied dev package as is common on 
Linux, or via a pre-built binary distribution archive available from the  the 
[ICU4C project](http://site.icu-project.org/download/). 

- CMake:
    This will automatically download an unpack any archive URL passed to it 
    via `ICU_URL`. It will search for ICU includes and libraries in the root 
    of that archive expecting to find `include`, `lib` and `bin` directories.
    If those dirs are deeper inside the archive (e.g. inside usr/local) then 
    passing the relative archive path in `ICU_ROOT` will cause it search 
    there. If you have a local binary copy, e.g. if you are patching ICU too, 
    you can have it use that instead by passing the path in 
    `FETCHCONTENT_SOURCE_DIR_ICU`. Lastly passing just `ICU_ROOT` as a 
    semi-colon separated list of paths to search allows you to use a ICU 
    distribution where executables, includes and libraries aren't neatly 
    arranged. This might be the case if you want to pass both debug and 
    release builds of ICU on Windows, in which case it will link the 
    appopriate version.

#### Linux

You should use your distributions package manager to install icu-dev package.

- CMake:
    By default it will automatically find the system dev package installed by 
    your package manager via the use of pkg-config.  
- autotools:
    This will auto detect the icu installation via the use of pkg-config
    To overide the detection you need to provide compiler and linker flags via
    the ICU_CFLAGS and ICU_LIBS environment variables.


#### Windows

- CMake:
    If no `ICU_URL` or `FETCHCONTENT_SOURCE_DIR_ICU` parameter are passed 
    then it will automatically set `ICU_URL` to be recent release and proceed 
    as above, by downloading and using that archive.  
- Nmake:
    - Create an icu folder under this project's top level folder and unzip
      the archive into it.  
    - makefile.mak copies the needed binaries to the folder where grcompiler
      is built. You may need to modify the file names for the icu/bin/*.dll
      files in makefile.mak since the file names include the version number
      of icu.  
    - The icu project only supplies release versions of the binaries. So, when
      building a debug version of grcompiler, it is linked to release versions
      of the ICU dlls. To link to debug versions instead, icu binaries have to be
      built from source and makefile.mak adjusted to use them. In the icu source,
      there is a VisualStudio file in the source\allinone directory that can be
      used to build the binaries. The "common" project is the one to build.
