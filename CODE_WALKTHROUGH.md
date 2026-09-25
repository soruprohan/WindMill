# Code Walkthrough — every file, block by block

Read in this order. Each file builds on the ones before it, so by the time you
reach `Scene.cpp` every tool it uses is already familiar.

```
 1. Vertex.h              what one vertex is
 2. Shader.h / .cpp       loading GLSL, setting uniforms
 3. default.vert / .frag  the GLSL that Shader loads
 4. VBO / EBO / VAO       three GPU buffer wrappers
 5. Primitives.h / .cpp   the shape generators
 6. Mesh.h / .cpp         one drawable shape = VAO + VBO + EBO
 7. Texture.h / .cpp      an image on the GPU
 8. Camera.h / .cpp       where you are and what you see
 9. Scene.h / .cpp        the actual farm
10. main.cpp              window, input, the loop
```

Blocks are described; lines are quoted only where the line *is* the point.

---

## 1. `src/Vertex.h`

One struct, no functions.

```cpp
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;     // filled from Phase 3, used from Phase 9 (lighting)
    glm::vec2 texCoord;   // filled from Phase 3, used from Phase 8 (textures)
    glm::vec3 color;      // placeholder colour, used Phases 3-7
};
```

This is the **one vertex format for the entire project**. Every generator in
`Primitives.cpp` emits these; every `VBO` uploads a `std::vector<Vertex>`;
`Mesh` tells the GPU where each field lives with `offsetof(Vertex, ...)`; and
the four `layout (location = N)` inputs in `default.vert` map to the four
fields in this exact order.

If you ever change this struct, those four places all have to agree.

The comment matters: `normal` has been filled in correctly since Phase 3
even though nothing reads it until Phase 9. That was the plan's deliberate
choice so lighting can be switched on later without touching any geometry.

---

## 2. `src/Shader.h` and `src/Shader.cpp`

A `Shader` object is one linked GL program built from a vertex file and a
fragment file. Its public surface is small: `Activate()`, `Delete()`, and a
family of `setXxx(name, value)` uniform setters.

### `Shader.h` — the class

| Block | What it is |
|---|---|
| `GLuint ID = 0;` | The GL program handle. `0` means "not usable" — `main` checks this. |
| Constructor | Takes two file paths. |
| `Activate()` / `Delete()` | `glUseProgram` / `glDeleteProgram`. |
| `setBool … setMat4` | Eight uniform setters, one per type used in the project. |
| `uniformCache` (private, `mutable`) | Maps uniform name → location. `mutable` so the `const` setters can fill it. |
| `location()` | The cached lookup. |
| `readFile / checkCompile / checkLink` | `static` helpers — they do not need a `Shader` instance. |

### `Shader.cpp` — the implementation

**Constructor.** Five steps, each of which can bail out leaving `ID == 0`:

1. `readFile` both paths into strings. Fail → return.
2. `glCreateShader(GL_VERTEX_SHADER)`, `glShaderSource`, `glCompileShader`, then `checkCompile`.
3. Same for the fragment shader.
4. Only if both compiled: `glCreateProgram`, attach both, `glLinkProgram`, `checkLink`. If link fails, delete the program and reset `ID` to 0.
5. `glDeleteShader` both stages. The linked program keeps its own copy; the stage objects are only needed during linking.

Prints `Shader OK: …` on success so you can see it in the console.

**`readFile`.** `std::ifstream` + `std::stringstream` slurp. On failure it prints
the path *and* the reminder to run from the project root, because a wrong
working directory is the usual cause.

**`checkCompile` and `checkLink`.** Same shape: query the status, query the
info-log length, and if the log is non-empty **print it whether or not the
stage succeeded**. That is intentional — a shader that compiles with a warning
and a shader that fails both give you a black screen, and only the log tells
them apart.

**`location`.** Look in `uniformCache`; miss → `glGetUniformLocation`, store it,
return it. Prints a warning once if the name is `-1` (not found, or optimised
out by the GLSL compiler because it was unused). Every setter goes through
this, so the string lookup happens once per uniform name for the life of the
program instead of every frame.

