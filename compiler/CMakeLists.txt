project(compiler)
cmake_policy(SET CMP0074 NEW)

if (DEFINED NUGET)
    file(GLOB _icu_nuget_libs_paths "${CMAKE_BINARY_DIR}/packages/icu4c.v*/build/native/lib/*/*")
    file(GLOB _icu_nuget_incs_paths "${CMAKE_BINARY_DIR}/packages/icu4c.v*/build/native/include")
    file(GLOB ICU_REDIST_ROOT "${CMAKE_BINARY_DIR}/packages/icu4c.v*.redist.*/build/native/bin")
    set(ICU_REDIST_ROOT "${ICU_REDIST_ROOT}/${CMAKE_VS_PLATFORM_NAME}")

    set(ICU_ROOT
        ${_icu_nuget_libs_paths}
        ${_icu_nuget_incs_paths}
        ${ICU_ROOT})
    message(STATUS "Using NuGet ICU package.")
endif()

find_package(ICU REQUIRED COMPONENTS uc i18n)
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
target_link_libraries(grcompiler ${ICU_I18N_LIBRARY} ${ICU_UC_LIBRARY} generic parser lz4)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_compile_options(grcompiler PRIVATE /Zc:wchar_t- /W3 /EHsc)

    if (DEFINED ICU_REDIST_ROOT)
        get_filename_component(ICU_REDIST_ROOT ${ICU_REDIST_ROOT} ABSOLUTE)
        add_custom_command(TARGET grcompiler POST_BUILD 
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${ICU_REDIST_ROOT}/$<$<BOOL:${NUGET}>:CONFIGURATION>"
                $<TARGET_FILE_DIR:grcompiler>)
    endif()
endif()

install(TARGETS grcompiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES stddef.gdh DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
