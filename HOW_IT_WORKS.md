# How This Project Works — A Plain-Language Tour

This document explains the Windmill Farm project **without assuming you know OpenGL**.
It stays at the "what does each file do and why does it exist" level. No line-by-line
code, no maths. Read this first; the phase notes (`phase1_to_3.md` etc.) go deeper.

---

## 1. What the program actually does

When you run it, a window opens showing a 3D countryside: two windmills with spinning
blades, a farmhouse, a water wheel, trees, a fence, lamp posts, and a sun in the sky.
You can fly around it with the keyboard and mouse, orbit it like a drone, switch between
two "camera lens" styles, and turn textures on and off.

That's it. Everything in this repo exists to make that window appear and respond to you.

---

## 2. The five-second mental model of OpenGL

You don't need to know OpenGL to follow the project, but you need one idea:

> **Your CPU program does not draw pixels. It hands lists of points and small programs
> to the graphics card (GPU), and the GPU draws the pixels.**

So the whole codebase is about three jobs:

| Job | Plain meaning | Which files do it |
|---|---|---|
| **Describe shapes** | "Here is a list of corner points that make a cube." | `Primitives`, `Vertex.h` |
| **Ship them to the GPU** | "GPU, keep a copy of these points and remember how to read them." | `VBO`, `EBO`, `VAO`, `Mesh`, `Texture` |
| **Tell the GPU how to paint them** | "GPU, run this tiny program on each point and each pixel." | `shaders/*.vert`, `shaders/*.frag`, `Shader` |

Then `Scene` decides *where* every shape goes, `Camera` decides *from where you look*,
and `main.cpp` runs the loop that repeats all of this ~60 times a second.

Two words you will see everywhere:

- **Shader** — a tiny program that runs *on the GPU*, not on your CPU. Written in a
  C-like language called GLSL, stored in the `shaders/` folder as plain text files.
- **Matrix** — a 4×4 grid of numbers that encodes "move this, rotate this, scale this."
  Multiplying a point by a matrix moves it. Multiplying two matrices chains the moves.
  The project uses the GLM library so you never write the maths yourself.

---

## 3. Folder map

```
WindMill/
├── src/                 All the C++ code (the program itself)
├── shaders/             Two tiny GPU programs, as text files
├── textures/            8 PNG images painted onto surfaces + the script that made them
├── include/             GLAD — a helper that lets C++ call OpenGL functions on Windows
├── libs/                stb_image.h — a single-file library that decodes PNGs
├── build/               Compiled output (generated, not in git)
├── build.ps1 / Makefile Two ways to compile the project
├── .vscode/             Ctrl+Shift+B wiring for VS Code
└── *.md                 Plan and per-phase notes
```

---

## 4. Each source file, one at a time

Think of these as layers, from "closest to raw OpenGL" at the bottom to "closest to
what you see" at the top.

### Layer 0 — third-party glue (you never edit these)

| File | What it is |
|---|---|
| `src/glad.c`, `include/glad/`, `include/KHR/` | On Windows, OpenGL functions aren't directly available to C++; GLAD looks them up at startup. Copied in, never touched. |
| `libs/stb_image.h` | Reads a `.png` file into raw pixel bytes. One header, no install. |
| GLFW (installed via MSYS2, not in repo) | Opens the window, reads keyboard/mouse, gives OpenGL a surface to draw on. |
| GLM (installed via MSYS2, not in repo) | Vector and matrix maths. `glm::vec3` is a 3D point; `glm::mat4` is a transform. |

### Layer 1 — the vertex format

**`src/Vertex.h`** — One small struct that says what "a point on a shape" carries:
its position, which way the surface faces (normal), where on a texture image it sits
(texCoord), and a colour. **Every shape in the project uses this same struct.** The
normal and colour aren't fully used yet, but they're filled in now so lighting (a future
phase) doesn't require rewriting every shape generator.

### Layer 2 — thin wrappers around GPU memory

OpenGL manages GPU objects through integer IDs and a lot of boilerplate. These four
classes hide that boilerplate so the rest of the code reads naturally.

