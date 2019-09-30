project(compiler)

find_package(ICU COMPONENTS uc i18n data REQUIRED)

add_subdirectory(Generic)
add_subdirectory(Grammar)
add_subdirectory(LZ4)

include_directories(Generic Grammar LZ4)

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
target_link_libraries(grcompiler ${ICU_LIBRARIES} generic parser lz4)

install(TARGETS grcompiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES stddef.gdh DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
