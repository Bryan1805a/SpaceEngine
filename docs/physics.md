# Physics & Simulation

This document describes the numerical and physical model behind SpaceEngine's N-body simulation.

## Units & Constants

| Quantity | Unit |
|----------|------|
| Distance | Astronomical Units (AU) |
| Time | Years |
| Mass | Earth masses (M_earth) |

The gravitational constant is expressed in these units so that circular-orbit speeds come out naturally:

```
G  = 0.000118549  AU^3 / (M_earth · year^2)
M_sun = 333000.0  M_earth
```

For a circular orbit, `v = sqrt(G · M / a)`. The base time step is `dt = 0.001` years (~0.365 days), scaled by the user-controlled `timeScale` each frame.

## Initial Placement

`main.cpp` builds the Solar System from orbital elements. For each planet, `calculateOrbit(semiMajorAxis, inclination, longitudeOfAscendingNode, centralMass)`:

1. Computes the circular-orbit speed `v = sqrt(G · M / a)`.
2. Places the body at `(a, 0, 0)` with velocity `(0, 0, -v)`.
3. Rotates the position/velocity by the inclination about the X axis, then by the longitude of ascending node about the Y axis.

This yields a realistic 3D orientation for each orbit. From then on, motion is fully emergent — the elements are never consulted again.

## Integration: Velocity Verlet

Each `System::step()` performs one velocity-Verlet step:

1. `Integrator::update` — advance position and half of the velocity using the *previous* accelerations, and spin orientations by angular velocity.
2. `GravitySolver::compute` — recompute accelerations from the new positions.
3. `Integrator::finalizeVelocity` — complete the velocity using the *new* accelerations.
4. `CollisionSystem::resolve` — merge intersecting bodies.
5. `Thermodynamics::update` — recompute surface temperatures.

Splitting the velocity update around the force evaluation makes the integrator second-order and symplectic enough that orbits do not artificially spiral over time.

## Gravity: Barnes-Hut Octree

A direct O(n²) force sum would be too slow for large body counts. Instead, `GravitySolver` builds a **Barnes-Hut octree** every step:

- The tree's root adapts to the bounding box of all bodies.
- Each node stores the total mass and center of mass of the bodies it contains.
- When querying the acceleration on a body, a node is treated as a single point mass if it is a leaf or if `size / distance < θ` (the multipole-acceptance criterion, `θ = 0.5`). Otherwise the query descends into the node's eight children.
- Acceleration queries run in parallel across CPU cores (`std::execution::par`).
- A maximum subdivision depth (64) guards against co-located bodies that would otherwise recurse forever.

## Collisions

Collision detection is O(n²) and only considers **asteroids** (planets and stars are ignored because their visual radii are exaggerated for visibility and would otherwise merge spuriously). When two asteroids intersect:

- Masses are summed (`m = m_i + m_j`).
- Velocity is the momentum-conserving weighted average.
- Position is the center of mass.
- Radius combines by volume (`r³ = r_i³ + r_j³`); albedo, greenhouse factor, and temperature are mass-weighted averages.

The merged body keeps the identity (`id`, `assetIndex`, `parentId`) of the first body.

## Thermodynamics

`Thermodynamics::update` finds the first star in the system and uses it as the heat source:

- **Stars** are set to 5778 K (the Sun's surface temperature).
- **Planets** use a Stefan-Boltzmann heat-balance approximation:
  `T = 278.3 · (1 − albedo)^0.25 / sqrt(distance) · greenhouse`.
- **Rogue planets** (no host star) are set to 2.7 K, the cosmic microwave background temperature.

The implementation currently uses a single heat source; a future update could sum contributions from multiple stars.