| File | Plain meaning |
|---|---|
| `src/VBO.h/.cpp` | **Vertex Buffer Object.** "Copy this list of `Vertex` structs onto the GPU." |
| `src/EBO.h/.cpp` | **Element Buffer Object.** "Copy this list of index numbers onto the GPU." Indices say which three vertices form each triangle, so shared corners aren't stored twice. |
| `src/VAO.h/.cpp` | **Vertex Array Object.** "Remember that the position lives in the first 3 floats, the normal in the next 3..." — a recording of how to read a VBO. |
| `src/Shader.h/.cpp` | Reads the two text files from `shaders/`, asks the GPU to compile them, prints errors if any, and gives you `setMat4(...)`, `setVec3(...)` etc. to send values into the shader. |
| `src/Texture.h/.cpp` | Loads a PNG with stb_image, uploads it to the GPU, sets it to tile/repeat. |

You can read all five as: **"take a thing from the CPU, put it on the GPU, give me a
handle."**

### Layer 3 — shapes

**`src/Primitives.h/.cpp`** — Functions that *generate* geometry with maths instead of
loading model files:

| Function | Makes | Used for |
|---|---|---|
| `makeCube()` | a unit cube | house body, windmill head, blades, fence, door, windows |
| `makePlane()` | a flat square | the ground |
| `makeCylinder()` | a can shape | tower, tree trunks, lamp poles, wheel hub |
| `makeCone()` | a cone | tree foliage |
| `makeSphere()` | a ball | sun, lamp bulbs |
| `makePrism()` | a triangular roof shape | farmhouse roof |

Key convention: **every shape is 1 unit big and centred on the origin.** Nothing is
pre-sized. The scene stretches, spins, and moves each one with a matrix at draw time.
That's why a single cylinder mesh can be a tall thin tower *and* a short fat hub.

**`src/Mesh.h/.cpp`** — Takes the output of a `Primitives::make…()` call, creates a
VAO+VBO+EBO for it, and gives you one method: `Draw(shader, matrix, colour, texture)`.
Six meshes are built once at startup and drawn hundreds of times per frame with
different matrices. Geometry is *never* rebuilt inside the frame loop.

### Layer 4 — the world

**`src/Scene.h/.cpp`** — The "level design" file. It owns the six meshes and eight
textures, decides where everything sits, and has one `draw…()` helper per object type:
`drawGround`, `drawWindmill`, `drawHouse`, `drawWaterWheel`, `drawTree`, `drawFence`,
`drawLampPost`, `drawSun`. `Update(deltaTime)` advances the blade and wheel angles;
`Draw(shader)` calls every helper.

The most important idea in the whole project lives in `drawWindmill`:

```
Windmill base (position on the ground)
 └── Tower
 └── Head  (can turn left/right)
      └── Hub  (spins)
           └── Blade × 4
```

Each child's matrix is built *from its parent's matrix*. So turning the head turns the
hub and all four blades with it, while the blades keep spinning on their own. That's
the "nested rotational hierarchy" in the project title, and it's the thing you'll be
asked to explain in a defence.

Object placement (which tree goes where, how big) is stored as plain data in small
structs (`Transform`, `Windmill`), so moving things is a number edit, not a code edit.

### Layer 5 — the viewer

**`src/Camera.h/.cpp`** — Where you stand and where you look. Two modes:

- **Free-fly** — WASD to move, E/R up/down, hold right-mouse to look around.
- **Orbit** (`F`) — the camera circles a fixed point in the middle of the farm.

It also holds the projection toggle (`P`): **perspective** (far things look smaller,
like a real camera) vs **orthographic** (no shrinking with distance, like a blueprint).

The camera hands back two matrices each frame — "view" (where the eye is) and
"projection" (what lens) — and the shader uses them to squash the 3D world onto your
2D screen.

### Layer 6 — the conductor

**`src/main.cpp`** — Startup, the loop, shutdown. Roughly:

1. Open a window (GLFW), load OpenGL functions (GLAD), turn on depth testing so near
   things hide far things.
