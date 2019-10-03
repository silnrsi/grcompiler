project(compiler)

if (NUGET AND NOT ICU_ROOT)
    file(GLOB ICU_ROOT "${CMAKE_BINARY_DIR}/packages/icu4c.v140")
    set(ICU_ROOT ${ICU_ROOT}/build/native)

    find_library(ICU_I18N_LIBRARY 
        NAMES icuin icui18n 
        PATHS "${ICU_ROOT}/lib/${CMAKE_VS_PLATFORM_NAME}/${CMAKE_BUILD_TYPE}"
        NO_DEFAULT_PATH)
    find_library(ICU_UC_LIBRARY
        NAMES icuuc
        PATHS "${ICU_ROOT}/lib/${CMAKE_VS_PLATFORM_NAME}/${CMAKE_BUILD_TYPE}"
        NO_DEFAULT_PATH)
    find_path(ICU_INCLUDE_DIR
        NAMES "unicode/utypes.h"
        HINTS ${ICU_ROOT}
        PATH_SUFFIXES include
        DOC "ICU include directory")
    set(ICU_REDIST_ROOT "${CMAKE_BINARY_DIR}/packages/icu4c.v140.redist/build/native/bin/${CMAKE_VS_PLATFORM_NAME}")
else()
	find_package(ICU REQUIRED COMPONENTS uc i18n)
endif()


add_subdirectory(Generic)
add_subdirectory(Grammar)
add_subdirectory(LZ4)

include_directories(Generic Grammar LZ4 ${ICU_INCLUDE_DIR})

add_library(TtfUtil OBJECT TtfUtil.cpp)

add_executable(grcompiler 
    Compiler.cpp 
    ErrorCheckClasses.cpp 
    ErrorCheckFeatures.cpp 
    ErrorCheckRules.cpp 
    Fsm.cpp 
    GlyphBoundaries.cpp 
    GdlExpression.cpp 
    GdlFeatures.cpp 
    GdlGlyphClassDefn.cpp 
    GdlRenderer.cpp 
    GdlRule.cpp 
    GdlTablePass.cpp 
    GrcErrorList.cpp 
    GrcFont.cpp 
    GrcGlyphAttrMatrix.cpp 
    GrcManager.cpp 
    GrcMasterTable.cpp 
    GrcSymTable.cpp 
    GrpExtensions.cpp 
    GrpLexer.cpp 
    GrpParser.cpp 
    GrpParserDebug.cpp 
    OutputToFont.cpp 
    ParserTreeWalker.cpp 
    PostParser.cpp 
    $<TARGET_OBJECTS:TtfUtil>
    main.cpp)
target_link_libraries(grcompiler ${ICU_I18N_LIBRARY} ${ICU_UC_LIBRARY} generic parser lz4)

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set_target_properties(grcompiler PROPERTIES
        COMPILE_OPTIONS "/Zc:wchar_t-;/W3;/GR;/EHsc"
        COMPILE_DEFINITIONS "_SCL_SECURE_NO_WARNINGS;_CRT_SECURE_NO_WARNINGS;_WINDOWS;WIN32")
    
    add_custom_command(TARGET grcompiler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ICU_REDIST_ROOT}/$<CONFIGURATION>"
            $<TARGET_FILE_DIR:grcompiler>)
endif()

install(TARGETS grcompiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES stddef.gdh DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
