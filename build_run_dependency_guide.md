# Windmill Farm

A 3D countryside scene with nested rotational hierarchies, in OpenGL 3.3 Core.
See [windmill_farm_implementation_plan.md](windmill_farm_implementation_plan.md) for the phase plan.

**Current status: Phase 6 complete** — the full countryside scene: two windmills
with nested rotating hierarchies, farmhouse, water wheel, trees, fence, lamp
posts and sun, flat-shaded and animated.

- [phase1_to_3.md](phase1_to_3.md) — Shader class, buffer wrappers, MVP and depth
- [phase4_to_6.md](phase4_to_6.md) — primitive library, Mesh, windmill hierarchy, scene

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
| stb_image.h | to be dropped into `libs/` at Phase 8 |

## Layout

```
WindMill/
├── src/            main.cpp, glad.c, Scene, Primitives, Mesh, Shader, VAO/VBO/EBO
├── shaders/        default.vert / default.frag
├── textures/       image files (Phase 8)
├── libs/           header-only third-party (stb_image.h)
├── include/        glad/ and KHR/ headers
└── build/          objects and WindmillFarm.exe (generated, gitignored)
```

## Controls

| Key | Action |
|---|---|
| `,` / `.` | Yaw the windmill heads (temporary — folded into Phase 7's scheme) |
| `ESC` | Quit |

(The full control list grows through Phase 7 and Phase 10.)