2. Compile the shader, build the `Scene`, build the `Camera`, print the controls list.
3. **Loop until the window closes:**
   - measure how much time passed since last frame (`deltaTime`)
   - read the keyboard (`camera.Inputs`, toggles for F/P/T/SPACE, +/- speed, ,/. yaw)
   - `scene.Update(deltaTime)` — advance animation
   - clear the screen to sky blue
   - send the camera matrices to the shader
   - `scene.Draw(shader)` — draw everything
   - swap the finished image onto the screen
4. Free everything and quit.

### The GPU side — `shaders/`

Two text files the GPU runs; the `Shader` class loads them.

- **`default.vert`** (vertex shader) — runs *once per corner point*. Its only real job:
  multiply the point by `projection × view × model` to find where it lands on screen,
  and pass the colour/texture-coordinate/normal along.
- **`default.frag`** (fragment shader) — runs *once per pixel*. Right now it just picks
  a colour: the texture's pixel if textures are on and this object has one, otherwise
  the flat `objectColor`. No lighting yet — that's what the next phases add here.

### `textures/`

Eight 256×256 PNGs (grass, bark, leaves, wood, brick, roof, stone, metal) that tile
seamlessly. `generate_textures.py` is the script that drew them procedurally with no
image editor — run it again if you want to tweak a pattern.

---

## 5. How one frame flows, end to end

```
keyboard/mouse ──► Camera ──► view + projection matrices ─┐
                                                          ▼
Scene::Update ──► blade/wheel angles ──► Scene::Draw ──► for each object:
                                                            model matrix
                                                            colour
                                                            texture
                                                               │
                                                               ▼
                                                     Mesh::Draw ──► GPU
                                                                     │
                                          default.vert (per point) ◄─┘
                                          default.frag (per pixel)
                                                     │
                                                     ▼
                                                 the window
```

---

## 6. How to build and run

| Way | Command |
|---|---|
| VS Code | `Ctrl+Shift+B` (Build), or Terminal → Run Task → **Build & Run** |
| PowerShell | `.\build.ps1 -Run` |
| MSYS2 MINGW64 shell | `make run` |

Both scripts compile every `.cpp`/`.c` in `src/` and link against GLFW + OpenGL. The
program **must be launched from the project root** so `shaders/grass.png`-style relative
paths work; both scripts handle that. Don't use the Code Runner button — it only
compiles one file and this project has many.

Full dependency notes: `build_run_dependency_guide.md`.

---

## 7. Controls (also printed to the console on startup)

| Key | Action |
|---|---|
| `W` `S` `A` `D` | Move (in orbit: zoom / swing) |
| `E` `R` | Up / down |
| `LEFT SHIFT` | Go faster |
| `RIGHT MOUSE` (hold) | Look around |
| `F` | Orbit ⇄ free-fly |
| `P` | Perspective ⇄ orthographic |
| `T` | Textures on / off |
| `SPACE` | Pause animation |
| `+` `-` | Blade speed |
| `,` `.` | Turn the windmill heads |
| `ESC` | Quit |

---

## 8. Where the project is and what's next

**Done (Phases 0–8):** window, shaders, GPU buffers, 3D maths, shape library, windmill
hierarchy, full scene, camera + controls, textures.

**Remaining (Phases 9–12):** lighting. The `normal` field in `Vertex.h` and the
`vNormal` pass-through in the shaders are already in place waiting for it. Lighting
work will land almost entirely in `default.frag`, plus a few new uniforms sent from
`Scene`/`main.cpp`, and key bindings `1`–`7` to toggle each light. See
`windmill_farm_implementation_plan.md` for the plan.

---

## 9. If you want to go one level deeper

Read in this order — each builds on the previous:

1. `phase1_to_3.md` — how a triangle gets to the screen (Shader, VAO/VBO/EBO, matrices)
2. `phase4_to_6.md` — Primitives, Mesh, the windmill hierarchy, Scene layout
3. `phase7_to_8.md` — Camera maths, orbit mode, textures
4. Then the code, bottom-up: `Vertex.h` → `VBO/EBO/VAO` → `Shader` → `Mesh` →
   `Primitives` → `Scene` → `Camera` → `main.cpp`
