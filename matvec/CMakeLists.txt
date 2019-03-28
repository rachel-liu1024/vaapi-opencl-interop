cmake_minimum_required(VERSION 2.8)

project(matvec)

set (SOURCE_FILES 
    matvec.c
)

include_directories (matvec ${CMAKE_CURRENT_LIST_DIR}/../../compute-runtime/third_party/opencl_headers)

link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(matvec ${SOURCE_FILES})
target_link_libraries(matvec OpenCL)
