project(grc_regression_test)

include_directories(${compiler_SOURCE_DIR})

add_executable(GrcRegressionTest
    CompareFontTables.cpp 
    FileInput.cpp 
    GrcRtFileFont.cpp 
    GrcRegressionTest.cpp 
    $<TARGET_OBJECTS:TtfUtil>)
target_link_libraries(GrcRegressionTest generic)

find_program(PERL perl)
if (PERL)
    add_test(NAME check-local COMMAND ${PERL} ${PROJECT_SOURCE_DIR}/regtest -k -v -g $<TARGET_FILE:grcompiler> -p $<TARGET_FILE:gdlpp> -r $<TARGET_FILE:GrcRegressionTest> -d ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/fonts)
else()
    message("Perl interpreter not found cannot run tests.")
endif()