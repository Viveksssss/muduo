cmake_minimum_required(VERSION 3.10.0)
project(muduo VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # -fPIC，动态库必需
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-O2)


# 动态库
aux_source_directory(./src SRC_LIST)
add_library(muduo SHARED ${SRC_LIST})
target_link_libraries(muduo pthread)
target_include_directories(muduo PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

install(DIRECTORY include/
    DESTINATION include/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h"
)

install (TARGETS muduo
    EXPORT muduoTargets 
    LIBRARY DESTINATION lib
)

install(EXPORT muduoTargets
    FILE muduoConfig.cmake
    DESTINATION lib/cmake/muduo
)