cmake_minimum_required(VERSION 3.25)

project(ThreeBodySimulation
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(three_body
    src/main.cpp
)

target_include_directories(three_body PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)