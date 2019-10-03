project(grc_regression_test)

include_directories(${compiler_SOURCE_DIR})

add_executable(GrcRegressionTest
    CompareFontTables.cpp 
    FileInput.cpp 
    GrcRtFileFont.cpp 
    GrcRegressionTest.cpp 
    $<TARGET_OBJECTS:TtfUtil>)
target_link_libraries(GrcRegressionTest generic)

set(FONT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/fonts)
set(DEFAULT_VERSION 2) 

function(upgrade_benchmark BENCHMARK)
    foreach(dir IN LISTS ICUDIRS)
        set(benchmark "icu${dir}/${BENCHMARK}")
        if (EXISTS ${FONT_SOURCE_DIR}/${benchmark})
            set(BENCHMARK ${benchmark} PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

function(test_font FONT VERSION)
    if (VERSION EQUAL 0)
        set(TEST_SUFFIX "")
        set(VERSION ${DEFAULT_VERSION})
    else()
        set(TEST_SUFFIX "_v${VERSION}")
    endif()
    set(OUT_FONT ${FONT}Test${TEST_SUFFIX}.ttf)
    set(BENCHMARK ${FONT}Benchmark${TEST_SUFFIX}.ttf)
    if (NOT EXISTS ${FONT_SOURCE_DIR}/${BENCHMARK})
        return()
    endif()
    upgrade_benchmark(${BENCHMARK})

    # Compile the font
    add_test(NAME compile_${OUT_FONT}
        COMMAND grcompiler -q -v${VERSION} -e ${PROJECT_BINARY_DIR}/${OUT_FONT}.gdlerr.txt ${FONT_SOURCE_DIR}/${FONT}Main.gdl ${FONT_SOURCE_DIR}/${FONT}Input.ttf ${OUT_FONT})
    set_tests_properties(compile_${OUT_FONT} PROPERTIES
        ENVIRONMENT GDLPP=$<TARGET_FILE:gdlpp>
        ATTACHED_FILES ${PROJECT_BINARY_DIR}/${OUT_FONT}.gdlerr.log
        REQUIRED_FILES "${FONT_SOURCE_DIR}/${FONT}Input.ttf;${FONT_SOURCE_DIR}/${FONT}Main.gdl"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

    # Run the regression tests
    string(REPLACE "/" "-" BENCHMARK_NAME "${BENCHMARK}")
    add_test(NAME GrcRegressionTest_${BENCHMARK_NAME}
        COMMAND GrcRegressionTest -l ${PROJECT_BINARY_DIR}/${BENCHMARK_NAME}.grcregtest.log ${FONT_SOURCE_DIR}/${BENCHMARK} ${OUT_FONT})
    set_tests_properties(GrcRegressionTest_${BENCHMARK_NAME} PROPERTIES
        DEPENDS compile_${OUT_FONT}
        REQUIRED_FILES "${FONT_SOURCE_DIR}/${BENCHMARK};${OUT_FONT}"
        ATTACHED_FILES ${PROJECT_BINARY_DIR}/${BENCHMARK_NAME}.grcregtest.log
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    
    # Compare the output to the expected output
    add_test(NAME compare_${BENCHMARK}
        COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT_FONT} ${FONT_SOURCE_DIR}/${BENCHMARK})
    set_tests_properties(compare_${BENCHMARK} PROPERTIES
        DEPENDS compile_${OUT_FONT}
        REQUIRED_FILES "${FONT_SOURCE_DIR}/${BENCHMARK};${OUT_FONT}"
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


file(GLOB FONTS
    RELATIVE ${FONT_SOURCE_DIR}
    CONFIGURE_DEPENDS
    ${FONT_SOURCE_DIR}/*Input.ttf)
list(TRANSFORM FONTS REPLACE "Input.ttf$" "")
get_icu_dirs()

foreach (FONT IN LISTS FONTS)
    foreach (VERSION IN ITEMS 2 3 0)
        test_font(${FONT} ${VERSION})
    endforeach()
endforeach()
