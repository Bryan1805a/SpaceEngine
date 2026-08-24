# Engine Architecture

SpaceEngine is structured into strictly separated modules to keep the physics engine independent from the OpenGL rendering context.

## 1. Simulation Module (`src/Simulation/`)
Responsible for the N-Body physics.
- **`World`**: The core container holding all entities (`positions`, `velocities`, `masses`). It calculates the gravitational forces (F = G * (m1*m2)/r^2) applied to every body.
- **`Systems` (CollisionSystem)**: Handles collision detection. When two bodies intersect, their masses and momentum merge perfectly into a new, larger body (inelastic collision). Planets and stars are explicitly flagged to ignore merging so they don't consume each other due to their exaggerated visual radii.

## 2. Graphics Module (`src/Graphics/`)
Responsible for visual representation using OpenGL.
- **`Renderer`**: The core manager of the graphics pipeline. It manages the window, camera, FBOs (Framebuffers), and shader dispatch.
- **`Mesh` & `Shader`**: Abstractions over OpenGL VBO/VAO states, submeshes for complex models (e.g., Saturn's rings), and GLSL compilation.
- **`Camera`**: A quaternion-based camera capable of both standard Euler-angle free-flight and spherical tracking/orbiting around a locked target.

## 3. UI Module (ImGui integration)
Embedded within `main.cpp` and `Renderer.cpp`. It acts as the bridge between the user and the engine, reading `sim` state and writing to `renderer` variables. The UI uses an acrylic frosted-glass style which composites the blurred 3D scene beneath its windows.

## Folder Structure
```text
SpaceEngine/
├── assets/           # Textures, .obj models, GLSL Shaders
├── build/            # CMake build outputs
├── docs/             # Documentation
├── include/          # Public header files (.hpp)
└── src/              # Source code (.cpp)
    ├── Graphics/     # OpenGL rendering engine
    ├── Simulation/   # Physics and mathematics
    └── main.cpp      # Entry point & UI logic
```
