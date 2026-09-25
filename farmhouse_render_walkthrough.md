# How One Object Gets Rendered — the Farmhouse, Traced End to End

This follows a single object — the farmhouse body — from "a function call in
`Scene.cpp`" all the way to "coloured pixels on your screen". Everything else
in the scene (windmills, trees, the river...) goes through exactly this same
pipeline; the farmhouse is just a clear, simple example to trace by hand.

Read [CODE_WALKTHROUGH.md](CODE_WALKTHROUGH.md) first if any class name here
(`Mesh`, `Shader`, `VAO`...) is unfamiliar — this document assumes you know
what each one is and focuses purely on the sequence of events for one object.

---

## The house is actually 4 separate draw calls

`Scene::drawHouse` draws the body, the roof, the door, and two windows — five
`Mesh::Draw` calls in total, one straight after another:

```cpp
void Scene::drawHouse(Shader& shader, const Transform& t)
{
    const glm::vec3 bodySize(6.0f, 3.0f, 4.6f);
    const glm::vec3 roofSize(6.7f, 2.1f, 5.1f);

    glm::mat4 base = t.matrix();

    glm::mat4 body = glm::translate(base, glm::vec3(0.0f, bodySize.y * 0.5f, 0.0f));
    cube.Draw(shader, glm::scale(body, bodySize), C_WALL, &texBrick);
    ...
```

We'll trace just the **body** (the brick box) — the roof, door and windows
follow the identical pattern, each with its own offset and size.

---

## Stage 0 — Before the program even runs a frame (happens once, at startup)

The body isn't a "house shape". It's the same **unit cube** every cube-shaped
object in the scene uses — windmill heads, fence posts, the door, the
windows, all of it.

1. **`Primitives::makeCube()`** builds 24 `Vertex` structs (position, normal,
   texCoord, white color) and 36 indices, entirely in CPU memory. See
   [CODE_WALKTHROUGH.md §5](CODE_WALKTHROUGH.md).
2. **`Mesh cube(Primitives::makeCube())`** — one of `Scene`'s member
   variables — takes that CPU data and uploads it to the GPU exactly once:
   a `VBO` holds the 24 vertices, an `EBO` holds the 36 indices, a `VAO`
   records how to read them.

After this point, **the cube shape lives entirely on the GPU** and is never
touched from the CPU side again. Drawing it 500 times in a frame re-uses that
same uploaded data 500 times — nothing is re-uploaded per object or per frame.

---

## Stage 1 — Every frame: the render loop reaches the house

`main.cpp`'s loop calls `scene.Draw(shader)`, which calls
`drawHouse(shader, house)` — `house` being a `Transform` set once in
`Scene`'s constructor:

```cpp
house.position    = glm::vec3(1.0f, 0.0f, 7.5f);
house.rotationDeg  = glm::vec3(0.0f, -18.0f, 0.0f);
// house.scale is left at the Transform default: (1, 1, 1)
```

---

## Stage 2 — Building the body's model matrix

This is the "where does this specific box go in the world" step. Three
matrices get combined:

```cpp
glm::mat4 base = t.matrix();
glm::mat4 body = glm::translate(base, glm::vec3(0.0f, bodySize.y * 0.5f, 0.0f));
cube.Draw(shader, glm::scale(body, bodySize), C_WALL, &texBrick);
```

**`t.matrix()`** (`Transform::matrix()` in `Scene.cpp`) builds the house's
overall placement:

```cpp
glm::mat4 Transform::matrix() const
{
    glm::mat4 m = glm::translate(glm::mat4(1.0f), position);      // move to (1, 0, 7.5)
    m = glm::rotate(m, glm::radians(rotationDeg.y), {0,1,0});      // turn -18° around Y
    m = glm::rotate(m, glm::radians(rotationDeg.x), {1,0,0});      // (0°, no-op here)
    m = glm::rotate(m, glm::radians(rotationDeg.z), {0,0,1});      // (0°, no-op here)
    m = glm::scale(m, scale);                                      // (1,1,1, no-op here)
    return m;
}
```

**Then**, still in `drawHouse`, two more steps stack on top of `base`:

- `translate(base, (0, 1.5, 0))` — lifts the box up by half its height
  (`bodySize.y * 0.5 = 1.5`). This is the "offset before scale" trick used
  everywhere in this project: a unit cube is centred on its own origin, so
  without this the box would stick half into the ground.
