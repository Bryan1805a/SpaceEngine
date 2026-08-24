# Data Flow

The heartbeat of SpaceEngine is defined in `main.cpp`. The data flows sequentially from the Physics Engine to the Renderer every frame.

## The Frame Loop

1. **Input Polling**: GLFW events and keyboard/mouse states are read.
2. **Physics Integration**: 
   - The delta time (`dt`) is multiplied by `timeScale`.
   - `sim.update(dt)` is called.
   - The simulation computes gravitational forces, updates velocities, applies positions, and checks for collisions.
3. **State Extraction**:
   - `main.cpp` extracts the flattened Structural of Arrays (SoA) data (`sim.getPositions()`, `sim.getNames()`, etc.) from the simulation via const references.
4. **Rendering Dispatch**:
   - `renderer.clear()` prepares the framebuffers.
   - `renderer.draw(count, positions, velocities, ...)` is called.
   - The Renderer loops over the data, transforming the pure physical `Vector3` coordinates into scaled OpenGL space, binds the corresponding `.obj` geometries and PBR textures dynamically, and issues `glDrawElements` calls.
5. **UI Construction**:
   - ImGui constructs the SpaceEngine HUD using the extracted data arrays to populate the "Celestial Bodies" list. Interactions here modify the underlying `sim` arrays or `renderer` camera states.
6. **Swap Buffers**: The final composed frame is sent to the display.
