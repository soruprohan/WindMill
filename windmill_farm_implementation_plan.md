# Windmill Farm — Implementation Plan

**Project:** *Windmill Farm: A 3D Countryside Scene with Nested Rotational Hierarchies in OpenGL*
**Stack:** C++17, OpenGL 3.3 Core, GLFW, GLAD, GLM, stb_image

---

## Guiding Strategy

The scene is built **geometry first, lighting last**. Phases 0–7 produce a complete, animated, navigable, textured scene lit only by vertex colours. Phases 8–11 replace that flat colouring with a full Phong lighting system. This ordering means you always have something runnable to show, and lighting bugs never get tangled up with geometry bugs.

**One critical decision made up front:** even though lighting does not arrive until Phase 8, every vertex you generate from Phase 3 onward must already carry a **normal** and a **texture coordinate**. Retrofitting normals into a dozen mesh generators later is painful and error-prone. Define the vertex format once, fill it correctly, and simply ignore the unused attributes until you need them.

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;      // filled from Phase 3, used from Phase 8
    glm::vec2 texCoord;    // filled from Phase 3, used from Phase 7
    glm::vec3 color;       // placeholder colour, used Phases 3-7
};
```

---

## Phase 0 — Environment and Project Skeleton

**Goal:** A window opens, clears to a solid colour, and closes cleanly.

### Tasks

1. Install and link **GLFW** (windowing/input) and **GLAD** (function loader, configured for OpenGL 3.3 Core). Follow the setup PDF in your course materials.
2. Add **GLM** (header-only maths) and **stb_image.h** (texture loading) to the include path.
3. Set up the folder structure:

```
WindmillFarm/
├── src/
│   ├── main.cpp
│   ├── Shader.h / .cpp
│   ├── VAO.h / .cpp
│   ├── VBO.h / .cpp
│   ├── EBO.h / .cpp
│   ├── Texture.h / .cpp
│   ├── Camera.h / .cpp
│   ├── Mesh.h / .cpp
│   └── Primitives.h / .cpp
├── shaders/
│   ├── default.vert
│   └── default.frag
├── textures/
└── libs/
```

4. Write `main.cpp` with the standard loop: `glfwInit` → window hints (3, 3, core) → create window → `gladLoadGL` → `glViewport` → render loop with `glClear` + `glfwSwapBuffers` + `glfwPollEvents` → terminate.
5. Register a framebuffer size callback so resizing updates the viewport.

**Checkpoint:** A resizable window filled with a sky-blue colour.

**Pitfall:** If `gladLoadGL` fails, you almost certainly called it before `glfwMakeContextCurrent`.

---

## Phase 1 — Shader Class and First Triangle

**Goal:** GLSL shaders load from disk, compile, and draw a triangle.

### Tasks

1. Write a `Shader` class that reads `.vert` and `.frag` files into strings, compiles both, links a program, and reports compile/link errors through `glGetShaderInfoLog`.
2. Give it `Activate()`, `Delete()`, and uniform setter helpers (`setMat4`, `setVec3`, `setFloat`, `setInt`).
3. Write minimal shaders that pass a position through and output a fixed colour.
4. Draw a hardcoded triangle to verify the whole path works.

**Checkpoint:** A coloured triangle on screen.

**Pitfall:** Always print the shader info log even on success during early development — a silently failing shader produces a black screen that looks identical to a geometry bug.

---

## Phase 2 — Buffer Abstractions

**Goal:** VAO, VBO and EBO wrapped into reusable classes.

### Tasks

1. `VBO` — constructor takes `std::vector<Vertex>`, uploads with `glBufferData`. Methods: `Bind`, `Unbind`, `Delete`.
2. `EBO` — same pattern for `std::vector<GLuint>` indices.
3. `VAO` — with a `LinkAttrib(VBO&, layout, numComponents, type, stride, offset)` method so attribute setup is one line per attribute.
4. Rewrite the triangle to use these classes, then extend to an indexed rectangle to confirm the EBO path works.

**Checkpoint:** A rectangle drawn via `glDrawElements`.

---

## Phase 3 — Going 3D: MVP, Depth, and a Cube

**Goal:** A rotating cube in correct 3D perspective.

### Tasks

1. Add the three matrices as uniforms in the vertex shader:

```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

