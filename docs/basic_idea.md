# Basic Idea & Core Concepts

## Purpose

The primary goal of **SpaceEngine** is to simulate and visualize celestial mechanics in an interactive, visually rich 3D environment. It serves both as a physics sandbox and as a showcase for modern OpenGL techniques.

## Core Concepts

### 1. Scale and Precision

Space simulation requires handling enormous distances (Astronomical Units) and extreme masses.

- The simulation uses `double` precision (64-bit) for all physics quantities (positions, velocities, masses, accelerations) via a custom `Vector3`, preventing floating-point drift over long distances.
- World units are **Astronomical Units** (1 AU ≈ 1.496×10⁸ km) and mass is measured in Earth masses.
- For rendering, positions are downcast to 32-bit `glm::vec3` floats. To keep the depth buffer precise whether you are viewing the whole system (~100 AU) or a single planet (~10⁻⁴ AU), the near/far planes are rebuilt every frame from the distance to the nearest scene surface (adaptive near/far planes).

### 2. Time Control

The simulation is decoupled from the frame rate. A `timeScale` multiplies the fixed base time step (`dt = 0.001` years ≈ 0.365 days) before each physics step, letting the user speed up or pause time. The integration scheme (velocity Verlet) is chosen so that orbits do not decay artificially as time is accelerated.

### 3. Emergent Orbits vs. Hardcoded Rails

Unlike traditional games that place planets on fixed 2D rails, every body in SpaceEngine is affected by gravity. Spawning a massive black hole near Earth genuinely disrupts orbits and can fling planets out of the system. Orbital elements (semi-major axis, inclination, longitude of ascending node) are used **only** to compute each body's initial position and velocity; after that, all motion is fully emergent from the N-body simulation. The orbit lines drawn in the viewport are likewise derived from each body's instantaneous angular-momentum vector (`h = r × v`), not from stored orbital elements.

### 4. Identity vs. Array Position

Bodies are stored in flat Structure-of-Arrays (SoA) vectors, and bodies can be added, merged, or destroyed at any time. Because removals use a swap-and-pop, a body's *index* is not stable. Each body therefore also carries a stable numeric `id`, plus render hints (an `assetIndex` pointing at its model/textures and a `parentId` for its orbit line). The renderer and UI key off these stable fields rather than array indices, so lighting, models, and selection stay correct as the body list changes.
