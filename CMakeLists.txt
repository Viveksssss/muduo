cmake_minimum_required(VERSION 3.10.0)
project(muduo VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # -fPIC，动态库必需

add_compile_options(-g -O2)

# 动态库
aux_source_directory(./src SRC_LIST)
add_library(muduo SHARED ${SRC_LIST})
target_link_libraries(muduo pthread)
target_include_directories(muduo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 可执行文件
add_executable(muduo_server ${SRC_LIST} main.cpp)
target_link_libraries(muduo_server PRIVATE muduo)
target_include_directories(muduo_server PRIVATE ${CMAKE_SOURCE_DIR}/include)