**The setters.** One-liners: `glUniform1i`, `glUniform1f`, `glUniform2fv`…
`glm::value_ptr` turns a GLM vector/matrix into the `float*` GL wants.
`GL_FALSE` in the matrix calls means "do not transpose" — GLM and GL both use
column-major order, so no transpose is needed.

---

## 3. `shaders/default.vert` and `shaders/default.frag`

These are what `Shader` loads. They run on the GPU: the vertex shader once per
vertex, the fragment shader once per pixel.

### `default.vert`

```glsl
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;
```
The four fields of `Vertex`, in the same order, at the locations `Mesh` links
them to.

```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
```
The three matrices set from C++: `model` per object by `Mesh::Draw`, the other
two per frame by `main`.

```glsl
gl_Position = projection * view * model * vec4(aPos, 1.0);
```
**The line the whole 3D pipeline hangs on.** Matrices compose right-to-left:
`model` moves the vertex into the world, `view` moves the world in front of
the camera, `projection` squashes it into the screen.

The three `out` variables just pass colour, UV and normal through to the
fragment shader unchanged. Phase 9 will start transforming the normal here.

### `default.frag`

```glsl
uniform vec3 objectColor;
uniform sampler2D diffuse0;
uniform bool useTexture;   // global T toggle
uniform bool hasTexture;   // per draw
uniform vec2 uvOffset;     // water scrolling
```

The body is three lines:

```glsl
vec3 base = objectColor;
if (useTexture && hasTexture)
    base = texture(diffuse0, vTexCoord + uvOffset).rgb;
FragColor = vec4(base * vColor, 1.0);
```

Start with the flat colour. If textures are on globally **and** this
particular draw has one, sample the texture instead. `uvOffset` is zero for
everything except the river and waterfall, where it slides the texture every
frame — that sliding is the entire flowing-water effect.

`vColor` is white on every primitive, so `base * vColor` is currently a
no-op; it keeps the attribute alive for later.

---

## 4. `src/VBO`, `src/EBO`, `src/VAO`

Three tiny wrappers. Each owns one GL handle and exposes `Bind`, `Unbind`,
`Delete`. Learn these and `Mesh` becomes obvious.

### `VBO` — Vertex Buffer Object

The GPU-side copy of a `std::vector<Vertex>`.

Constructor: `glGenBuffers`, `glBindBuffer(GL_ARRAY_BUFFER, …)`, then
`glBufferData` with `vertices.size() * sizeof(Vertex)` bytes and
`GL_STATIC_DRAW` (upload once, draw many times — exactly this project's
pattern).

### `EBO` — Element Buffer Object

Same shape for a `std::vector<GLuint>` of indices, bound to
`GL_ELEMENT_ARRAY_BUFFER`. It also stores `count`, the number of indices, so
`glDrawElements` can read `ebo.count` instead of the caller tracking it.

The comment on `Unbind` is a real trap: **a VAO records the EBO binding**, so
unbinding the EBO while a VAO is still bound erases the EBO from that VAO and
the next draw reads from nothing. `Mesh` unbinds the VAO first for exactly this
reason.

### `VAO` — Vertex Array Object

The VAO records *how to interpret* a VBO: which bytes are position, which are
normal, and so on.

`LinkAttrib(vbo, layout, numComponents, type, stride, offset)` collapses the
four-call dance into one line:

```cpp
vbo.Bind();
glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
glEnableVertexAttribArray(layout);
vbo.Unbind();
```

`layout` is the `location = N` in the vertex shader; `stride` is
`sizeof(Vertex)`; `offset` is where that field starts inside the struct.

---

## 5. `src/Primitives.h` and `src/Primitives.cpp`

Six functions that each build a shape as CPU-side data. Nothing here talks to
the GPU.

### `Primitives.h`