- `scale(body, (6, 3, 4.6))` — stretches the 1×1×1 unit cube into a
  6×3×4.6 box.

The final matrix handed to `cube.Draw` is the product of all of these,
applied right-to-left:

```
model = T(1, 0, 7.5) · Ry(-18°) · T(0, 1.5, 0) · S(6, 3, 4.6)
```

**What this one matrix means, read right to left:** take the unit cube,
stretch it into a 6×3×4.6 box, lift it so its base sits on the ground instead
of straddling it, spin it -18° so it isn't perfectly axis-aligned, then move
the whole thing out to where the house sits in the farm.

### Tracing one actual corner through it

Take one of the cube's 24 vertices: the top-right corner of the front face,
local position `(0.5, 0.5, 0.5)`.

| Step | Operation | Result |
|---|---|---|
| Start | local cube corner | `(0.5, 0.5, 0.5)` |
| Scale `(6, 3, 4.6)` | multiply each axis | `(3.0, 1.5, 2.3)` |
| Translate `(0, 1.5, 0)` | lift up by half the height | `(3.0, 3.0, 2.3)` |
| Rotate -18° around Y | spins around the vertical axis | `(2.14, 3.0, 3.11)` (rounded) |
| Translate `(1, 0, 7.5)` | move to the house's spot | `(3.14, 3.0, 10.61)` (rounded) |

That last number, `(3.14, 3.0, 10.61)`, is this corner's actual position **in
the world** — you could point the free-fly camera there and see it. The `y`
coordinate landing at exactly `3.0` isn't a coincidence: `bodySize.y` is 3,
so the top of the box should be 3 units off the ground, and it is.

This exact sequence — scale, translate, rotate, translate — runs for **all
24 corners of the body**, and it runs again from scratch for the roof, the
door, and each window, each with its own numbers. None of this math happens
on the CPU at runtime, though — see the next stage.

---

## Stage 3 — Handing it to the GPU: `Mesh::Draw`

```cpp
void Mesh::Draw(Shader& shader, const glm::mat4& model, const glm::vec3& color,
                const Texture* texture) const
{
    shader.setMat4("model", model);          // upload the matrix we just built
    shader.setVec3("objectColor", color);    // upload C_WALL (a pale wall colour)

    const bool textured = (texture != nullptr) && texture->Valid();
    shader.setBool("hasTexture", textured);  // true here - texBrick was passed in
    if (textured) texture->Bind();           // make texBrick the active texture

    vao.Bind();                                          // "use the cube's layout"
    glDrawElements(GL_TRIANGLES, ebo.count, GL_UNSIGNED_INT, 0);   // the actual draw
    vao.Unbind();
}
```

`shader.setMat4("model", model)` doesn't do any math — it just copies the
16 numbers of that matrix into GPU memory, where the shader will read it.
The CPU's job for this object ends at `glDrawElements`. Everything from
here on runs **on the GPU**.

Two uniforms were already set once, earlier that same frame, by `main.cpp` —
not per object, just once for the whole frame:

```cpp
shader.setMat4("projection", camera.ProjectionMatrix(aspect));
shader.setMat4("view", camera.ViewMatrix());
```

So by the time `glDrawElements` runs for the body, the shader has all three
matrices available: `model` (just set), `view` and `projection` (set once
for the frame).

---

## Stage 4 — The vertex shader: 24 times, once per corner

`glDrawElements` triggers `shaders/default.vert` to run once for **each of
the cube's 24 vertices**. The GPU can run many of these at once in parallel —
this isn't a loop happening one at a time.

```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    vColor    = aColor;
    vTexCoord = aTexCoord;
    vNormal   = aNormal;
}
```

For our traced corner, `aPos` is `(0.5, 0.5, 0.5)` — the raw data sitting in
the VBO, untouched since Stage 0. The one line `gl_Position = projection *
view * model * ...` does, **in a single GPU instruction**, everything Stage 2
worked out by hand:

1. `model * aPos` → the world position we calculated: `(3.14, 3.0, 10.61)`
2. `view * (that)` → shifts and rotates the whole world so the camera sits at
   the origin looking down its own forward direction. Where exactly this
   lands depends on where the camera currently is — that's why `view` is
   rebuilt every frame from `Camera::ViewMatrix()`.
3. `projection * (that)` → squashes the result into **clip space**, the
   funnel-shaped region the GPU knows how to flatten onto a 2D screen. Points
   further from the camera end up compressed more, which is what makes
   distant objects look smaller.