2. In the application, build `view` with `glm::lookAt` and `projection` with `glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f)`.
3. **Enable depth testing** — `glEnable(GL_DEPTH_TEST)` once at startup, and add `GL_DEPTH_BUFFER_BIT` to every `glClear`. Forgetting the second half is the single most common cause of "my cube looks inside-out".
4. Build a 24-vertex cube (four vertices per face, so each face gets its own normal) with positions, normals, texture coordinates and a placeholder colour.
5. Apply `glm::rotate` to `model` using `glfwGetTime()` to confirm animation works.

**Checkpoint:** A cube spinning in perspective with visibly correct face ordering.

**Note on matrix order:** GLM composes right-to-left, so `model = T * R * S` applies **scale first, then rotate, then translate** — which is what you want. Writing it in the other order will scale your rotated object along the wrong axes.

---

## Phase 4 — Primitive Geometry Library

**Goal:** Programmatic generators for every shape the scene needs.

### Tasks

Write functions in `Primitives.cpp` that each return `{vector<Vertex>, vector<GLuint>}`:

| Function | Used for |
|---|---|
| `makeCube()` | house body, windmill head, blades, fence posts, door, windows |
| `makePlane(subdivisions)` | ground |
| `makeCylinder(segments, radius, height)` | windmill tower, tree trunks, lamp posts, water wheel hub |
| `makeCone(segments, radius, height)` | tree foliage, roof cap |
| `makeSphere(sectors, stacks)` | sun, lamp bulbs |
| `makePrism()` | house roof (triangular prism) |

**Generation approach for the curved surfaces:** sweep an angle `θ` from 0 to 2π in `segments` steps. For each step compute `x = r·cos(θ)`, `z = r·sin(θ)`. Emit a ring of vertices at the bottom and another at the top, then stitch adjacent rings into quads (two triangles each).

**Normals matter now, even though lighting is later:**
- Cylinder side wall → `normalize(vec3(x, 0, z))`
- Cylinder caps → `(0, ±1, 0)`
- Sphere → `normalize(position)` (since it is centred at the origin)
- Cone side → tilted outward; the mathematically correct version accounts for the slope, but `normalize(vec3(x, r/h, z))` is a good approximation
- Cube → axis-aligned per face

**Also write a `Mesh` class** that owns a VAO/VBO/EBO triple and exposes `Draw(Shader&, glm::mat4 model)`. Every primitive is generated **once** at startup and re-drawn many times with different model matrices. Never regenerate geometry inside the render loop.

**Checkpoint:** A test screen showing one of each primitive side by side.

---

## Phase 5 — The Windmill: Nested Transformation Hierarchy

**Goal:** The centrepiece of the project — and the part your report is built around.

### The Hierarchy

```
Ground
 └── Windmill base position        (translate + scale)
      ├── Tower                     (cylinder)
      └── Head pivot                (rotate about Y — yaw)
           ├── Head housing          (cube)
           └── Hub pivot             (rotate about Z — blade spin)
                └── Blade × 4         (each offset by 90° about Z)
```

### Implementation

```cpp
glm::mat4 base = glm::translate(glm::mat4(1.0f), windmillPos);

// Tower
glm::mat4 tower = glm::scale(base, glm::vec3(1.0f, towerHeight, 1.0f));
cylinderMesh.Draw(shader, tower);

// Head — yaws about its own vertical pivot at the top of the tower
glm::mat4 headPivot = glm::translate(base, glm::vec3(0.0f, towerHeight, 0.0f));
headPivot = glm::rotate(headPivot, glm::radians(yawAngle), glm::vec3(0,1,0));
cubeMesh.Draw(shader, glm::scale(headPivot, headSize));

// Hub — inherits the yaw, adds its own independent spin
glm::mat4 hubPivot = glm::translate(headPivot, glm::vec3(0.0f, 0.0f, headDepth));
hubPivot = glm::rotate(hubPivot, glm::radians(bladeAngle), glm::vec3(0,0,1));

// Four blades, each rotated a further 90° around the hub axis
for (int i = 0; i < 4; ++i) {
    glm::mat4 blade = glm::rotate(hubPivot, glm::radians(90.0f * i), glm::vec3(0,0,1));
    blade = glm::translate(blade, glm::vec3(0.0f, bladeLength * 0.5f, 0.0f));
    blade = glm::scale(blade, glm::vec3(0.15f, bladeLength, 0.05f));
    cubeMesh.Draw(shader, blade);
}
```