```cpp
struct MeshData { std::vector<Vertex> vertices; std::vector<GLuint> indices; };
```
The return type of every generator — the two vectors a `Mesh` needs.

The header comment sets three rules every generator obeys:

1. **Unit size, centred on the origin.** Everything fits in a 1×1×1 box. The
   scene scales each one with a model matrix, so `glm::scale(vec3(w, h, d))`
   gives exactly those dimensions and one cylinder mesh serves the tower,
   trunks, poles and axle alike.
2. **Vertex colour is white.** Colour comes from the `objectColor` uniform,
   so one mesh draws in any colour.
3. **Counter-clockwise winding seen from outside.** This is what lets
   back-face culling stay on.

### `Primitives.cpp`

**Anonymous-namespace helpers.** `WHITE`, and `addQuad(indices, base)` which
pushes the six indices for two triangles covering vertices `base..base+3`.

**`makeCube`.** A table of six faces, each with a normal and four corners
listed counter-clockwise from outside. The loop emits four vertices per face
and calls `addQuad`. 24 vertices, not 8, because each face needs its own
normal — a shared corner vertex could only carry one.

**`makePlane`.** The two-argument version just calls the three-argument one
with the same scale twice. The real one builds an `(n+1)×(n+1)` grid of
vertices in the XZ plane at y = 0, normal +Y, UVs multiplied by
`uvScaleU` / `uvScaleV` so a texture tiles instead of stretching (the ground
uses 50; the river uses 20 along and 1.5 across). Index loop stitches each grid
cell into two CCW triangles.

**`makeCylinder`.** Two parts that share no vertices:

- *Side wall.* `segments + 1` pairs of (bottom, top) vertices — the extra
  one duplicates angle 0 at angle 2π so the texture seam closes. Normal is
  `(cos θ, 0, sin θ)`. Quads stitched between neighbouring pairs.
- *Caps.* For each of top and bottom: one centre vertex plus a ring, normal
  `(0, ±1, 0)`, stitched as a fan. The bottom fan and top fan use opposite
  index order so both wind CCW from outside.

They cannot share vertices because a wall vertex's normal points outward and
a cap vertex's normal points up or down.

**`makeCone`.** Built the same way as the cylinder's side: a bottom ring and a
"top ring" whose vertices all sit at the apex `(0, 0.5, 0)` but each carry
their own angle's normal and U coordinate. One triangle per segment (the
second half of each quad would have zero area). Then a base disc fan.

The important line is the side normal:

```cpp
glm::normalize(glm::vec3(H * std::cos(theta), r, H * std::sin(theta)))
```

Not `(x, 0, z)` — the surface slopes, so the normal tilts up. This is the
exact result of crossing the surface tangents; the plan's `(x, r/h, z)` was an
approximation.

**`makeSphere`.** Standard UV sphere: for each of `stacks + 1` latitude rings,
`sectors + 1` vertices around. Position from `(φ, θ)`, normal is simply the
normalised position because the sphere is centred at the origin. The index
loop skips one triangle of each quad at the two poles, where a ring collapses
to a point and that triangle would be degenerate.

**`makePrism`.** The gable roof. Six named corners (left/right, base/apex,
front/back), two slope normals worked out as perpendicular to the slope
edge, then a small `quad` lambda emits the two slopes and the underside, and
two hand-written triangles close the gable ends. Ridge runs along Z.

---

## 6. `src/Mesh.h` and `src/Mesh.cpp`

A `Mesh` is one shape on the GPU: it owns a VAO, a VBO and an EBO, and can
draw itself with any model matrix, colour and texture.

### `Mesh.h`

| Block | What it is |
|---|---|
| `explicit Mesh(const MeshData&)` | Uploads the generator's output. |
| `Draw(shader, model, color, texture)` | Colour defaults to white, texture to `nullptr`. |
| `Delete()` | Releases all three buffers. |
| Deleted copy ctor / assignment | It holds GL handles; a copy would delete them twice. |
| `VAO vao; VBO vbo; EBO ebo;` | **Declaration order is construction order** — see below. |