`vColor`, `vTexCoord` and `vNormal` are just copied through unchanged, to be
picked up by the fragment shader next.

---

## Stage 5 — Between the two shaders: rasterization

Once all 24 transformed corners are known, the GPU groups them into the 12
triangles the index buffer describes (2 triangles per face × 6 faces) and
works out **which pixels on screen each triangle covers**. This step has no
GLSL code behind it — it's fixed-function hardware — but it's the reason the
fragment shader below runs once *per pixel*, not once per vertex.

For every pixel inside a triangle, the GPU also **interpolates** `vColor`,
`vTexCoord` and `vNormal` — blending the values from that triangle's three
corners based on how close the pixel is to each one. A pixel near the middle
of a brick-wall triangle gets a `vTexCoord` partway between its three
corners' UVs, which is what makes the brick texture look continuous rather
than blocky.

---

## Stage 6 — The fragment shader: once per pixel

```glsl
uniform vec3 objectColor;
uniform sampler2D diffuse0;
uniform bool useTexture;
uniform bool hasTexture;
uniform vec2 uvOffset;

void main()
{
    vec3 base = objectColor;

    if (useTexture && hasTexture)
        base = texture(diffuse0, vTexCoord + uvOffset).rgb;

    FragColor = vec4(base * vColor, 1.0);
}
```

For a pixel inside the brick wall:

- `objectColor` is `C_WALL`, uploaded back in Stage 3.
- `useTexture` is the global T-key toggle (`main.cpp`); `hasTexture` was set
  `true` because `&texBrick` was passed to `Draw`. With both true, `base`
  becomes a sample from `brick.png` at this pixel's interpolated `vTexCoord`
  (`uvOffset` is zero here — it's only non-zero for the river and waterfall).
- `vColor` is white (every generated vertex starts white — see
  `Primitives.h`), so `base * vColor` doesn't change anything; the colour
  attribute is present but currently unused for anything visible.
- `FragColor` is the final RGBA colour written for this one pixel.

If `T` is pressed and textures are off, `useTexture` becomes false and this
pixel falls back to the flat `objectColor` instead — the whole house turns
into flat brick-coloured panels with no pattern, which is exactly the
toggle's intended effect.

---

## Stage 7 — Depth test, then the pixel actually appears

Before a fragment's colour is allowed to land in the framebuffer, OpenGL
compares its depth (how far from the camera it is) against whatever is
already stored for that pixel. If something nearer was already drawn there,
this pixel is discarded — this is what stops the far wall of the house from
painting over the near wall. `glEnable(GL_DEPTH_TEST)` in `main.cpp` turns
this check on once at startup, and `GL_DEPTH_BUFFER_BIT` in the per-frame
`glClear` resets it fresh every frame.

This is also why the door and windows are pushed out by `0.01` units in
`drawHouse` — they sit essentially on the same surface as the wall behind
them, and without that tiny offset, which one wins the depth test would be
decided by floating-point rounding and could flicker between frames
(z-fighting).

Once a pixel survives the depth test, its colour is written into the
framebuffer. After every object in `Scene::Draw` has been processed this
same way, `glfwSwapBuffers(window)` in `main.cpp` shows that completed frame
on screen — and the whole sequence repeats for the next frame, typically 60
times a second.

---

## The whole trip, compressed to one line each

```
Primitives::makeCube()      →  24 vertices + 36 indices, in CPU memory       (once, at startup)
Mesh(data)                  →  uploaded to a VBO/EBO/VAO on the GPU          (once, at startup)
Scene::drawHouse()          →  builds the model matrix: T · Ry · T · S      (every frame)
Mesh::Draw()                →  uploads model/objectColor/texture uniforms   (every frame)
glDrawElements()            →  tells the GPU to actually draw               (every frame)
default.vert (x24)          →  gl_Position = projection · view · model · aPos
rasterizer                  →  works out which pixels the triangles cover, interpolates vColor/vTexCoord/vNormal
default.frag (x pixels)     →  samples brick.png, writes FragColor
depth test                  →  keeps only the nearest pixel at each spot
glfwSwapBuffers()           →  the finished frame appears on screen
```

Everything above the `default.vert` line happens once per shape, at startup.
Everything from `Scene::drawHouse()` down happens **every single frame**, for
**every single object** in the scene — the farmhouse body is one of roughly
210 such draw calls `Scene::Draw` makes each frame (mostly the water wheel's
paddles, the trees' cones, and the fence's posts and rails).
