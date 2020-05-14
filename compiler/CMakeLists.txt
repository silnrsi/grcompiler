project(compiler)
cmake_policy(SET CMP0074 NEW)

if (DEFINED ICU_URL)
    FetchContent_Declare(icubinaries URL ${ICU_URL})

    FetchContent_GetProperties(icubinaries)
    if (NOT icubinaries_POPULATED)
        FetchContent_Populate(icubinaries)
    endif()

    if (icubinaries_POPULATED)
        set(ICU_ROOT ${icubinaries_SOURCE_DIR}/${ICU_ROOT})
    else()
        set(ICU_ROOT "")
    endif()
endif()

find_package(ICU REQUIRED COMPONENTS uc i18n data)
message(STATUS "ICU Libraries: " ${ICU_VERSION})
get_target_property(_icuuc_location ICU::uc IMPORTED_LOCATION)
get_target_property(_icui18n_location ICU::i18n IMPORTED_LOCATION)
get_target_property(_icudata_location ICU::data IMPORTED_LOCATION)
message("ICU uc lib: " ${_icuuc_location})
message("ICU i18n lib: " ${_icui18n_location})
message("ICU data lib: " ${_icudata_location})

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

if (icubinaries_POPULATED)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        add_custom_command(TARGET grcompiler POST_BUILD 
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:ICU::uc>
                $<TARGET_FILE:ICU::i18n>
                $<TARGET_FILE:ICU::data>
                $<TARGET_FILE_DIR:grcompiler>)
    endif()
endif()

install(TARGETS grcompiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES stddef.gdh DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
