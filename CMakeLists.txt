cmake_minimum_required(VERSION 4.0)

project(NBodySimulation
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

# 3. Download Dear ImGui
message(STATUS "Downloading Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b
)
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui SOURCE_DIR imgui_dir)

find_package(OpenGL REQUIRED)

add_executable(n_body
    src/main.cpp
    src/glad.c
    include/Simulation/Simulation.hpp
    src/Simulation/Simulation.cpp
    src/Simulation/World.cpp
    src/Simulation/Systems.cpp
    src/Graphics/Renderer.cpp
    src/Graphics/Shader.cpp
    src/Graphics/Camera.cpp
    src/Graphics/Mesh.cpp
    src/Physics/Octree.cpp

    # Compiling with ImGui source code
    ${imgui_dir}/imgui.cpp
    ${imgui_dir}/imgui_draw.cpp
    ${imgui_dir}/imgui_tables.cpp
    ${imgui_dir}/imgui_widgets.cpp
    ${imgui_dir}/backends/imgui_impl_glfw.cpp
    ${imgui_dir}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(n_body PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    ${imgui_dir}
    ${imgui_dir}/backends
)

target_link_libraries(n_body PRIVATE
    glfw
    glm::glm
    OpenGL::GL
)

# Copy shader assets next to the executable so they can be loaded at runtime
add_custom_command(TARGET n_body POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:n_body>/assets"
)