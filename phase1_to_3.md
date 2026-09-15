# Phases 1 – 3

What was built after the Phase 0 window, and why each piece exists.

**End state:** a 24-vertex cube spinning in correct perspective, drawn through
`glDrawElements`, with depth testing on and GLSL loaded from disk at runtime.

---

## Phase 1 — Shader class and GLSL from disk

**Goal:** stop hardcoding shader source in C++; load, compile and link it from files.

### Files created

| File | Contents |
|---|---|
| `src/Shader.h` | `Shader` class interface |
| `src/Shader.cpp` | file loading, compilation, linking, error reporting, uniform setters |

### What it does

The constructor takes two paths, reads both files, compiles a vertex and a
fragment shader, links them into a program, and stores the handle in `ID`.

`Activate()` calls `glUseProgram`. `Delete()` releases the program and zeroes
`ID` so a double delete is harmless.

Uniform setters cover everything later phases need: `setBool`, `setInt`,
`setFloat`, `setVec2/3/4`, `setMat3`, `setMat4`.

### Two decisions worth noting

**The info log prints even on success.** A shader that compiles with warnings
and a shader that fails both produce a black screen, and they look identical
from the outside. `checkCompile` pulls the log whenever it is non-empty, not
only on failure, so problems surface immediately rather than at Phase 9.

**Uniform locations are cached.** `glGetUniformLocation` is a string lookup
into the driver, and the render loop sets `model` / `view` / `projection`
every frame. `Shader::location()` memoises the result in an
`unordered_map`. It also warns once if a uniform name is missing — which
catches typos and uniforms the GLSL compiler stripped because they were unused.

**If a shader file cannot be opened**, the error names the path and reminds you
that the program must run from the project root. `main` checks `shader.ID == 0`
and aborts rather than rendering to a blank screen.

---

## Phase 2 — Buffer abstractions

**Goal:** wrap the raw GL buffer calls so that later phases set up geometry in
a few readable lines instead of a dozen stateful calls.

### Files created

| File | Contents |
|---|---|
| `src/Vertex.h` | the `Vertex` struct — the single vertex format for the whole project |
| `src/VBO.h` / `.cpp` | vertex buffer wrapper |
| `src/EBO.h` / `.cpp` | index buffer wrapper |
| `src/VAO.h` / `.cpp` | vertex array wrapper with `LinkAttrib` |

