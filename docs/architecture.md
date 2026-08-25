# Engine Architecture

SpaceEngine is structured into strictly separated modules so that the physics engine stays independent of the OpenGL rendering context.

## 1. Simulation Module (`src/Simulation/`)

Responsible for the N-body physics. Bodies live in a flat Structure-of-Arrays (SoA) layout.

- **`World`**: The data container. Holds parallel arrays for every property of every body — `names`, `masses`, `positions`, `velocities`, `accelerations`, `orientations`, `types`, `radii`, `temperatures`, and the stable identity fields `ids`, `assetIndices`, `parentIds`. Provides `addBody()` (returns a stable id), `removeBody()` (swap-and-pop), and `resetAccelerations()`.
- **`System`**: The orchestrator. Owns a `World` plus the sub-systems below and runs the per-frame `step()` pipeline.
- **`Integrator`**: Velocity-Verlet integration — advances positions and a half velocity step, then (after new accelerations are known) completes the velocity. Also spins each body's orientation quaternion from its angular velocity.
- **`GravitySolver`**: Computes gravitational acceleration for every body using a **Barnes-Hut octree** (see the Physics module). Runs the per-body acceleration queries in parallel (`std::execution::par`).
- **`CollisionSystem`**: Detects intersections and merges colliding bodies inelastically (conservation of momentum). Restricted to asteroids — planets/stars are ignored so their exaggerated visual radii never cause false merges.
- **`Thermodynamics`**: Computes surface temperatures from the host star via a Stefan-Boltzmann heat-balance model (stars fixed at ~5778 K, rogue planets at the ~2.7 K cosmic background).

## 2. Physics Module (`src/Physics/`)

- **`Octree`**: A Barnes-Hut spatial tree that approximates the force on each body in O(n log n) instead of O(n²). Leaves/branches accumulate total mass and center of mass; a multipole-acceptance criterion (`size / r < θ`) decides when a whole region may be treated as a single point mass. A maximum depth cap prevents pathological recursion for co-located bodies.

## 3. Graphics Module (`src/Graphics/`)

Responsible for visual representation via OpenGL.

- **`Renderer`**: The core manager of the graphics pipeline. Owns the window, camera, shaders, FBOs, planet models/textures, and shader dispatch.
- **`Mesh`**: Abstraction over VBO/VAO/EBO state; generates spheres, quads, and orbit lines, and loads `.obj` models (with submeshes for multi-material bodies such as Saturn's rings).
- **`Shader`**: GLSL compilation/linking and uniform setters.
- **`Camera`**: An Euler-angle (yaw/pitch) camera with two modes — free-fly (WASD + mouse look) and locked orbit/tracking (spherical orbit around a target with exponential zoom).
- **`PostProcessor`**: Off-screen HDR framebuffer plus bloom ping-pong buffers and the final tone-mapping composite.

## 4. UI Module (ImGui)

Embedded in `main.cpp` (widgets) and `Renderer.cpp` (ImGui initialization and theme). It bridges the user and the engine by reading `sim` state and writing to `renderer`/`sim` variables. The theme is a modern monochrome "floating window" style: rounded corners, white-on-transparent accents, and a frosted-glass background that composites the blurred 3D scene beneath each window.

## Folder Structure

```text
SpaceEngine/
├── assets/            # GLSL shaders, .obj models, textures, HDR skybox
├── build/             # CMake build outputs
├── docs/              # Documentation
├── include/           # Public headers (.hpp)
│   ├── Graphics/
│   ├── Math/          # Vector3 (double-precision math)
│   ├── Physics/       # Octree
│   ├── Simulation/    # World, Systems, Simulation
│   ├── glad/          # OpenGL loader headers
│   └── KHR/           # OpenGL platform headers
├── src/               # Source code (.cpp)
│   ├── Graphics/      # OpenGL rendering engine
│   ├── Physics/       # Octree implementation
│   ├── Simulation/    # Physics implementation
│   ├── glad.c         # OpenGL function loader
│   ├── stb_image.cpp  # Image-loading implementation
│   ├── tiny_obj.cpp   # OBJ-loading implementation
│   └── main.cpp       # Entry point & UI logic
├── third_party/       # Vendored single-header libraries (tiny_obj_loader, stb_image)
└── CMakeLists.txt
```
