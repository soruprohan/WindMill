# Phases 7 – 8

Navigation and surface detail. After these the project is roughly 70–75% done:
everything is in place except lighting.

**End state:** fly or orbit around a fully textured farm, switch between
perspective and orthographic, pause and speed up the animation, and toggle
textures off to fall back to flat colour.

---

## Phase 7 — Camera, projection and keyboard control

**Goal:** full navigation and interaction, still with no lighting.

### Files created

| File | Contents |
|---|---|
| `src/Camera.h` | `Camera` class interface |
| `src/Camera.cpp` | free-fly movement, mouse look, orbit mode, projection matrices |

### Yaw and pitch are the source of truth

The obvious way to do mouse look is to keep an orientation vector and rotate it
each frame, then check afterwards whether it has gone too far. That check is
awkward and drifts — repeated rotations accumulate float error, and "too far"
has to be tested as an angle between two vectors.

`Camera` stores **yaw and pitch as floats** and rebuilds the orientation from
them:

```cpp
orientation = normalize(vec3(cos(yaw) * cos(pitch),
                             sin(pitch),
                             sin(yaw) * cos(pitch)));
```

Clamping a stored pitch to ±89° is then exact and cannot drift. The clamp
matters: at exactly ±90° the forward vector is parallel to `up`, their cross
product collapses to zero, and the view matrix becomes degenerate — the
classic "camera flips when you look straight up".

### Frame-rate independent movement

Every movement is multiplied by `deltaTime`, so the camera covers the same
distance per second whether the machine runs at 30 or 300 fps:

```cpp
float step = speed * deltaTime;
if (shift) step *= 2.5f;
```

W/S move along `orientation`, A/D along `normalize(cross(orientation, up))`,
E/R along `up`. Left Shift is a hold-to-sprint, which the 60×60 scene needs.

### Mouse look

Holding the right mouse button hides the cursor and turns cursor movement into
yaw and pitch. Two details matter:

- On the **first frame** of the drag the cursor is warped to the window centre
  and the frame is skipped, otherwise wherever the pointer happened to be
  registers as one enormous jump.
- After reading, the cursor is **re-centred every frame**, so it can never
  reach the edge of the screen and stop producing movement.

### Orbit mode (F)

Instead of free-look, the camera is placed on a circle around a fixed look-at
point, exactly as the plan describes:

```cpp
position = vec3(target.x + radius * cos(angle), height, target.z + radius * sin(angle));
orientation = normalize(target - position);
```

The angle advances on its own; W/S zoom, A/D swing manually, E/R change height,
each clamped to a sensible range.

Both modes keep `position` and `orientation` up to date, so `ViewMatrix()` is
built identically either way and Phase 9 can read the eye position for its
specular term without caring which mode is active.

`ToggleOrbit()` also **syncs state in both directions** so the view never
jumps: entering orbit adopts the camera's current radius, height and bearing;
leaving it hands the current facing back to yaw and pitch.

### Projection toggle (P)

```cpp
orthographic ? glm::ortho(-aspect * s, aspect * s, -s, s, 0.1f, 300.0f)
             : glm::perspective(glm::radians(fovDeg), aspect, 0.1f, 300.0f);
```

`s` is 14, which frames the farm well. Both are rebuilt each frame from the
current framebuffer size, so resizing never distorts the scene.

### Edge-triggered toggles

A toggle read with a plain `glfwGetKey` flips on every frame the key is held,
so one tap flickers it dozens of times. `pressedOnce()` in `main.cpp` keeps the
previous state per key and fires only on the transition to pressed. F, P, T and
SPACE all use it; `+`/`-` and `,`/`.` are deliberately continuous so they ramp
smoothly while held.

### `printControls()`

Called once at startup and lists every binding — an explicit requirement of the
lab assignment, so it is written now rather than left to the end.

---

## Phase 8 — Textures

**Goal:** surfaces carry image detail instead of flat colour.

### Files created

| File | Contents |
|---|---|
| `libs/stb_image.h` | image loader (v2.14) |
| `src/Texture.h` | `Texture` class interface |
| `src/Texture.cpp` | loading, mipmaps, filtering, the stb implementation |
| `textures/*.png` | eight seamless tiling textures |
| `textures/generate_textures.py` | the generator that produced them |

`stb_image.h` was already on this machine, in `CSE 4208 - Graphics/Lab_4/codes`.
That copy was used rather than installing anything. `Texture.cpp` is the single
translation unit that defines `STB_IMAGE_IMPLEMENTATION`.

### The textures

They are **generated procedurally** by a small pure-stdlib Python script that
writes PNGs by hand with `zlib` and `struct` — no image libraries and nothing
downloaded. Every pattern is built on value noise sampled over a wrapping
lattice, so all eight tile seamlessly under `GL_REPEAT`.

| Texture | Used by |
|---|---|
| `grass.png` | ground |
| `leaves.png` | tree foliage |
| `bark.png` | tree trunks |
| `wood.png` | windmill head and blades, fence, water wheel, door |
| `brick.png` | farmhouse walls |
| `roof.png` | farmhouse roof |
| `stone.png` | windmill towers |
| `metal.png` | lamp posts, windmill hub |

The sun, the lamp bulbs and the house windows stay flat-coloured on purpose —
the first two become emissive in Phase 10.

Re-run with `python textures/generate_textures.py textures` to regenerate.

### Two loading details that are easy to get wrong

