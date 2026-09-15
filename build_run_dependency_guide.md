# Windmill Farm

A 3D countryside scene with nested rotational hierarchies, in OpenGL 3.3 Core.
See [windmill_farm_implementation_plan.md](windmill_farm_implementation_plan.md) for the phase plan.

**Current status: Phase 3 complete** — a 24-vertex cube spinning in perspective,
drawn via `glDrawElements` with depth testing on and GLSL loaded from disk.
See [phase1_to_3.md](phase1_to_3.md) for what Phases 1–3 added.

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
├── src/            main.cpp, glad.c, Shader, VAO/VBO/EBO, Vertex.h
├── shaders/        default.vert / default.frag
├── textures/       image files (Phase 8)
├── libs/           header-only third-party (stb_image.h)
├── include/        glad/ and KHR/ headers
└── build/          objects and WindmillFarm.exe (generated, gitignored)
```

## Controls

| Key | Action |
|---|---|
| `ESC` | Quit |

(The full control list grows through Phase 7 and Phase 10.)