The essential idea to be able to explain in your defence: **`hubPivot` is built from `headPivot`, so changing `yawAngle` swings all four blades with the head, while `bladeAngle` moves only the blades.** That is the parent-child inheritance the whole project is named after.

### The offset-then-scale trick

A unit cube is centred at its origin, so scaling it to blade length would stretch it in both directions from the hub. Translating by `bladeLength * 0.5` **before** scaling moves the blade so it extends outward from the hub only. The same trick applies to the tower and tree trunks (translate up by half the height so the base sits on the ground).

**Checkpoint:** Two windmills with independently spinning blades. Temporarily bind `yawAngle` to a key and confirm the blades swing with the head while continuing to spin.

---

## Phase 6 — Completing the Scene

**Goal:** Every object from the report is present and placed.

### Tasks

Write a small `Scene` class or a `drawScene()` function containing the placement of:

1. **Ground plane** — a large flat quad, scaled to roughly 60×60 units.
2. **Farmhouse** — cube body, triangular prism roof, small cubes inset slightly into the front face for door and windows. Push them out by ~0.01 units to avoid z-fighting.
3. **Water wheel** — a cylinder hub with 8–12 paddle cubes arranged radially. This is a **second, simpler hierarchy**: `wheelPivot = translate * rotateZ(wheelAngle)`, then each paddle at `rotateZ(i * 360/n) * translate(radius) * scale`.
4. **Trees** — cylinder trunk plus one or two cones. Write a `drawTree(glm::vec3 pos, float scale)` helper and call it 6–8 times with varied positions and scales.
5. **Fence** — a loop placing posts (thin cubes) at regular intervals along the farm boundary, with horizontal rails between them.
6. **Lamp posts** — cylinder pole, small sphere for the bulb. Record each bulb's world position in a `std::vector<glm::vec3>` — Phase 9 will need exactly these positions for the point lights.
7. **Sun** — a sphere placed high in the sky. Keep its position in a variable; it becomes the directional light source in Phase 9.

**Organisation tip:** store per-object transform data (position, rotation, scale) in small structs in a vector rather than hardcoding matrices inline. Repositioning the scene later then becomes a data edit, not a code edit.

**Checkpoint:** The complete scene, flat-shaded, with blades and water wheel turning.

---

## Phase 7 — Camera, Projection and Keyboard Control

**Goal:** Full navigation and interaction, still with no lighting.

### Tasks

1. **Camera class** holding `position`, `orientation`, `up`, plus `speed` and `sensitivity`. It produces the view matrix with `glm::lookAt(position, position + orientation, up)`.
2. **Movement** — W/S along `orientation`, A/D along `normalize(cross(orientation, up))`, E/R along `up`. Multiply all movement by `deltaTime` so speed is frame-rate independent:

```cpp
float currentFrame = glfwGetTime();
deltaTime = currentFrame - lastFrame;
lastFrame = currentFrame;
```

3. **Mouse look** — hide the cursor on right-click hold, compute pitch/yaw from cursor delta, and **clamp pitch to ±89°** to prevent the view flipping at the poles.
4. **Orbit mode** (key `F`) — instead of free-look, place the camera on a circle around a fixed look-at point:

```cpp
float x = target.x + orbitRadius * cos(orbitAngle);
float z = target.z + orbitRadius * sin(orbitAngle);
view = glm::lookAt(glm::vec3(x, orbitHeight, z), target, up);
```

5. **Projection toggle** (key `P`) — switch between `glm::perspective(...)` and `glm::ortho(-a*s, a*s, -s, s, 0.1f, 100.0f)` where `a` is aspect ratio and `s` a zoom scale around 10.
6. **Animation controls** — `+`/`-` to change blade speed, `SPACE` to pause and resume all animation.
7. **Console printout** — a `printControls()` function called once at startup listing every key and its action. This is an explicit requirement in your lab assignments; do not leave it to the end.

**Checkpoint:** You can fly around the scene, orbit it, and switch projections. **At this point the project is roughly 70% complete.**

---

## Phase 8 — Textures