### `Mesh.cpp`

**`bindThenPass` helper.** This is the one non-obvious block in the file,
and the comment explains it fully. In a core-profile context there is no
default VAO, so binding an `GL_ELEMENT_ARRAY_BUFFER` is only legal — and only
*recorded* — while a VAO is bound. But the `EBO` constructor does that
binding, and C++ runs member initialisers **before** the constructor body. So
the VAO has to be bound from inside the initialiser list. The helper binds the
VAO and returns the vertices unchanged, so it can sit in the `vbo(...)`
initialiser:

```cpp
Mesh::Mesh(const MeshData& data)
    : vao(), vbo(bindThenPass(vao, data.vertices)), ebo(data.indices)
```

`vao` is constructed first (declared first), `vbo`'s initialiser binds it,
then `ebo` is constructed with the VAO bound and its binding gets recorded.

**Constructor body.** Four `LinkAttrib` calls, one per `Vertex` field, using
`offsetof`. Then unbind — VAO first, EBO last, for the reason in §4.

**`Draw`.** Sets `model` and `objectColor`; decides `textured` (a texture was
passed *and* it loaded successfully); sets `hasTexture` accordingly and binds
the texture if so; then `vao.Bind()`, `glDrawElements(GL_TRIANGLES,
ebo.count, GL_UNSIGNED_INT, 0)`, `vao.Unbind()`.

---

## 7. `src/Texture.h` and `src/Texture.cpp`

Loads a PNG from disk into a GL texture object with mipmaps and repeat wrap.

### `Texture.h`

Constructor takes a path and a texture unit (default 0). `Bind`, `Unbind`,
`Delete`, and `Valid()` which is just `ID != 0`. Copying is deleted, same
reason as `Mesh`.