### The vertex format

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;     // filled from Phase 3, used from Phase 9 (lighting)
    glm::vec2 texCoord;   // filled from Phase 3, used from Phase 8 (textures)
    glm::vec3 color;      // placeholder colour, used Phases 3-7
};
```

Normals and texture coordinates are filled in correctly **now**, even though
nothing reads them for several more phases. The alternative — retrofitting
normals into six or seven mesh generators once lighting arrives — is the kind
of change that breaks geometry that was previously working.

### The classes

`VBO` takes a `std::vector<Vertex>` and uploads it with `glBufferData`.
`EBO` does the same for `std::vector<GLuint>`, and additionally stores
`count`, so the draw call can read `ebo.count` instead of tracking the index
total separately.

`VAO::LinkAttrib(vbo, layout, numComponents, type, stride, offset)` collapses
the bind / `glVertexAttribPointer` / `glEnableVertexAttribArray` / unbind
sequence into one line per attribute. Setting up all four attributes is now
four lines.

### One ordering trap

A VAO records the element buffer binding as part of its state. Unbinding the
EBO while the VAO is still bound therefore *erases* the EBO from the VAO, and
the next `glDrawElements` reads from nothing. In `main.cpp` the VAO is unbound
first and the EBO only afterwards. `EBO::Unbind` carries a comment saying so.

---

## Phase 3 — 3D: MVP, depth testing, animated cube

**Goal:** a cube in correct perspective that rotates and self-occludes properly.

### Files changed

| File | Change |
|---|---|
| `shaders/default.vert` | added `model`, `view`, `projection` uniforms |
| `shaders/default.frag` | unchanged behaviour — outputs the placeholder colour |
| `src/main.cpp` | depth testing, the cube generator, and the MVP render loop |

### The matrices

```glsl
gl_Position = projection * view * model * vec4(aPos, 1.0);
```

GLM composes right-to-left, so the model matrix acts first and projection last.

`projection` is rebuilt every frame from the current framebuffer size, so
resizing the window keeps the aspect ratio correct rather than stretching the
cube. `view` comes from `glm::lookAt` at `(0, 1.5, 4)` aimed at the origin.

### Depth testing — both halves

```cpp
glEnable(GL_DEPTH_TEST);                                  // once, at startup
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // every frame
```

Doing only the first is the classic cause of a cube that looks inside-out: the
test runs, but stale depth values from the previous frame are never cleared.

### Why 24 vertices and not 8

Each face gets its own four vertices so it can carry its own normal
`(0,0,1)`, `(0,-1,0)` and so on. Sharing the 8 geometric corners would force
one averaged normal per corner, which rounds off the lighting on what should be
a hard-edged shape. Six faces × 4 vertices = 24 vertices, 36 indices.

Winding is counter-clockwise viewed from outside on all six faces, so back-face
culling can be switched on later without anything disappearing.

The generator is a `static` function in `main.cpp` for now. Phase 4 moves it
into `Primitives::makeCube()` alongside the cylinder, cone, sphere and prism.

### Animation

```cpp
float t = (float)glfwGetTime();
glm::mat4 model = glm::rotate(glm::mat4(1.0f), t * glm::radians(45.0f),
                              glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));
```

45°/second on a tilted axis, so several faces rotate into view rather than the
cube spinning flat.

---

## Verification

Built clean with `-std=c++17 -Wall`, no warnings. Runtime output:

```
OpenGL 3.3.0 Core Profile Context 22.20.44.221025
Renderer: AMD Radeon(TM) Graphics
GLSL: 4.60
Shader OK: shaders/default.vert + shaders/default.frag
```

Checks performed:

- **Shader path** — "Shader OK" with an empty info log: both stages compiled and
  the program linked with no diagnostics.
- **Uniforms** — no "uniform not found" warnings, so all three MVP uniforms
  resolved and are being written each frame.
- **Framebuffer** — client area measured at exactly 1280 × 720, cube centred in
  it, confirming `glViewport` matches the window.
- **Depth** — three faces visible with correct occlusion, clean shared edges, no
  z-fighting and no inside-out faces.
- **Animation** — two frames captured 1.5 s apart differ in 952 of 14,400
  sampled pixels, so `glfwGetTime` is driving the model matrix.
- **Shutdown** — closing the window exits through the normal path; every GL
  object is released via `Delete()`.

---

## File inventory after Phase 3

```
src/
├── main.cpp      window, cube generator, MVP render loop
├── Shader.h/.cpp GLSL loading, compiling, linking, uniforms    [Phase 1]
├── Vertex.h      the shared vertex format                      [Phase 2]
├── VBO.h/.cpp    vertex buffer                                 [Phase 2]
├── EBO.h/.cpp    index buffer                                  [Phase 2]
├── VAO.h/.cpp    vertex array + LinkAttrib                     [Phase 2]
└── glad.c        GL function loader                            [Phase 0]

shaders/
├── default.vert  MVP transform, passes colour/uv/normal through
└── default.frag  outputs the placeholder vertex colour
```

## Next

**Phase 4** — `Primitives.h/.cpp` with `makeCube`, `makePlane`, `makeCylinder`,
`makeCone`, `makeSphere` and `makePrism`, plus a `Mesh` class owning a
VAO/VBO/EBO triple and exposing `Draw(Shader&, glm::mat4 model)`. Every
primitive is generated once at startup and redrawn many times with different
model matrices — geometry is never regenerated inside the render loop.