**Goal:** Surfaces carry image detail instead of flat colour.

### Tasks

1. `Texture` class — `stbi_load` the image, `glGenTextures`, set wrap mode to `GL_REPEAT` and filtering to `GL_LINEAR` (with `GL_LINEAR_MIPMAP_LINEAR` for minification), `glTexImage2D`, `glGenerateMipmap`, then free the CPU-side pixels.
2. Call `stbi_set_flip_vertically_on_load(true)` — OpenGL's texture origin is bottom-left, image files are top-left. Skipping this gives upside-down textures.
3. Add `sampler2D` uniforms to the fragment shader and bind textures to texture units with `glActiveTexture(GL_TEXTURE0 + unit)`.
4. Source or make simple tiling textures: grass, wood planks, brick or plaster, roof tiles, metal.
5. For the ground plane, scale UVs (e.g. multiply by 20 in the vertex data) so the grass tiles rather than stretching across the whole plane.
6. Add a **texture toggle key** (`T`) that switches between textured and plain-colour rendering, satisfying the "toggle the features" requirement.

**Checkpoint:** Grass on the ground, wood on the windmill, brick on the house.

---

## Phase 9 — Phong Lighting: Directional Light

**Goal:** The lighting foundation, with a single light source.

### Tasks

1. Pass `aNormal` through the vertex shader into world space:

```glsl
Normal = mat3(transpose(inverse(model))) * aNormal;
FragPos = vec3(model * vec4(aPos, 1.0));
```

The `transpose(inverse(model))` normal matrix is required because **non-uniform scaling skews normals**. Your blades and tower are scaled non-uniformly, so this is not optional here. If performance matters, compute it on the CPU and pass it as a uniform.

2. Implement the three Phong components in the fragment shader:

```glsl
// Ambient
vec3 ambient = ambientStrength * lightColor;

// Diffuse
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(-light.direction);
float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * lightColor;

// Specular
vec3 viewDir = normalize(camPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
vec3 specular = specularStrength * spec * lightColor;

FragColor = vec4((ambient + diffuse + specular) * texColor, 1.0);
```

3. Set the directional light's direction from the sun's position toward the scene origin.
4. Pass `camPos` as a uniform each frame — the specular term needs the view vector.

**Checkpoint:** The scene is lit from one direction; faces angled away from the sun are visibly darker. **Blade rotation should now be far more readable**, because each blade changes brightness as it turns.

**Pitfall:** If everything is uniformly lit with no shading variation, your normals are almost certainly wrong or all pointing the same way. Debug by outputting `FragColor = vec4(normalize(Normal) * 0.5 + 0.5, 1.0)` — each axis direction should show as a distinct colour.

---

## Phase 10 — Multiple Light Types

**Goal:** Point lights, spot light, emissive, and per-component toggles.

### Tasks

1. **Restructure the shader with structs and functions:**

```glsl
struct DirLight   { vec3 direction; vec3 ambient, diffuse, specular; };
struct PointLight { vec3 position; float constant, linear, quadratic;
                    vec3 ambient, diffuse, specular; };
struct SpotLight  { vec3 position, direction; float cutOff;
                    float constant, linear, quadratic;
                    vec3 ambient, diffuse, specular; };

#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 CalcDirLight(DirLight l, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight l, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight l, vec3 normal, vec3 fragPos, vec3 viewDir);
```

`main()` then accumulates the results, gated by boolean uniforms.

2. **Point lights** at the lamp post bulb positions saved in Phase 6, with distance attenuation:

```glsl
float d = length(light.position - fragPos);
float attenuation = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);
```

Reasonable values for a scene of this scale: `constant = 1.0`, `linear = 0.09`, `quadratic = 0.032`.

3. **Spot light** — aimed at one windmill's base. The single cut-off angle version required by your lab:

```glsl
float theta = dot(lightDir, normalize(-light.direction));
if (theta > light.cutOff) { /* apply diffuse + specular */ }
else { /* ambient only */ }
```

Store `cutOff` as `cos(radians(angle))` and compare cosines directly — comparing angles would require an `acos` per fragment for no benefit.

4. **Emissive** — the sun and lamp bulbs use a separate, minimal shader that outputs their colour directly with no lighting calculation, so they appear to glow rather than being lit.

5. **Toggle uniforms** wired to keys, matching your lab assignment exactly:

