project(compiler)
cmake_policy(SET CMP0074 NEW)

FetchContent_Declare(icu URL ${ICU_URL})

if (DEFINED ICU_URL OR DEFINED FETCHCONTENT_SOURCE_DIR_ICU)
    FetchContent_GetProperties(icu)
    if (NOT icu_POPULATED)
        message(STATUS "Fetching: " ${ICU_URL})
        FetchContent_Populate(icu)
    endif()

    if (icu_POPULATED)
        set(ICU_ROOT ${icu_SOURCE_DIR}/${ICU_ROOT})
    else()
        set(ICU_ROOT "")
    endif()
endif()

find_package(ICU REQUIRED COMPONENTS uc i18n data)
message(STATUS "ICU Libraries: " ${ICU_VERSION})

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

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_compile_options(grcompiler PRIVATE /Zc:wchar_t- /W3 /EHsc)
endif()

target_link_libraries(grcompiler ICU::uc ICU::i18n generic parser lz4)

if (icu_POPULATED)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        file(GLOB_RECURSE _icu_dlls "${icu_SOURCE_DIR}/**${CMAKE_SHARED_LIBRARY_SUFFIX}")
        add_custom_command(TARGET grcompiler POST_BUILD 
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_icu_dlls}
                $<TARGET_FILE_DIR:grcompiler>)
    endif()
endif()

install(TARGETS grcompiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES stddef.gdh DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
