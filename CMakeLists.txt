cmake_minimum_required(VERSION 4.0)

project(ThreeBodySimulation
    VERSION 0.1.0
    LANGUAGES CXX
)

# CMake 4 removes compatibility with policies older than 3.5. GLM 0.9.9.8 still
# declares the older minimum, so opt into a modern minimum policy before its
# FetchContent is processed.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# External Denpendencies management
include(FetchContent)

# 1. Download GLFW (Windows and event handle)
message(STATUS "Downloading GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. Download GLM (Math cho OpenGL)
message(STATUS "Downloading GLM...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

find_package(OpenGL REQUIRED)

add_executable(three_body
    src/main.cpp
    src/glad.c
    include/Simulation/Simulation.hpp
    src/Simulation/Simulation.cpp
)

target_include_directories(three_body PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(three_body PRIVATE 
    glfw
    glm::glm
    OpenGL::GL
)