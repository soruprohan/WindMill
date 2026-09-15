# Phases 4 – 6

From a single spinning cube to the complete countryside scene.

**End state:** ground, two windmills with nested rotating hierarchies, a
farmhouse, a turning water wheel, eight trees, a fenced boundary, four lamp
posts and the sun — around 163 draw calls per frame from six meshes, all
flat-shaded, with back-face culling on.

---

## Phase 4 — Primitive library and the Mesh class

**Goal:** programmatic generators for every shape the scene needs, and a class
that owns the GPU buffers for one of them.

### Files created

| File | Contents |
|---|---|
| `src/Primitives.h` | `MeshData` struct and the six generator declarations |
| `src/Primitives.cpp` | cube, plane, cylinder, cone, sphere, prism |
| `src/Mesh.h` | `Mesh` class interface |
| `src/Mesh.cpp` | VAO/VBO/EBO ownership, attribute linking, `Draw` |

### The unit-primitive convention

Every generator returns a shape **centred on the origin and fitting inside a
1×1×1 box**. Nothing is sized for a particular object; the scene scales each
one with a model matrix.

That single decision pays off repeatedly. `glm::scale(vec3(w, h, d))` always
produces exactly those dimensions, and one cylinder mesh serves the windmill
tower, the tree trunks, the lamp posts, the windmill hub and the water wheel
axle. Six generators cover the whole scene.

### Normals

| Surface | Normal |
|---|---|
| Cube, prism | axis-aligned or face-perpendicular, per face |
| Plane | `(0, 1, 0)` |
| Cylinder wall | `normalize(x, 0, z)` |
| Cylinder / cone caps | `(0, ±1, 0)` |
| Sphere | `normalize(position)` — it is centred on the origin |
| Cone side | `normalize(H·cosθ, r, H·sinθ)` |

The cone deserves a note. Its surface slopes, so the normal tilts upward
rather than pointing straight out — it is **not** `normalize(x, 0, z)`. The
plan suggests `normalize(x, r/h, z)` as an approximation; the exact version
costs nothing extra, so `Primitives.cpp` uses the real one derived from the
surface tangents. The cone apex is emitted once per triangle rather than
shared, because the normal there differs for every surrounding face.

### Per-object colour

Primitives are generated with **white** vertex colours. The fragment shader
gained a `uniform vec3 objectColor` which is multiplied into the vertex
colour, and `Mesh::Draw` takes a colour argument that sets it.

Without this, every object sharing a shape would have to share a colour, or
the same primitive would have to be regenerated once per colour. One cylinder
mesh now draws a pale stone tower, a brown tree trunk and a dark metal lamp
post.

### The Mesh class and a core-profile trap

`Mesh` owns a VAO/VBO/EBO triple, links all four vertex attributes, and exposes
`Draw(shader, model, colour)`. Every primitive is generated **once** at
startup and redrawn many times with different matrices; geometry is never
regenerated inside the render loop.

One subtlety cost some thought. The `EBO` constructor calls `glBindBuffer` with
`GL_ELEMENT_ARRAY_BUFFER`, and in a core profile there is no default vertex
array object — that binding is only legal, and only *recorded*, while a VAO is
bound. But C++ runs member initialisers **before** the constructor body, so
binding the VAO in the body would be too late. `Mesh.cpp` binds it from inside
the initialiser list through a small named helper, with a comment explaining
why it is not simply the first line of the body.

`Mesh` is also non-copyable: it holds GL handles, and a copy would delete them
twice.

---

## Phase 5 — The windmill hierarchy

**Goal:** the centrepiece — nested transformations where a child inherits its
parent's motion and adds its own.

### Files created

Implemented in `Scene.cpp` as `Scene::drawWindmill`, alongside the `Windmill`
struct in `Scene.h`.

### The hierarchy

```
base                    translate to the windmill's spot, apply its scale
 ├── tower              cylinder, offset up by half its height
 └── headPivot          translate to tower top, then rotate about Y  (yaw)
      ├── head housing  cube
      └── hubPivot      translate forward, then rotate about Z  (blade spin)
           ├── hub cap  cylinder tipped 90° to lie along Z
           └── blade ×4 each a further 90° about Z
```

**The point of the whole project is one line of it:** `hubPivot` is built
*from* `headPivot`. Changing `yawDeg` swings all four blades with the head,
while `bladeAngle` moves only the blades. That is parent-child inheritance, and
it is what the report is built around.

### The offset-then-scale trick

A unit cube and a unit cylinder are centred on their own origin, so scaling one
to tower height would stretch it equally in both directions and sink half of it
underground. Translating **by half the height before scaling** moves it so it
extends in one direction only:

```cpp
glm::mat4 tower = glm::translate(base, glm::vec3(0.0f, TOWER_HEIGHT * 0.5f, 0.0f));
tower = glm::scale(tower, glm::vec3(TOWER_DIA, TOWER_HEIGHT, TOWER_DIA));
```

The same trick puts tree trunks and lamp poles on the ground, and makes each
water wheel spoke run from hub to rim instead of straddling the centre.

### Two independent windmills

Each `Windmill` carries its own `yawDeg` and `bladePhase`, so the two do not
turn in lockstep. `bladeAngle` is shared and advanced once per frame; the phase
offset is added per windmill.

---

## Phase 6 — Completing the scene

**Goal:** every object from the report present and placed.

### Files created

| File | Contents |
|---|---|
| `src/Scene.h` | `Transform` and `Windmill` structs, `Scene` class |
| `src/Scene.cpp` | the palette, the layout data, and all the draw helpers |

### Placement is data, not code

