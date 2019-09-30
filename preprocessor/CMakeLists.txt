project(preprocessor)

add_definitions(-Dunix -DGDLPP -DPKGDATADIR="${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}")
add_executable(gdlpp cpp1.c cpp2.c cpp3.c cpp4.c cpp5.c cpp6.c memory.c usecpp.c)
install(TARGETS gdlpp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
