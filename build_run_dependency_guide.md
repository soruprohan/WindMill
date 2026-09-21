# Windmill Farm

A 3D countryside scene with nested rotational hierarchies, in OpenGL 3.3 Core.
See [windmill_farm_implementation_plan.md](windmill_farm_implementation_plan.md) for the phase plan.

**Current status: Phase 8 complete, plus scene improvements** — a fully textured
countryside scene ringed by mountains, with a river fed by a waterfall driving
the water wheel. Fly or orbit around it, with perspective/orthographic and
texture toggles. Lighting (Phases 9–11) is the remaining work.

- [phase1_to_3.md](phase1_to_3.md) — Shader class, buffer wrappers, MVP and depth
- [phase4_to_6.md](phase4_to_6.md) — primitive library, Mesh, windmill hierarchy, scene
- [phase7_to_8.md](phase7_to_8.md) — camera, projections, controls, textures
- [scene_improvements.md](scene_improvements.md) — river, waterfall, mountains (outside the plan)

## Build

In VS Code: `Ctrl+Shift+B` (Build), or run the **Build & Run** task.

From PowerShell:

```powershell
.\build.ps1          # build
.\build.ps1 -Run     # build and run
.\build.ps1 -Clean   # wipe build/ and rebuild
```

From an **MSYS2 MINGW64** shell:

```bash
make run
```

> Do **not** use the Code Runner "Run" button (`Ctrl+Alt+N`) on this project. That
> script compiles a single `.cpp` plus `glad.c`; from Phase 1 onward this project
> has several `.cpp` files and would fail to link.

The program must run with the project root as its working directory, so that
`shaders/` and `textures/` resolve. `build.ps1` and `make run` both handle this.

## Dependencies

| Library | Where it comes from |
|---|---|
| GLFW 3 | MSYS2 package `mingw-w64-x86_64-glfw` |
| GLM | MSYS2 package `mingw-w64-x86_64-glm` |
| GLAD | vendored in `include/glad`, `include/KHR`, `src/glad.c` (gl 4.6 compatibility) |
| stb_image.h | v2.14, vendored in `libs/` (copied from `CSE 4208 - Graphics/Lab_4`) |

## Layout

```
WindMill/
├── src/            main.cpp, glad.c, Camera, Scene, Primitives, Mesh, Texture, Shader, VAO/VBO/EBO
├── shaders/        default.vert / default.frag
├── textures/       11 tiling PNGs + generate_textures.py
├── libs/           header-only third-party (stb_image.h)
├── include/        glad/ and KHR/ headers
└── build/          objects and WindmillFarm.exe (generated, gitignored)
```

## Controls

| Key | Action |
|---|---|
| `W` `S` | Forward / backward (orbit: zoom) |
| `A` `D` | Strafe left / right (orbit: swing) |
| `E` `R` | Rise / descend (orbit: height) |
| `LEFT SHIFT` | Move faster (hold) |
| `RIGHT MOUSE` | Look around (hold; cursor hides) |
| `F` | Toggle orbit camera / free-fly |
| `P` | Toggle perspective / orthographic |
| `T` | Toggle textures on / off |
| `SPACE` | Pause / resume all animation |
| `+` / `-` | Blade speed up / down |
| `,` / `.` | Yaw the windmill heads |
| `ESC` | Quit |

`printControls()` prints this same list to the console at startup.
(Phase 10 adds the light-toggle keys 1-7.)
