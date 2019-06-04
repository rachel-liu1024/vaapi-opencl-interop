
cmake_minimum_required(VERSION 2.8)

project(cl2va)

set (SOURCE_FILES 
    cl2va.c
    va_display.c
    va_display.h
    va_display_x11.c
)

include_directories (cl2va /usr/local/include)
include_directories (cl2va ${CMAKE_CURRENT_LIST_DIR}/../../compute-runtime/third_party/opencl_headers)

find_library(OPENCL_LIBRARY NAMES OpenCL HINTS /usr/local/lib)

add_executable(cl2va ${SOURCE_FILES})
target_link_libraries(cl2va va va-x11 X11 ${OPENCL_LIBRARY})
