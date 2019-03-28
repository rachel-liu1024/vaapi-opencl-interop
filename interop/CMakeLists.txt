
cmake_minimum_required(VERSION 2.8)

project(vaocl)

set (SOURCE_FILES 
    vaocl.c
    va_display.c
    va_display.h
    va_display_x11.c
)

include_directories (vaocl /usr/local/include)
include_directories (vaocl ${CMAKE_CURRENT_LIST_DIR}/../../compute-runtime/third_party/opencl_headers)

link_directories(/usr/local/lib/)
link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(vaocl ${SOURCE_FILES})
target_link_libraries(vaocl va va-x11 X11 OpenCL)
