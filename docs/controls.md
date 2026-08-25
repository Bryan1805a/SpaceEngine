# Controls

## Camera

SpaceEngine has two camera modes.

### Free-Fly Mode

| Input | Action |
|-------|--------|
| `W` / `S` | Move forward / backward |
| `A` / `D` | Strafe left / right |
| Mouse | Look around |
| UI "Speed" slider | Movement speed (units/s) |

Movement speed is also auto-scaled to the region you are viewing, so you can cruise across AU-scale distances yet still maneuver precisely around a small planet.

### Locked Orbit Mode

Right-click a body in the "Celestial Bodies" list and choose **Lock & Focus Camera** to orbit it.

| Input | Action |
|-------|--------|
| Mouse | Rotate the orbit around the target |
| `W` / `S` | Zoom in / out |
| Scroll wheel | Zoom in / out |
| Right-click → **Unlock Camera** | Return to free-fly |

Zoom is exponential, so you can travel from the whole system (~100 AU) down to a single planet in a few spins. Zoom clamps so you never dive inside the planet's surface.

## UI / HUD

| Input | Action |
|-------|--------|
| `H` | Toggle the entire HUD |
| `Tab` | Toggle the persistent UI cursor (pauses mouse-look so you can click) |
| `Alt` (hold) | Temporarily show the cursor while flying |
| `Esc` | Quit the application |
| `F11` | Toggle fullscreen |

### Panels

- **SpaceEngine Settings** (left): time-scale slider, pause/reset, camera speed, body count, FPS, and a **Spawn Black Hole** button (launches a massive body along the camera's forward direction).
- **Celestial Bodies** (right): the list of bodies, colored by type. Selecting a body shows its type, mass, and distance and draws a target reticle on it in the 3D view.

### Selection & Context Menus

- **Click** a body to select it (clears the previous selection).
- **Ctrl + Click** toggles an item in a multi-selection.
- **Right-click** a body opens a context menu: **Lock & Focus Camera**, **Unlock Camera**, or **Destroy Body**.
- **Right-click** empty space in the list panel for **Select All** / **Deselect All**.

## Notes

- The "Time Scale" slider ranges from 0× (paused) to 5×, applied to a base step of ~0.365 days.
- Spawning a black hole injects a `STAR`-type body with a large mass and the camera's current velocity, which will visibly disrupt surrounding orbits.