Per-object placement lives in structs in vectors rather than matrices hardcoded
inline, so repositioning the scene is a data edit:

```cpp
struct Transform {
    glm::vec3 position, rotationDeg, scale;
    glm::mat4 matrix() const;
};
```

Trees and the farmhouse are placed this way; windmills use the `Windmill`
struct, which adds the two animation angles.

### What is in the scene

| Object | Built from |
|---|---|
| Ground | one plane, 60×60, 8 subdivisions, UVs pre-scaled ×20 for Phase 8 |
| Windmills ×2 | cylinder tower, cube head, cylinder hub, 4 cube blades |
| Farmhouse | cube body, **prism** gable roof, cube door, 2 cube windows |
| Water wheel | 2 support posts, cylinder axle, 10× (spoke + rim segment + paddle) |
| Trees ×8 | cylinder trunk plus two overlapping cones |
| Fence | posts and two rails per span, four sides, with a gateway gap |
| Lamp posts ×4 | cylinder pole, sphere bulb |
| Sun | one sphere, high in the sky |

The door and windows are coplanar with the front wall, so they are pushed out
by **0.01 units** to avoid z-fighting.

The water wheel is the **second, simpler hierarchy**: one pivot rotating about
Z, with every spoke, rim segment and paddle hanging off it. Its support posts
are deliberately *not* on the pivot — they hold the axle while the wheel turns
inside them.

### Positions saved for later phases

`Scene` records the four lamp bulb world positions once during layout and
exposes them through `LampBulbPositions()`. Phase 10 puts a point light at each.
`SunPosition()` likewise feeds Phase 9's directional light. Neither is
recomputed per frame.

### Frame-rate independent animation

`Scene::Update(deltaTime)` advances `bladeAngle` and `wheelAngle` in degrees
per second, so animation speed does not vary with framerate. The `paused`,
`bladeSpeed` and `wheelSpeed` fields already exist for Phase 7 to bind keys to.

---

## Also changed

**`shaders/default.frag`** — added the `objectColor` uniform.

**`src/main.cpp`** — now builds a `Scene`, computes `deltaTime`, and enables
back-face culling. The camera is a slow automatic orbit, clearly marked as a
placeholder that Phase 7 replaces with the `Camera` class. `,` and `.` yaw the
windmill heads, which is the Phase 5 checkpoint the plan asks for.

**Back-face culling** was switched on deliberately. It roughly halves the
triangles rasterised, but the real reason to enable it during development is
that it turns every winding mistake into an immediately visible hole: a face
wound the wrong way simply disappears. Nothing disappeared, which is empirical
confirmation that all six generators wind counter-clockwise from outside.

---

## Verification

Built clean with `-std=c++17 -Wall`, zero warnings, and ran with an empty
stderr — no missing-uniform warnings, so `objectColor` resolves alongside the
three MVP matrices.

Checks performed against captured frames:

- **Every object present** — ground, both windmills, farmhouse with door and
  windows, water wheel, eight trees, fence with gateway, four lamp posts and
  the sun all render.
- **Winding** — with back-face culling enabled nothing vanished, confirming the
  cube, plane, cylinder, cone, sphere and prism generators are all correct.
  The sphere derivation was the one most at risk and is confirmed twice over,
  by the lamp bulbs and by the sun.
- **Depth** — no z-fighting anywhere, including the door and windows that sit
  flush against the farmhouse wall.
- **Animation** — blades and water wheel advance between frames; the two
  windmills sit at visibly different blade angles, confirming the per-windmill
  phase offset.
- **The hierarchy** — holding `.` for ~2 s yawed both heads about 130°. The
  head housings reoriented and the blade planes swung with them while the
  blades kept spinning. This is the Phase 5 checkpoint, and it confirms
  `hubPivot` inherits `headPivot`.
- **Sun** — confirmed rendering by scanning captured frames for its exact
  colour; it appears in the sky for the part of the orbit that faces it.

### Two fixes made during verification

The first build placed the **sun** at y = 27, which sat above the top of the
frame at every orbit position — it was rendering, but never visible. It was
lowered to y = 19 and the placeholder camera re-aimed above the horizon.

The **water wheel** originally had only a hub and ten paddles, which read as a
brown blob rather than a wheel. It gained rim segments between the spokes,
support posts, a larger radius, and a position clear of the farmhouse.

---

## File inventory after Phase 6

```
src/
├── main.cpp          window, placeholder orbit camera, render loop
├── Scene.h/.cpp      layout data, windmill hierarchy, all draw helpers  [5, 6]
├── Primitives.h/.cpp six unit-primitive generators                      [4]
├── Mesh.h/.cpp       VAO/VBO/EBO ownership, Draw                        [4]
├── Shader.h/.cpp     GLSL loading, compiling, linking, uniforms         [1]
├── Vertex.h          the shared vertex format                           [2]
├── VBO.h/.cpp        vertex buffer                                      [2]
├── EBO.h/.cpp        index buffer                                       [2]
├── VAO.h/.cpp        vertex array + LinkAttrib                          [2]
└── glad.c            GL function loader                                 [0]

shaders/
├── default.vert      MVP transform, passes colour/uv/normal through
└── default.frag      vertex colour × objectColor
```

## Next

**Phase 7** — the `Camera` class: WASD movement scaled by `deltaTime`, mouse
look with pitch clamped to ±89°, an orbit mode on `F`, a perspective/ortho
toggle on `P`, blade speed on `+`/`-`, pause on `SPACE`, and a `printControls()`
listing every binding. The placeholder orbit in `main.cpp` and the temporary
`,`/`.` yaw keys get folded into that scheme.

After Phase 7 the project is roughly 70% complete.
