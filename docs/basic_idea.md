# Basic Idea & Core Concepts

## Purpose
The primary goal of **SpaceEngine** is to simulate and visualize celestial mechanics in an interactive, visually stunning 3D environment. It serves as both a physics sandbox and a rendering showcase for modern OpenGL techniques.

## Core Concepts

### 1. Scale and Precision
Space simulation requires handling massive distances (Astronomical Units) and extreme masses. 
- The simulation uses `double` precision floats (64-bit) for all physics calculations (positions, velocities, masses) to prevent floating-point inaccuracies over long distances.
- For rendering, positions are downscaled and passed to the GPU as 32-bit floats (`glm::vec3`), centered relative to the camera to prevent depth-buffer tearing and jittering at extreme distances (a technique known as Floating Origin, though currently managed via adaptive near/far planes).

### 2. Time Control
The simulation is decoupled from the frame rate. A `timeScale` variable allows the user to accelerate, pause, or reverse time. The physics engine uses a highly stable integration loop to ensure orbits do not decay artificially when time is sped up.

### 3. Procedural Orbits vs N-Body
Unlike traditional games that place planets on hardcoded 2D rails, every single body in SpaceEngine is affected by gravity. If you spawn a massive black hole near Earth, it will dynamically disrupt the orbit and fling planets out of the solar system. The engine calculates the orbital elements (semi-major axis, eccentricity, inclination) only for initial placement and drawing the 3D orbit lines; the actual movement is entirely dynamic and emergent.