### `Texture.cpp`

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```
stb_image is a single header. Exactly one `.cpp` in the program must define
this macro before including it, and this is that file.

**Constructor**, in order:

1. `stbi_set_flip_vertically_on_load(true)` — GL's texture origin is
   bottom-left, image files are top-down. Without this every texture is
   upside-down.
2. `stbi_load` → width, height, channel count, and a pixel buffer. Failure
   prints the path and stb's reason, and leaves `ID == 0`.
3. Pick `GL_RED` / `GL_RGB` / `GL_RGBA` from the channel count.
4. `glGenTextures`, `glActiveTexture(GL_TEXTURE0 + unit)`, `glBindTexture`.
5. Filtering: `GL_LINEAR_MIPMAP_LINEAR` when the texture is far away
   (trilinear, so distant grass does not shimmer), `GL_LINEAR` when close.
   Wrap `GL_REPEAT` on both axes so UVs above 1 tile.
6. `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` for 3-channel images. GL assumes
   4-byte row alignment by default, and an RGB image whose width is not a
   multiple of 4 would shear.
7. `glTexImage2D` uploads; `glGenerateMipmap` builds the smaller levels.
8. `stbi_image_free` — the GPU has its copy. Unbind. Print `Texture OK`.

**`Bind` / `Unbind`.** Select the unit, then bind or unbind on it.

---

## 8. `src/Camera.h` and `src/Camera.cpp`

Where the viewer is and which way they face, in two modes: free-fly and
orbit. Also owns the perspective/orthographic choice.

### `Camera.h`

Public: constructor `(startPosition, yawDeg, pitchDeg)`, `Inputs()`,
`ViewMatrix()`, `ProjectionMatrix(aspect)`, the two toggles, and three read
accessors. Three tunables: `speed`, `sensitivity`, `fovDeg`.

Private state worth knowing:

- `position`, `orientation`, `up` — what `ViewMatrix` is built from, in both
  modes.
- `yaw`, `pitch` — **the source of truth in free-fly mode.** The header comment
  says why: clamping a stored angle is exact; rotating a vector and testing
  afterwards drifts.
- `firstClick` — for mouse look.
- The orbit parameters: `orbitTarget`, `orbitRadius`, `orbitHeight`,
  `orbitAngle`, `orbitSpeed`.
- `orthoScale` — half-height of the orthographic box.

### `Camera.cpp`

**`PITCH_LIMIT = 89`.** At ±90° the forward vector is parallel to `up`, their
cross product is zero, and the view matrix is garbage. One degree short avoids
it.

**`updateOrientation`.** Clamp pitch, then rebuild the direction vector from
the two angles:

```cpp
orientation = normalize(vec3(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch)));
```

**`Inputs`.** Dispatches to `orbitInputs` or `freeFlyInputs`.

**`freeFlyInputs`.** `step = speed * deltaTime` (×2.5 with Shift). W/S along
`orientation`, A/D along `right = cross(orientation, up)`, E/R along `up`.
Then `mouseLook`. Multiplying by `deltaTime` is what makes movement
frame-rate independent.

**`mouseLook`.** Only while the right button is held. Three phases:

- Button *not* held: if we were dragging, restore the cursor and reset
  `firstClick`. Return.
- First frame of a drag: hide the cursor, warp it to the window centre, set
  `firstClick = false`, return *without reading* — otherwise wherever the
  cursor happened to be would register as one huge jump.
- Every later frame: read the cursor, turn its offset from centre into yaw and
  pitch deltas (pitch subtracts because screen Y points down), rebuild the
  orientation, warp back to centre so the cursor never reaches the screen edge.

**`orbitInputs`.** Advance `orbitAngle` automatically. A/D swing it manually,
W/S change the radius, E/R the height, each clamped. Then compute the position
on the circle and aim at the target:

```cpp
position    = vec3(target.x + radius * cos(a), height, target.z + radius * sin(a));
orientation = normalize(target - position);
```

**`ToggleOrbit`.** Flips the mode and **syncs state both ways** so the view
never jumps: entering orbit, it reads the current radius, height and bearing
from where the camera already is; leaving orbit, it converts the current
orientation back into yaw and pitch with `atan2` and `asin`.

**`ViewMatrix`.** `glm::lookAt(position, position + orientation, up)` —
identical in both modes.

**`ProjectionMatrix`.** `glm::ortho` with `orthoScale` half-height and the
aspect ratio applied to the width, or `glm::perspective` with `fovDeg`. Both
use near 0.1, far 300.

---

## 9. `src/Scene.h` and `src/Scene.cpp`

The actual farm. Everything on screen is placed and drawn here.

### `Scene.h`

**`Transform`** — position, rotation (degrees, applied Y then X then Z),
scale, and a `matrix()` that composes them. Used for trees, the house and the
mountains. Placement is data in a vector, not matrices in code.

**`Windmill`** — position, uniform scale, head yaw, and a blade phase offset
so the two windmills are not in lockstep.

**`Scene` public API:**

| Member | Purpose |
|---|---|
| `Update(dt)` | Advance the animation angles. |
| `Draw(shader)` | Draw everything. |
| `Delete()` | Release every mesh and texture. |
| `AdjustHeadYaw(deg)` | Yaw both windmill heads — the Phase 5 hierarchy demo. |
| `LampBulbPositions()` / `SunPosition()` | Saved for Phase 9/10 lights. |
| `bladeSpeed`, `paused`, `riverFlow` | Animation state `main` binds keys to. |

**`Scene` private state:** the draw helpers; six primary `Mesh` objects plus
three (`riverPlane`, `fallPlane`, `mountain`) that exist only for their
different texture tiling; eleven `Texture` objects; three animation
accumulators; and the placement vectors.

### `Scene.cpp`

**Anonymous namespace: palette and dimensions.** Every flat colour
(`C_GRASS`, `C_WALL`…) and every scene number (`GROUND_SIZE`, `RIVER_Z`,
`WHEEL_RADIUS`…) is a named constant here. The comments explain the
relationships — e.g. the river starts at the foot of the west mountain, the
wheel axle sits `WHEEL_RADIUS - 0.35` high so paddles dip in.

**`Transform::matrix`.** `translate → rotate Y → rotate X → rotate Z → scale`,
composed left to right in code which means applied right to left to the
vertex: scale first, translate last.

**Constructor.** The initialiser list is where every primitive is generated
*exactly once* and every texture loaded once. Note the three extra meshes:
`riverPlane` tiles 20 along × 1.5 across, `fallPlane` 3.5 × 1, `mountain` is a
36-segment cone tiling 30 round × 10 up. The body is pure layout data:
windmill entries, house transform, wheel position, an 18-row tree table, four
lamp posts (with bulb world positions computed and stored once), a 15-row
mountain table whose first entry is the river's source, and the sun position.

**`Update`.** Skips if paused. Otherwise advances `bladeAngle` by
`bladeSpeed`, advances `waterScroll` by `riverFlow`, and derives the wheel's
angular speed from the river:

```cpp
const float wheelSpeed = glm::degrees(riverFlow / WHEEL_RADIUS);
```

A paddle at the rim moves at the water's speed, so angular velocity is
linear velocity over radius. One number drives both the water and the wheel.

**`Draw`.** Resets `uvOffset` to zero first, then calls every draw helper in
a fixed order: ground, mountains, river, waterfall, windmills, house, wheel,
trees, fence, lamps, sun.

**`drawGround`.** Scale the unit plane to `GROUND_SIZE`, draw with grass.

**`drawWindmill` — the centrepiece.** Read the comment diagram in the file.
The chain is:

```
base      = translate(position) · scale(scale)
tower     = base · translate(0, H/2, 0) · scale(dia, H, dia)
headPivot = base · translate(0, H, 0) · rotateY(yawDeg)
hubPivot  = headPivot · translate(0, 0, headDepth/2) · rotateZ(bladeAngle + phase)
blade[i]  = hubPivot · rotateZ(90·i) · translate(0, L/2, 0) · scale(w, L, d)
```

**`hubPivot` is built from `headPivot`.** That single fact is why yawing the
head swings all four blades with it while the blades keep spinning on their
own axis — parent-child inheritance, the project's whole point.

The other trick to notice, used everywhere in this file: **translate by half
the height before scaling**. A unit cylinder is centred on its origin, so
scaling it to height 5 stretches it 2.5 up and 2.5 down. Translating by 2.5
first moves it so it extends upward only. Tower, trunks, poles, spokes and
mountains all do this.

**`drawHouse`.** Body cube, prism roof sitting on top of it, and a door and
two windows placed on the front face at `bodySize.z/2 + 0.01`. The 0.01 is
the z-fighting fix — they are coplanar with the wall, so they are pushed a
hair in front of it.

**`drawWaterWheel`.** A second, simpler hierarchy. Two support posts are
drawn *off* the pivot (they hold the axle still). Then one `pivot` rotated by
`+wheelAngle` about Z (positive = anticlockwise from the camera = bottom
paddles move +X, the same way the river flows). Everything else hangs off it:
an axle cylinder tipped 90° to lie along Z, and per paddle a spoke (offset by
half its length, same trick), a rim segment scaled to the chord between
neighbours so the ring closes, and the paddle itself standing proud of the rim.

**`drawTree`.** Trunk cylinder plus two overlapping cones, all relative to
`t.matrix()`.

**`drawFence` / `drawFenceRun`.** Four runs between the fence corners; the
front one is split in two to leave a gateway. `drawFenceRun` places
`spans + 1` posts along the line, then two rails per span, each rail rotated
about Y by `atan2(dir.x, dir.z)` so its local Z lines up with the run.

**`drawLampPost`.** Pole cylinder plus a bulb sphere drawn with *no texture*
(the fourth argument is omitted) so it stays flat yellow.

**`drawSun`.** One sphere, no texture.

**`drawRiver`.** Compute `unitsPerTile = length / RIVER_TILES`, set
`uvOffset.x = -waterScroll / unitsPerTile` (negative: shifting the lookup
backward makes the pattern move forward), draw the river plane, reset the
offset to zero, then draw two long low dirt cubes as banks.

**`drawWaterfall`.** Reads the source mountain's radius and height from
`mountains[0]`, computes the slope angle with `atan2(height, radius)`, walks
from the foot of the east slope part-way up it plus a small step along the
normal so it does not z-fight the cone, then draws the plane rotated about Z
by `-slopeDeg` — which tilts a +Y-facing plane to face +X-and-up, exactly the
slope's direction. Scrolls at 2.5× river speed.

**`drawMountain`.** Lift the cone by half its height, scale by the
transform's scale, draw with rock.

---

## 10. `src/main.cpp`

The glue. Reading top to bottom:

**Constants.** Window size and title; the sky colour.

**`framebufferSizeCallback`.** `glViewport` to the new size, so resizing does
not stretch the image.

**`pressedOnce`.** The edge-detector: returns true only on the frame a key
goes from up to down, tracked through a `bool&` the caller owns. Without this
one tap of F would flip orbit on and off dozens of times while the key is
down.

**`printControls`.** Prints the full binding list once at startup — an explicit
lab requirement.

**`main`, setup.** In order, and the order matters:

1. `glfwInit`, hints for 3.3 core, `glfwCreateWindow`.
2. `glfwMakeContextCurrent` **then** `gladLoadGLLoader`. Any `gl…` call before
   the loader runs crashes.
3. `glfwSwapInterval(1)` for vsync; initial `glViewport`; register the resize
   callback.
4. `glEnable(GL_DEPTH_TEST)` — half of the depth fix. The other half is the
   `GL_DEPTH_BUFFER_BIT` in `glClear` below.
5. `glEnable(GL_CULL_FACE)` — legal because every primitive winds CCW. Also a
   development aid: a wrongly wound face simply vanishes.
6. Construct the `Shader`; abort if `ID == 0`. `Activate` it and set
   `diffuse0 = 0` once (all textures bind to unit 0).
7. Construct `Scene` (which loads every mesh and texture) and `Camera`.
8. Four `wasX` booleans for the edge-triggered keys; `printControls()`;
   seed `lastFrame`.

**`main`, render loop.** Each iteration:

- **Timing.** `deltaTime = now - lastFrame`. Everything that moves multiplies
  by this.
- **Input.** ESC closes. `camera.Inputs()` handles movement. Then the four
  edge-triggered toggles (F orbit, P projection, T textures, SPACE pause),
  each printing its new state. Then the held keys: `+`/`-` ramp `bladeSpeed`,
  `,`/`.` yaw the windmill heads.
- **Update.** `scene.Update(deltaTime)`.
- **Render.** `glClearColor` + `glClear(COLOR | DEPTH)`; activate the shader;
  recompute the aspect ratio from the current framebuffer size; set
  `projection`, `view` and `useTexture`; `scene.Draw(shader)`; swap buffers;
  poll events.

**Shutdown.** `scene.Delete()`, `shader.Delete()`, destroy the window,
`glfwTerminate`.

---

## The data flow, end to end

If you hold one picture in your head, make it this one:

```
Primitives::makeX()  →  MeshData (CPU vectors of Vertex)
                            │
                        Mesh(data)  →  VBO + EBO uploaded, VAO linked
                            │
Scene::drawX()  builds a model matrix, picks a colour and a Texture
                            │
                    Mesh::Draw(shader, model, colour, texture)
                            │
              sets uniforms → binds texture → binds VAO → glDrawElements
                            │
     default.vert: projection · view · model · position  →  screen
                            │
     default.frag: objectColor or texture sample  →  pixel
```

`main` supplies `view`, `projection` and `useTexture` once per frame from the
`Camera` and the T key. `Scene::Update` advances the angles. Everything else
is data tables in `Scene.cpp`'s constructor.
