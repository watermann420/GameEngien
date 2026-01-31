cmake_minimum_required(VERSION 3.16)
project(GameEngineLinux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(GameEngineLinux
    src/main.cpp
    src/Renderer2D.h
    src/Renderer2D.cpp
    src/BmpWriter.h
    src/BmpWriter.cpp
)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

