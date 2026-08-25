# Data Flow & Frame Loop

The heartbeat of SpaceEngine lives in `main.cpp`. Data flows sequentially from the physics engine to the renderer every frame.

## The Frame Loop

1. **Input Polling**
   - `renderer.pollEvents()` reads GLFW events and detects window resizes.
   - `renderer.processInput(deltaTime)` handles Esc/F11/Tab and camera movement.

2. **Camera Tracking**
   - `renderer.updateCameraTracking(sim.getPositions())` updates the locked-orbit camera so it follows its target.

3. **Physics Integration**
   - The time step is set from the UI time scale: `sim.setDt(baseDt * timeScale)`.
   - `sim.step()` runs the velocity-Verlet pipeline (integrate → gravity → finalize → collide → thermodynamics).

4. **State Extraction**
   - `main.cpp` grabs const references to the flattened SoA arrays: `getNames()`, `getPositions()`, `getVelocities()`, `getMasses()`, `getRadii()`, `getTemperatures()`, `getTypes()`, plus the stable identity arrays `getIds()`, `getAssetIndices()`, `getParentIds()`.

5. **Rendering Dispatch**
   - `renderer.clear()` prepares the framebuffers.
   - `renderer.draw(count, positions, velocities, radii, orientations, types, temperatures, assetIndices, parentIds, ids)` is called.
   - The renderer locates the host star by body type, resolves each body's model/textures via `assetIndices`, and issues the multi-pass draw (shadow map → geometry → bloom → tone mapping; see [Rendering Pipeline](rendering_pipeline.md)).

6. **UI Construction**
   - `renderer.beginUI()` starts an ImGui frame.
   - ImGui builds the HUD, reading the same SoA references and writing to `sim`/`renderer`. Body removals are **deferred** until after the list loop (using stable ids) so the swap-and-pop removal never disturbs iteration.

7. **Swap Buffers**
   - `renderer.endUI()` renders the ImGui draw data, then `renderer.swapBuffers()` presents the final composed frame.
