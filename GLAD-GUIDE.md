# Using glad in a new OpenGL project

## Do I even need glad?

| What your code uses | Loader needed? |
|---|---|
| freeglut with old-style GL (`glBegin`, `glVertex2f`, `gluOrtho2D`), like the WindMill project | **No.** Just `#include <GL/glut.h>` and run. |
| GLFW + modern GL (shaders, `glGenBuffers`, VAOs) | **Yes**, either **glad** or **GLEW** (GLEW is already installed through MSYS2) |
| A tutorial that writes `#include <glad/glad.h>` (LearnOpenGL, most YouTube tutorials) | **Yes, glad**, so follow this guide |

glad is **not a library you install**. It is three source files that get copied into **each project**:

```
include/glad/glad.h
include/KHR/khrplatform.h
src/glad.c
```

Ready-made copies live in this template folder:
`E:\Study 4-1\🖼️  Image Lab\OpenGL-Glad-Template`
They were generated for OpenGL 4.6 with the **compatibility** profile, so they contain every OpenGL function, old and new.

---

## Option A: start from the template (easiest)

1. Copy the whole `OpenGL-Glad-Template` folder and rename the copy, e.g. `MyShaderProject`.
2. In VS Code choose **File → Open Folder…** and open the renamed folder.
3. Open `main.cpp` and click **Run** (or press `Ctrl+Alt+N`).
   You should see a window with a colored triangle. Press `Esc` to close it.
4. Replace the code in `main.cpp` with your own.

## Option B: add glad to a project you already created

1. From `OpenGL-Glad-Template`, copy the **`include`** and **`src`** folders into your project folder.
   Your project must look like this:

   ```
   MyProject/
   ├── main.cpp              ← the file you run (must be in this top folder)
   ├── include/
   │   ├── glad/glad.h
   │   └── KHR/khrplatform.h
   └── src/
       └── glad.c
   ```

2. At the top of your `.cpp`, **include glad first**, before GLFW or any other OpenGL header:

   ```cpp
   #include <glad/glad.h>   // always first
   #include <GLFW/glfw3.h>
   ```

3. Load glad **after** the window's context is current and **before** any `gl...` call:

   ```cpp
   GLFWwindow* window = glfwCreateWindow(800, 600, "My App", nullptr, nullptr);
   glfwMakeContextCurrent(window);

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
       std::cerr << "Failed to initialize glad\n";
       return -1;
   }
   // gl... calls are safe from here on
   ```

4. Click **Run**. Nothing else is needed: no settings, no `tasks.json`.

### Why no configuration is needed

The Run button calls `C:\Users\HP\.cpp-runner\run-cpp.ps1`. When your file includes `glad/glad.h`, the script automatically:

- compiles `glad.c` along with your file (it looks in the file's own folder, then `src/`, then `glad/src/`)
- adds the `include/` (or `glad/include/`) folder to the header search path
- links the right libraries (`-lglfw3 -lopengl32 -lgdi32`, plus freeglut if you use it)

It also detects freeglut, GLFW and GLEW, so the same Run button works for plain DSA code too.

---

## Building from the terminal instead (optional)

From inside the project folder:

```bash
g++ main.cpp src/glad.c -Iinclude -o main.exe -lglfw3 -lopengl32 -lgdi32
./main.exe
```

With freeglut instead of GLFW:

```bash
g++ main.cpp src/glad.c -Iinclude -o main.exe -lfreeglut -lglu32 -lopengl32 -lgdi32 -lwinmm
```

With freeglut, call `gladLoadGL();` right after `glutCreateWindow(...)`.

---

## Troubleshooting

| Error message | Cause | Fix |
|---|---|---|
| `fatal error: glad/glad.h: No such file or directory` | The `include` folder is missing or in the wrong place | Put `include/` next to the `.cpp` you are running (see the layout above) |
| `undefined reference to 'gladLoadGLLoader'` or `'glad_glClear'` | `glad.c` wasn't compiled | Put `glad.c` in `src/` (or next to your `.cpp`) |
| `#error OpenGL header already included, remove this include, glad already provides it` | Something included GL before glad | Move `#include <glad/glad.h>` to the very top |
| Program crashes at the first `gl...` call | glad was never loaded, or was loaded before `glfwMakeContextCurrent` | Call `gladLoadGLLoader(...)` right after `glfwMakeContextCurrent(window)` |
| `'glBegin' was not declared` | glad files generated with the **Core** profile | Use the template's files (compatibility profile), or regenerate them (below) |
| Red squiggles under glad/GL names, but the program builds and runs | VS Code opened a different folder, or IntelliSense hasn't caught up | Open the project folder itself (**File → Open Folder**), then `Ctrl+Shift+P` → **Developer: Reload Window** |
| `ld.exe: cannot open output file E:\Study 4-1\???  Image Lab\...: Invalid argument` | The linker can't write to a full path containing emoji or other non-ASCII characters | The Run button already handles this. In a terminal, `cd` into the project folder and use a relative name: `-o main.exe` |
| `GLFW/glfw3.h: No such file or directory` | GLFW package missing | In the **MSYS2 MINGW64** terminal: `pacman -S mingw-w64-x86_64-glfw` |
| Double-clicking the `.exe` gives "missing DLL" or "entry point not found" | `C:\msys64\mingw64\bin` isn't first on PATH | Run it from VS Code, or keep `C:\msys64\mingw64\bin` above `C:\MinGW\bin` in PATH |

---

## Regenerating glad (different version or extensions)

Only needed if a tutorial asks for specific extensions or a particular version.

1. Go to <https://glad.dav1d.de/>.
2. Choose:
   - **Language:** C/C++
   - **Specification:** OpenGL
   - **API → gl:** Version 4.6 (leave gles1/gles2/glsc2 at None)
   - **Profile:** Compatibility (works for everything) or Core (modern only)
   - **Options:** keep **Generate a loader** ticked
3. Click **Generate**, download `glad.zip`, and unzip it.
4. Replace your project's `include/` and `src/` folders with the ones from the zip.

**glad 2** (the newer site, <https://gen.glad.sh/>) works a bit differently:
- It produces `include/glad/gl.h` and `src/gl.c`.
- You `#include <glad/gl.h>`.
- You load it with `gladLoadGL(glfwGetProcAddress)`.

The Run button supports that layout too. Just don't mix glad 1 and glad 2 files in the same project.
