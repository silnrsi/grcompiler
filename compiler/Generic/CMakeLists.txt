project(generic)
include(FindIconv)

add_library(generic STATIC Platform.cpp)
add_definitions(-DGR_NAMESPACE)

if (Iconv_FOUND)
    target_link_libraries(generic ${Iconv_LIBRARY})
endif()