| Key | Action |
|---|---|
| 1 | Directional light on/off |
| 2 | Point lights on/off |
| 3 | Spot light on/off |
| 4 | Emissive on/off |
| 5 | Ambient component on/off |
| 6 | Diffuse component on/off |
| 7 | Specular component on/off |

**Checkpoint:** Lamp posts cast pools of light that fade with distance; the spot light throws a visible circle; every toggle key produces a clear, immediate change.

**Pitfall:** Uniform arrays must be set element by element — build the name string (`"pointLights[0].position"`) in a loop. Caching the locations once at startup avoids a lot of redundant `glGetUniformLocation` calls in the render loop.

---

## Phase 11 — Materials and Specular Maps

**Goal:** Different surfaces respond to light differently.

### Tasks

1. Define a `Material` struct with `ambient`, `diffuse`, `specular` and `shininess`, and assign sensible values per object type:

| Surface | Specular | Shininess |
|---|---|---|
| Grass | very low | 4 |
| Wood / windmill | low | 16 |
| Brick / plaster | low-medium | 32 |
| Metal fittings, lamp posts | high | 128 |

2. Add **specular maps** — a greyscale texture sampled as `texture(material.specular, TexCoords).rgb` and multiplied into the specular term. This lets one surface have varying reflectivity, e.g. window glass bright on an otherwise matte house wall.

**Checkpoint:** Metal parts show tight bright highlights while grass shows essentially none.

---

## Phase 12 — Polish and Submission

### Tasks

1. Re-verify the console control list is complete and matches the actual bindings.
2. Tune the sky clear colour and light colours together — a warm sun with a slightly blue ambient reads far better than pure white on grey.
3. Optionally animate the sun in a slow arc, updating the directional light's direction so the whole scene shifts from morning to evening. This is a small amount of code for a large amount of visual impact during the demo.
4. Delete all GL objects on shutdown (`VAO.Delete()`, `shader.Delete()`, and so on).
5. Test at several window sizes to confirm the aspect ratio updates correctly.
6. Write a `README.md` with build instructions and the control list.
7. **Take screenshots at each toggle state** — combined lighting, ambient only, diffuse only, directional only, plus the orthographic and orbit views. These make the report far stronger and cost nothing to capture.

---

## Phase Summary

| Phase | Deliverable | Approx. effort |
|---|---|---|
| 0 | Window opens | Small |
| 1 | Shader class, triangle | Small |
| 2 | VAO/VBO/EBO classes | Small |
| 3 | 3D cube, MVP, depth | Small |
| 4 | Primitive library | **Large** |
| 5 | Windmill hierarchy | **Large** |
| 6 | Full scene assembled | Medium |
| 7 | Camera + all controls | Medium |
| 8 | Textures | Medium |
| 9 | Directional Phong lighting | **Large** |
| 10 | All light types + toggles | **Large** |
| 11 | Materials, specular maps | Small |
| 12 | Polish, screenshots, README | Small |

Phases 4, 5, 9 and 10 are where the real work sits. Everything else is comparatively mechanical.

---

## Debugging Reference

| Symptom | Most likely cause |
|---|---|
| Black screen | Shader failed to compile — check the info log |
| Objects render inside-out | `GL_DEPTH_BUFFER_BIT` missing from `glClear` |
| Textures upside-down | `stbi_set_flip_vertically_on_load(true)` not called |
| Everything uniformly bright, no shading | Normals wrong or all identical |
| Lighting distorts when an object is scaled | Missing `transpose(inverse(model))` normal matrix |
| Flickering surfaces touching each other | Z-fighting — offset coplanar faces slightly |
| Movement speed varies with framerate | `deltaTime` not applied to movement |
| Camera flips when looking straight up | Pitch not clamped to ±89° |
| Only the first point light works | Array uniforms not set index by index |

---

## Suggested Working Order

Phases 0–3 map almost directly onto Victor Gordan's tutorials 0–7, so you can move through them quickly. Phase 4 onward is your own work. Do not start Phase 9 until Phases 4–7 are genuinely finished — debugging a lighting equation against geometry you are still changing is the fastest way to lose a weekend.

Commit to git at the end of every phase. Each phase boundary is a working, demonstrable state, which makes them natural restore points.
