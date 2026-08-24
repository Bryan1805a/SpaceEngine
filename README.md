# SpaceEngine

**SpaceEngine** is a high-performance 3D celestial mechanics and space simulation engine written in C++ and OpenGL. It provides a realistic N-body simulation of the Solar System (and beyond) paired with a modern, physically-inspired rendering pipeline. 

![SpaceEngine](docs/assets/banner.png) *(Note: Placeholder for banner image)*

## Features
- **N-Body Physics Simulation**: Real-time numerical integration handling gravitational forces between celestial bodies, including collision detection (merging of asteroids).
- **Advanced 3D Rendering**: 
  - Real-time Point Light Shadows (Omnidirectional shadow mapping from the Sun).
  - High-quality Bloom and HDR Tone Mapping.
  - Procedural Screen-Space Lens Flares.
- **Sleek Acrylic UI**: A SpaceEngine-inspired HUD built with ImGui, featuring a frosted-glass (Gaussian blur) background, square target reticles, and multi-select capabilities.
- **Orbital Mechanics**: Planets are generated with realistic orbital inclinations, semi-major axes, and Longitude of Ascending Nodes. Dynamic orbit lines are calculated accurately using angular momentum vectors.
- **Dynamic Camera System**: Seamlessly switch between a Free-Fly mode and a locked Orbital/Tracking mode that orbits around selected bodies.

## Building the Project
### Prerequisites
- **CMake** (3.10 or higher)
- **C++17 Compiler** (MSVC, GCC, or Clang)
- **Git LFS** (Required to pull large `.obj` planet models)

### Build Instructions (Windows / Linux / macOS)
1. Clone the repository and pull LFS objects:
   ```bash
   git clone https://github.com/Bryan1805a/SpaceEngine.git
   cd SpaceEngine
   git lfs pull
   ```
2. Build with CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```

## Controls
- **W, A, S, D**: Move camera (Free-fly mode).
- **Mouse**: Look around.
- **Scroll Wheel**: Adjust movement speed / zoom.
- **H**: Toggle the User Interface (HUD).
- **Right Click (UI)**: Open context menus for planet selection and camera locking.
- **Ctrl + Click (UI)**: Multi-select planets.

## Documentation
Dive deeper into the architecture and design of the engine:
1. [Basic Idea & Core Concepts](docs/basic_idea.md)
2. [Engine Architecture](docs/architecture.md)
3. [Data Flow & Frame Loop](docs/data_flow.md)
4. [Rendering Pipeline](docs/rendering_pipeline.md)
