cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(power-cells)

find_package(OpenGL REQUIRED)

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

add_library(sqlite3 STATIC)

target_sources(sqlite3
    PUBLIC
        include/sqlite3.h
        include/sqlite3ext.h
    PRIVATE
        sqlite3.c
)

target_include_directories(sqlite3
    PUBLIC
        include
)

add_library(stb STATIC)

target_sources(stb
    PUBLIC
        include/stb_image.h
        include/stb_truetype.h
    PRIVATE
        stb_image.cpp
        stb_truetype.cpp
)

target_include_directories(stb
    PUBLIC
        include
)

add_executable(power-cells)

target_sources(power-cells
    PUBLIC
        glad.c
        main.cpp
        opengl.h
)

target_include_directories(power-cells
    PRIVATE include
)

target_compile_features(power-cells
    PRIVATE cxx_std_17
)

target_link_libraries(power-cells
    PUBLIC
        ${OPENGL_LIBRARIES}
        glfw
        spdlog
        glm
        stb
        sqlite3
)

