# Rendering Pipeline

SpaceEngine uses a multi-pass OpenGL pipeline to achieve cinematic visuals and accurate lighting. The host star (the light source) is located each frame by body type, so lighting, shadows, and flares stay correct even as bodies are added or removed.

## 1. Shadow Pass (Depth Cubemap)

Because the Sun emits light in all directions, standard 2D shadow mapping is insufficient.

- The scene is rendered from the star's position into a **cubemap depth buffer** (2048² per face). A geometry shader (`shadow.geom`) broadcasts each triangle to all six faces at once.
- This produces omnidirectional shadows, letting planets eclipse each other (e.g. solar eclipses) and cast shadows onto planetary rings.
- The star itself is skipped during this pass so it cannot cast a shadow on itself.

## 2. Forward Geometry Pass

The scene is rendered into a floating-point HDR framebuffer with two color attachments: `FragColor` (the color) and `BrightColor` (pixels above a brightness threshold, for bloom).

- **Skybox**: an equirectangular HDR environment map is drawn first with `GL_LEQUAL` depth so it sits at infinity.
- **Bodies**: each body is drawn with a `T · R · S` model matrix (position, orientation quaternion, radius). The `planet.frag` shader uses **Blinn-Phong** shading with diffuse/specular/emission textures; normals are approximated spherically when missing from the `.obj` file. The shader receives the body type and temperature so stars render as hot HDR emitters and night-side city lights appear on Earth.
- **Orbit lines**: drawn with `GL_LINE_LOOP`. Their 3D orientation is derived from the physical angular-momentum vector (`h = r × v`), so they match each body's trajectory; the line's parent body is resolved from the stable `parentId` (e.g. the Moon orbits Earth, everything else orbits the star).

The projection uses **adaptive near/far planes**, recomputed each frame from the distance to the nearest surface, keeping depth precision tight from solar-system scale down to single-planet scale.

## 3. Procedural Lens Flare

- The star's screen-space position is computed via the view-projection matrices.
- A screen-space fragment shader (`flare.frag`) procedurally draws the central halo, diffraction ring, ghost blobs along the screen-center axis, and an anamorphic streak — no textures involved. Rendered with additive blending.
- A CPU-side occlusion test hides the flare when a planet passes between the camera and the star.

## 4. UI Acrylic Blur

- The HDR scene color is blurred through a dedicated ping-pong framebuffer pair (10 iterations of a 5-tap Gaussian).
- ImGui paints this blurred texture behind its windows (with rounded corners and a soft drop shadow) to produce a frosted-glass "acrylic" look. See `DrawAcrylicBackground` in `main.cpp`.

## 5. Bloom (Gaussian Blur)

- The `BrightColor` attachment is blurred through a second ping-pong framebuffer pair (15 iterations).
- Horizontal/vertical passes using a 5-tap Gaussian kernel spread bright pixels into a soft glow around stars and reflective surfaces.

## 6. Tone Mapping & Composite

- The HDR color and the bloom texture are additively blended, then an **exposure tone-mapping** operator (`screen.frag`, `1 − exp(−color · exposure)`) compresses the HDR range back into display (LDR) color space, mimicking the eye's adaptation to extreme contrast.
