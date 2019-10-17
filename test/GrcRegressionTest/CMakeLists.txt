project(grc_regression_test)

include_directories(${compiler_SOURCE_DIR})

add_executable(GrcRegressionTest
    CompareFontTables.cpp 
    FileInput.cpp 
    GrcRtFileFont.cpp 
    GrcRegressionTest.cpp 
    $<TARGET_OBJECTS:TtfUtil>)
target_link_libraries(GrcRegressionTest generic)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_compile_options(GrcRegressionTest PRIVATE /W3 /EHsc)
endif()


set(FONT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/fonts)

function(compile_test FONT VERSION PSNAME)
    string(REGEX REPLACE "Test(_v${VERSION})?$" "" _input ${FONT})
    string(REGEX REPLACE "(^.+)Test" "\\1Benchmark" _benchmark ${FONT})

    # Compile the font
    add_test(NAME compile_${FONT}
        COMMAND grcompiler -D -v${VERSION} ${ARGN} -e ${PROJECT_BINARY_DIR}/${FONT}.gdlerr.txt ${FONT_SOURCE_DIR}/${_input}Main.gdl ${FONT_SOURCE_DIR}/${_input}Input.ttf ${FONT}.ttf ${PSNAME})
    set_tests_properties(compile_${FONT} PROPERTIES
        ENVIRONMENT GDLPP=$<TARGET_FILE:gdlpp>
#        ATTACHED_FILES ${PROJECT_BINARY_DIR}/${FONT}.gdlerr.log
        REQUIRED_FILES "${FONT_SOURCE_DIR}/${_input}Input.ttf;${FONT_SOURCE_DIR}/${_input}Main.gdl"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})


    # Run the regression tests
    add_test(NAME GrcRegressionTest_${FONT}
        COMMAND GrcRegressionTest -l ${PROJECT_BINARY_DIR}/${FONT}.grcregtest.log ${FONT_SOURCE_DIR}/${_benchmark}.ttf ${FONT}.ttf)
    set_tests_properties(GrcRegressionTest_${FONT} PROPERTIES
        DEPENDS compile_${FONT}
        REQUIRED_FILES "${FONT_SOURCE_DIR}/${_benchmark}.ttf;${FONT}.ttf"
#        ATTACHED_FILES ${PROJECT_BINARY_DIR}/${BENCHMARK_NAME}.grcregtest.log
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    
    # Compare the output to the expected output
    add_test(NAME compare_${FONT}
        COMMAND ${CMAKE_COMMAND} -E compare_files ${FONT}.ttf ${FONT_SOURCE_DIR}/${_benchmark}.ttf)
    set_tests_properties(compare_${FONT} PROPERTIES
        DEPENDS compile_${FONT}
        REQUIRED_FILES "${FONT_SOURCE_DIR}/${_benchmark}.ttf;${FONT}.ttf"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endfunction()

function(get_icu_dirs)
    if(ICU_INCLUDE_DIR AND EXISTS "${ICU_INCLUDE_DIR}/unicode/uvernum.h")
        file(STRINGS "${ICU_INCLUDE_DIR}/unicode/uvernum.h" icu_header_str
        REGEX "^#define[\t ]+U_ICU_VERSION[\t ]+\".*\".*")

        string(REGEX REPLACE "^#define[\t ]+U_ICU_VERSION[\t ]+\"([^ \\n.]*)(\.[^ \\n]*)?\".*"
        "\\1" icu_version_string "${icu_header_str}")
    endif()

    file(GLOB icuders RELATIVE ${FONT_SOURCE_DIR} CONFIGURE_DEPENDS 
        ${FONT_SOURCE_DIR}/icu*)
    list(TRANSFORM icuders REPLACE "icu([0-9]+)$" "\\1")
    list(SORT icuders)
    list(REVERSE icuders)
    foreach(ver IN LISTS icuders)
        if (${ver} LESS_EQUAL ${icu_version_string})
            list(APPEND ICUDIRS "${ver}")
        endif()
    endforeach()
    set(ICUDIRS ${ICUDIRS} PARENT_SCOPE)
endfunction()

compile_test(SchTest 4 "")
compile_test(CharisTest 2 "")
compile_test(PigLatinTest_v2 2 "PigLatin GrRegTest V2" -p)
compile_test(PigLatinTest_v3 3 "")
compile_test(PadaukTest_v3 3 "" -offsets -p)
