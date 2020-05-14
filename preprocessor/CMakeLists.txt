project(preprocessor VERSION 1.5.1)

add_executable(gdlpp cpp1.c cpp2.c cpp3.c cpp4.c cpp5.c cpp6.c memory.c usecpp.c)
target_compile_definitions(gdlpp PRIVATE 
    unix UNIX
    GDLPP 
    PKGDATADIR="${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}")

if  (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_compile_options(gdlpp PRIVATE /W3 /EHsc)
endif()


install(TARGETS gdlpp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
