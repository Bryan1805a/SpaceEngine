# Rendering Pipeline

SpaceEngine utilizes a complex, multi-pass OpenGL rendering pipeline to achieve cinematic visuals and accurate lighting.

## 1. Shadow Pass (Depth Cubemap)
Because the Sun emits light in all directions, standard 2D shadow mapping is insufficient. 
- The engine renders the scene from the perspective of the Sun into a **CubeMap Depth Buffer** using a geometry shader (`shadow.geom`) to broadcast the triangles to all 6 faces simultaneously.
- This creates omnidirectional shadows, allowing planets to properly eclipse each other (e.g., Solar Eclipses) and cast accurate shadows onto planetary rings.

## 2. Forward Geometry Pass
- The scene is rendered normally into a Floating-Point Framebuffer (HDR FBO) with 2 color attachments: `FragColor` (normal colors) and `BrightColor` (pixels exceeding a brightness threshold).
- Planets use a **Blinn-Phong** shading model (`planet.frag`) mapped with diffuse, specular, and specular-mask textures. Normal vectors are dynamically calculated via spherical approximation if missing from the source `.obj` files.
- Dynamic orbit lines are drawn using `GL_LINE_LOOP`. Their 3D orientation is derived directly from the physical angular momentum vector (`h = r x v`) ensuring they perfectly match the planet's trajectory across all dimensions.

## 3. Bloom (Gaussian Blur)
- The `BrightColor` texture is passed through a **Ping-Pong Framebuffer** system.
- A 2-pass (Horizontal and Vertical) Gaussian Blur shader diffuses the bright pixels over 10 iterations to create a soft, glowing bloom effect around stars and highly reflective surfaces.

## 4. Procedural Lens Flare
- The screen-space coordinates of the Sun are calculated via view-projection matrix multiplications.
- Ghosting artifacts and a central halo are drawn procedurally in 2D by scaling and translating textures along the vector from the screen center to the Sun's position. Rendered with additive blending.

## 5. UI Acrylic Blur
- Before drawing the UI, the current scene color buffer is blurred again into a dedicated texture (`uiPingpongColorbuffers`).
- ImGui paints this blurred texture into the background of its windows, achieving a modern "frosted glass" acrylic look without sacrificing text legibility.
- Custom ImGui primitive drawing is used for on-screen HUD elements (reticles, selection circles).

## 6. Tone Mapping & Post-Processing
- The HDR buffer and Bloom buffer are additively blended together.
- An **Exposure Tone Mapping** algorithm (`screen.frag`) compresses the wide range of HDR light back into standard display color space (LDR). This mimics the human eye's adaptation to extreme contrast in space.
