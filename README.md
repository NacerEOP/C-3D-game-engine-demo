**3D Game Engine**

- **Short description:** This is a small C++ 3D game engine/demo focused on physically-based rendering (PBR), shadow mapping (directional/spot/point), and model loading via Assimp. It uses SFML for window/context and input handling, GLAD for OpenGL loading, and stb_image for image decoding.

**Features**
- PBR rendering pipeline
- Shadow mapping for directional, spot and point lights
- Model loading via Assimp (.gltf, .glb, .obj)
- Embedded texture handling for glTF/.glb
- Texture caching and management
- Simple scene graph: `Scene`, `GameObject`
- First-person camera + input via SFML
- Extensive logging via `Utils/Logger.hpp`

**Repository layout**
- `Core/` - application lifecycle (`Application`, `Engine`, `Time`)
- `Graphics/` - rendering (`Renderer`, `ShaderManager`, `TextureManager`, `Model`, `Mesh`, `Camera`, `Materials`)
- `Scene/` - `Scene` and `GameObject`
- `Input/` - `InputSystem` using SFML events and mouse capture
- `include/` - bundled third-party headers: `glad`, `glm`, `stb_image`, optional `nlohmann/json`
- `Resources/Shaders/` - GLSL shader sources used by the renderer
- `models/` - sample models used by the demo
- `src/` - `glad.c`, `stb_image_impl.cpp` implementations
- `Utils/` - `Logger.hpp`

**Prerequisites (Windows)**
- MinGW-w64 / `g++` supporting C++17 (or other compatible toolchain)
- `mingw32-make` or `make` (optional)
- SFML (3.x) development package (headers + libs)
- Assimp development package (headers + import library)
- OpenGL drivers and GPU

**Quick build & run (PowerShell)**
1. Edit `Makefile` or `build.bat` and set these variables to match your environment:
   - `SFML_PATH` (SFML include/lib path)
   - `ASSIMP_INCLUDE` (Assimp headers)
   - `ASSIMP_LIB` (Assimp import library path)

2. Build using the batch helper (recommended on Windows):
```powershell
.\build.bat
```

3. If you prefer `make` / `mingw32-make`:
```powershell
mingw32-make -j $env:NUMBER_OF_PROCESSORS
```

4. Run the produced executable:
```powershell
.\game.exe
```

**Notes & tips**
- The `Makefile` currently uses `-static` flags; for easier debugging remove `-static` or use dynamic linking while developing.
- Directional shadow map default size is set to 8192 in `Graphics/Renderer.cpp` — this can be reduced (e.g. 2048/4096) for compatibility with less capable GPUs.
- Logging goes to stdout via `Utils/Logger.hpp`. If you want file logging or log levels, consider adding a small logger backend.
- The repo was cleaned of many development debug prints and editor comments; the code should be ready for committing to GitHub.

**Suggested next steps before publishing**
- Add a `CMakeLists.txt` for cross-platform builds (I can help generate one).
- Add a `LICENSE` file (MIT, Apache-2.0, etc.) and a short `CONTRIBUTING.md`.
- Add `.gitignore` (ignore binaries, build artifacts, and local config). Example additions: `*.o`, `game.exe`, `*.log`, `/build/`, `/bin/`.

**Contact / Support**
If you want, I can:
- Convert the build system to `CMake` and provide a minimal `README` with step-by-step setup for Windows and Linux.
- Add a `LICENSE` and `.gitignore` suitable for GitHub.

---
