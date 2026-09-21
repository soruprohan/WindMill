# Scene Improvements — River, Waterfall and Mountains

Work done **outside the implementation plan**, between Phase 8 and Phase 9, to
fix two things the plan's scene had wrong once it was actually on screen.

1. The water wheel was turning on dry grass inside the fence.
2. The whole farm looked like a square tile floating in a blue void.

Both are fixed with the primitives and classes that already existed. No new
classes, no new files in `src/`, and the drawing code follows the same
offset-then-scale pattern used everywhere else.

---

## What changed, in one picture

```
              mountains all the way round
   ┌──────────────────────────────────────────────┐
   │  ▲ waterfall on the west mountain            │
   │  ║                                           │
   │  ╚═══ river, flowing east ══► wheel ═══► into east mountain
   │         ┌─────────── fence ───────────┐      │
   │         │  windmills   house   trees  │      │
   │         └────────────────────────────┘      │
   │                                              │
   └──────────────────────────────────────────────┘
```

---

## 1. The river

A long, thin plane laid 0.06 units above the grass, running east–west just
behind the back fence, with a raised dirt bank down each side so it reads as a
channel cut into the ground rather than a strip painted on it.

```cpp
const float RIVER_Z     = -22.0f;    // just behind the fence at z = -18
const float RIVER_X0    = -38.0f;    // foot of the west mountain
const float RIVER_X1    =  46.0f;    // into the east mountain
const float RIVER_WIDTH =   5.6f;
```

### Making it flow

The fragment shader gained one uniform:

```glsl
uniform vec2 uvOffset;
...
base = texture(diffuse0, vTexCoord + uvOffset).rgb;
```

Every frame the scene accumulates how far the water has travelled
(`waterScroll += riverFlow * deltaTime`), and `drawRiver` converts that
distance into texture space and sets `uvOffset` before drawing. Everything else
draws with the offset at zero. That one sliding lookup *is* the flowing-water
effect — there is no other trickery.

The water texture itself (`water.png`) has its streaks stretched along U, the
flow direction, so the sliding reads as current rather than as a pattern
drifting sideways.

### Why the river has its own mesh

`makePlane` used to tile its texture equally along both axes. A river 84 units
long and 5.6 wide needs about twenty repeats one way and one or two the other,
so `makePlane` gained a three-argument form:

```cpp
MeshData makePlane(int subdivisions, float uvScaleU, float uvScaleV);
```

The old two-argument call still works and just passes the same value twice.
`riverPlane` and `fallPlane` are separate `Mesh` members built with this.

---

## 2. The water wheel — moved, and now genuinely driven by the river

The wheel was moved from inside the fence to `(-14, y, RIVER_Z)`, standing in
the river with its support posts in the water. Its axle height is set from the
wheel's own radius so the bottom paddles dip below the surface:

```cpp
wheelPos = glm::vec3(-14.0f, WHEEL_RADIUS - 0.35f, RIVER_Z);
```

### One number drives both the water and the wheel

The old independent `wheelSpeed` is gone. `Scene` now has a single public
`riverFlow` (units per second), and `Update()` derives the wheel's angular
speed from it:

```cpp
waterScroll += riverFlow * deltaTime;

// A paddle in the water moves at the water's speed, so the angular speed is
// riverFlow / radius (radians per second).
const float wheelSpeed = glm::degrees(riverFlow / WHEEL_RADIUS);
wheelAngle = std::fmod(wheelAngle + wheelSpeed * deltaTime, 360.0f);
```

Change `riverFlow` and the current speeds up and the wheel turns faster, in
proportion. Pause the animation and both stop together. The code now says what
the picture shows: the river turns the wheel.

### Turning the right way

The wheel's rotation sign was flipped. Seen from the camera side (+Z), a
positive rotation about Z is anticlockwise, which carries the bottom paddles
toward +X — the same direction the water flows. The bottom of the wheel and the
water under it now move together, which is what makes the wheel look pushed
rather than just spinning.

---

## 3. The mountains

The floating look came from seeing the edge of the ground plane against the
sky. Two changes remove it entirely.

**The ground is 160×160** instead of 60×60 (the grass texture repeats were
raised from 20 to 50 to keep the same tile size), so its edge is far away.

