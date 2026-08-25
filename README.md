# SpaceEngine

**SpaceEngine** is a high-performance 3D celestial-mechanics and space-simulation engine written in C++20 and OpenGL. It runs a realistic N-body simulation of the Solar System paired with a modern, physically-inspired rendering pipeline and a clean, monochrome glass UI.

## Features

- **N-Body Physics**: Real-time numerical integration of gravitational forces between every body, accelerated by a parallel Barnes-Hut octree, plus inelastic collision merging for asteroids.
- **Advanced Rendering**:
  - Omnidirectional point-light shadow mapping (cubemap depth from the host star).
  - HDR framebuffer with bloom and exposure tone mapping.
  - Procedural screen-space lens flares.
  - Equirectangular HDR skybox.
- **Floating Glass UI**: A modern ImGui HUD with frosted-glass (Gaussian-blurred) panels, rounded corners, and white-on-transparent styling, including target reticles and multi-select.
- **Orbital Mechanics**: Bodies are placed with realistic semi-major axes, inclinations, and longitudes of ascending node. Live orbit lines are derived from each body's angular-momentum vector, so they match the actual trajectory in 3D.
- **Dynamic Camera**: Seamlessly switch between free-fly and a locked orbit/tracking mode around any selected body, with exponential zoom.

## Building

### Prerequisites

- **CMake** 4.0 or higher
- **C++20 compiler** (MSVC, GCC, or Clang)
- **OpenGL 4.4** core-profile capable GPU/driver
- **Git LFS** (required to fetch the large `.obj` planet models)

Dependencies (GLFW, GLM, Dear ImGui) are downloaded automatically at configure time via CMake `FetchContent`.

### Build Instructions (Windows / Linux / macOS)

1. Clone the repository and pull LFS objects:
   ```bash
   git clone https://github.com/Bryan1805a/SpaceEngine.git
   cd SpaceEngine
   git lfs pull
   ```
2. Configure and build:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```

Shaders and assets are copied next to the executable as a post-build step, so run the binary from the build output directory (e.g. `./build/Release/SpaceEngine`).

## Controls

| Input | Action |
|-------|--------|
| `W A S D` | Move camera (free-fly mode) |
| Mouse | Look around |
| Scroll wheel | Zoom (locked orbit mode) |
| `H` | Toggle the HUD |
| `Esc` | Quit |
| `F11` | Toggle fullscreen |
| `Tab` | Toggle the UI cursor (or hold `Alt` while flying) |
| Right-click (body list) | Context menu: lock camera / destroy body |
| `Ctrl + Click` | Multi-select bodies |

See [docs/controls.md](docs/controls.md) for a full walkthrough.

## Documentation

1. [Basic Idea & Core Concepts](docs/basic_idea.md)
2. [Engine Architecture](docs/architecture.md)
3. [Physics & Simulation](docs/physics.md)
4. [Data Flow & Frame Loop](docs/data_flow.md)
5. [Rendering Pipeline](docs/rendering_pipeline.md)
6. [Controls](docs/controls.md)