**The vertical flip.** OpenGL's texture origin is the bottom-left corner;
image files store their rows top-down. `stbi_set_flip_vertically_on_load(true)`
is called before loading, otherwise every texture arrives upside-down.

**Row alignment.** `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` is set for 3-channel
images. GL defaults to 4-byte row alignment, and a 3-channel image whose width
is not a multiple of 4 has rows that are not 4-byte aligned — the result is a
texture that shears diagonally. These textures are 256 px wide so it would not
have bitten here, but it would the moment a differently sized image is dropped in.

Filtering is `GL_LINEAR_MIPMAP_LINEAR` when minified — trilinear, so the
distant ground does not shimmer — and `GL_LINEAR` when magnified, with
`GL_REPEAT` on both axes.

### Tiling without an extra uniform

The ground plane already had its UVs multiplied by 20 in the vertex data back in
Phase 6 (`makePlane(8, 20.0f)`), exactly as the plan suggests.

For everything else, the repetition is **baked into the texture** — `brick.png`
contains eight courses of brick, `wood.png` six planks — so a plain 0..1 UV
span across a wall or a fence post already looks right. That avoids adding a
per-object UV-scale uniform and an extra argument to every draw call.

### The toggle (T)

Two booleans rather than one:

```glsl
uniform bool useTexture;   // global, bound to T
uniform bool hasTexture;   // per draw
```

`useTexture` is set once a frame from the T key. `hasTexture` is set by
`Mesh::Draw` from whether a texture pointer was passed, so the sun and the lamp
bulbs never sample a texture that was never bound. A surface is textured only
when both are true; otherwise it falls back to the Phase 6 flat colour, which
is why toggling textures off still produces a sensible-looking scene rather
than a black one.

---

## Also changed

- **`shaders/default.frag`** — samples `diffuse0`, gated by the two booleans.
- **`src/Mesh.h` / `.cpp`** — `Draw` takes an optional `const Texture*`.
- **`src/Scene.h` / `.cpp`** — owns the eight textures, loads them once at
  startup, passes one per draw call, and deletes them on shutdown.
- **`src/main.cpp`** — replaced the placeholder orbit with the `Camera` class,
  added every toggle, and expanded `printControls()`.

---

## Verification

Clean build with `-std=c++17 -Wall`, zero warnings, empty stderr at runtime —
so no missing-uniform warnings for `diffuse0`, `useTexture` or `hasTexture`.

All eight textures reported loaded at 256×256, 3 channels.

Each control was tested by injecting real key events and measuring how much of
the frame changed. Animation was **paused first** in the camera tests, so the
only thing that could move the image was the camera:

| Control | Result |
|---|---|
| `T` textures off | 54.6% of sampled pixels change, geometry identical |
| `T` textures on | returns to the original frame |
| `SPACE` pause | **0.00%** change across 2 s — animation genuinely frozen |
| `P` orthographic | 56.3% change; ground renders as a flat band with no convergence |
| `F` orbit | 66.4% change, and 53.8% more over the next 3 s as it sweeps |
| `W` forward | 65.7% change |

A note on method: the first automated pass produced misleading captures — the
injected keystrokes lagged the screenshots by one step, which made the
orthographic toggle look broken when it was not. Re-testing each control in
isolation, with the animation paused and a validity check on every capture,
gave the clean results above.

---

## File inventory after Phase 8

```
src/
├── main.cpp          window, controls, render loop
├── Camera.h/.cpp     free-fly, mouse look, orbit, projections        [7]
├── Texture.h/.cpp    image loading, mipmaps, filtering               [8]
├── Scene.h/.cpp      layout, windmill hierarchy, draw helpers        [5, 6]
├── Primitives.h/.cpp six unit-primitive generators                   [4]
├── Mesh.h/.cpp       VAO/VBO/EBO ownership, Draw                     [4]
├── Shader.h/.cpp     GLSL loading, compiling, uniforms               [1]
├── Vertex.h          the shared vertex format                        [2]
├── VBO/EBO/VAO       buffer wrappers                                 [2]
└── glad.c            GL function loader                              [0]

libs/     stb_image.h
textures/ grass, leaves, bark, wood, brick, roof, stone, metal (+ generator)
shaders/  default.vert, default.frag
```

## Controls

| Key | Action |
|---|---|
| `W` `S` | Forward / backward (orbit: zoom) |
| `A` `D` | Strafe (orbit: swing) |
| `E` `R` | Rise / descend (orbit: height) |
| `LEFT SHIFT` | Move faster (hold) |
| `RIGHT MOUSE` | Look around (hold) |
| `F` | Orbit camera / free-fly |
| `P` | Perspective / orthographic |
| `T` | Textures on / off |
| `SPACE` | Pause / resume animation |
| `+` `-` | Blade speed |
| `,` `.` | Yaw the windmill heads |
| `ESC` | Quit |

## Next

**Phase 9** — directional Phong lighting. The normals have been filled in
correctly since Phase 3 and have never been used; this is where they start to
matter. The vertex shader gains `Normal = mat3(transpose(inverse(model))) * aNormal`
and `FragPos`, and the fragment shader gets ambient, diffuse and specular.

The `transpose(inverse(model))` normal matrix is **not optional here**: the
blades, tower and fence rails are all scaled non-uniformly, and non-uniform
scaling skews normals.

`Scene::SunPosition()` already supplies the light direction, and
`Camera::Position()` the view vector the specular term needs.