**A ring of fifteen mountains** stands at a radius of roughly 50–60 units. Each
is just the existing cone with a rock texture, sized so neighbouring bases
overlap into a continuous range. From anywhere inside the ring the horizon is
mountain, never ground-edge. They are placed from a data table in the
constructor exactly like the trees:

```cpp
const float mountainData[15][4] = {
    // x       z     baseDia  height
    { -52.0f, -22.0f, 30.0f, 26.0f },   // river source (west)
    {  56.0f, -20.0f, 28.0f, 22.0f },   // river exit (east)
    ...
```

`drawMountain` is four lines: lift the cone by half its height so the base sits
on the ground, scale, draw.

Ten more trees were also scattered between the fence and the mountains so the
extra land does not look empty.

### A cone generator fix this exposed

Standing close to a mountain showed a visible seam in the rock texture at every
segment edge. The cause was in `makeCone`: each side triangle had its own apex
vertex with a U coordinate halfway between its two base corners, so the U values
on the shared edge between neighbouring triangles did not match.

`makeCone` was rewritten to work the way `makeCylinder` already does — a bottom
ring and a "top ring" whose vertices all sit at the apex but each carry their
own angle's normal and U. One triangle per segment, texture continuous all the
way round. The tree canopies benefit too, though at their size the seam was
never noticeable.

It also gained separate U and V texture scales, because a mountain's
circumference is about three times its slope length:

```cpp
mountain(Primitives::makeCone(36, 30.0f, 10.0f))   // 30 repeats round, 10 up
```

---

## 4. The waterfall

Optional in the request, cheap to add. A textured plane leaned back against the
east-facing slope of the west mountain, where the river begins.

The geometry is worked out from that mountain's own dimensions rather than
typed in, so moving or resizing the mountain moves the waterfall with it:

```cpp
const Transform& src = mountains[0];             // the river's source
const float radius = src.scale.x * 0.5f;
const float height = src.scale.y;
const float slopeDeg = glm::degrees(std::atan2(height, radius));
```

A unit plane faces +Y. Rotating it about Z by `-slopeDeg` tilts it to face
+X-and-up, which is exactly the direction that mountain's east slope faces.
After that rotation the plane's own X axis points *down* the slope, so the same
`uvOffset` scrolling used for the river makes the water fall — at 2.5× the river
speed, because falling water is faster than a stream.

It is nudged 0.25 units off the slope along its normal so it does not z-fight
the cone underneath.

---

## Files touched

| File | Change |
|---|---|
| `src/Scene.h` | `riverFlow` replaces `wheelSpeed`; new meshes, textures, mountain list, three draw helpers |
| `src/Scene.cpp` | river/wheel/waterfall/mountain constants and layout; `drawRiver`, `drawWaterfall`, `drawMountain`; wheel moved and re-driven |
| `src/Primitives.h` / `.cpp` | `makePlane` U/V overload; `makeCone` ring rewrite with U/V scales |
| `shaders/default.frag` | `uvOffset` uniform |
| `src/main.cpp` | starting camera pitch −8° → −5° so more sky and mountain are in frame |
| `textures/generate_textures.py` | three new patterns |
| `textures/water.png`, `rock.png`, `dirt.png` | generated |

Nothing new in `src/` — the improvements live in the existing files.

---

## Verification

Clean build, zero warnings, empty stderr; all eleven textures load.

- **Water moves.** Two frames 1.5 s apart from a stationary camera: 46% of
  pixels in the river band changed, 0% in the grass below it.
- **Wheel is in the water.** Close-up confirms the paddles dip below the
  surface and the wheel stands behind the fence with the range behind it.
- **Horizon is gone.** From the starting camera and from orbit, the view is
  bounded by mountains in every direction; the ground edge is never visible.
- **Cone seam fixed.** The same close-up that exposed the seam shows continuous
  rock texture across segment edges after the rewrite.
- **The whole chain reads.** From the east side of the orbit, one frame shows
  the waterfall on the west mountain, the river running behind the farm, the
  wheel in it, and the river disappearing into the east range.

## What this supersedes in earlier write-ups

- `phase4_to_6.md` describes the ground as 60×60, the wheel inside the fence
  with its own `wheelSpeed`, and eight trees. All three are now different.
- `phase7_to_8.md` lists eight textures; there are eleven.

Those documents are left as written — they describe the state at the end of
their phases.
