
cmake_minimum_required(VERSION 2.8)

project(vaocl)

set (SOURCE_FILES 
    vaocl.c
    va_display.c
    va_display.h
)

include_directories (vaocl /usr/local/include)
link_directories(/usr/local/lib/)

add_executable(vaocl ${SOURCE_FILES})
target_link_libraries(vaocl va)
