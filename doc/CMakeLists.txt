project(docs)

find_program(DOCBOOK2MAN NAMES docbook2x-man)

configure_file(gdlpp.1.in gdlpp.1 COPYONLY)
set(DOC_DEPENDS ${PROJECT_BINARY_DIR}/gdlpp.1)
file(GLOB DOCS *.pdf)
set(DOCS ${DOCS} README.gdlpp)

if (DOCBOOK2MAN)
    add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/grcompiler.1
        DEPENDS ${PROJECT_SOURCE_DIR}/grcompiler.1.xml
        COMMAND ${DOCBOOK2MAN} ${PROJECT_SOURCE_DIR}/grcompiler.1.xml)
    set(DOC_DEPENDS ${DOC_DEPENDS} ${PROJECT_BINARY_DIR}/grcompiler.1)
endif()

add_custom_target(docs ALL DEPENDS ${DOC_DEPENDS})
install(FILES ${DOC_DEPENDS} DESTINATION ${CMAKE_INSTALL_MANDIR})
install(FILES ${DOCS} DESTINATION ${CMAKE_INSTALL_DOCDIR}/${CMAKE_PROJECT_NAME})
