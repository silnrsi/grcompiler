project(generic)
if  (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_package(Iconv)
endif()

add_library(generic STATIC Platform.cpp)
target_compile_definitions(generic PUBLIC $<${BIGENDIAN_SYSTEM}:WORDS_BIGENDIAN>)
add_definitions(-DGR_NAMESPACE)

if (Iconv_FOUND)
    target_link_libraries(generic ${Iconv_LIBRARY})
endif()
