project(grc_regression_test)

include_directories(${compiler_SOURCE_DIR})

add_executable(GrcRegressionTest
    CompareFontTables.cpp 
    FileInput.cpp 
    GrcRtFileFont.cpp 
    GrcRegressionTest.cpp 
    $<TARGET_OBJECTS:TtfUtil>)

add_test(NAME check-local COMMAND ${PROJECT_SOURCE_DIR}/regtest -k -v -g $<TARGET_FILE:grcompiler> -p $<TARGET_FILE:gdlpp> -r $<TARGET_FILE:GrcRegressionTest> -d ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/fonts)
