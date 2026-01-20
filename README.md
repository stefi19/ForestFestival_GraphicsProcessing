# Forest Festival Graphics

A small OpenGL scene renderer featuring a festival scene (hat and rabbit, Ferris wheel, swing, playground, rain, fog, shadows, skybox).

## Overview

- Entry: `main.cpp` — initializes the window, shaders, models, and contains the main loop and rendering passes.
- Core modules:
  - `Window` (`Window.h`, `Window.cpp`) — GLFW window and GL context setup.
  - `Shader` (`Shader.hpp`, `Shader.cpp`) — GLSL loader/compilation/linking and activation.
  - `Camera` (`Camera.hpp`) — camera transforms and movement API.
  - `Model3D` / `Mesh` (`Model3D.hpp/cpp`, `Mesh.hpp/cpp`) — OBJ loader (tinyobjloader), texture handling (stb_image), per-mesh buffers, and draw logic.
  - `SkyBox` (`SkyBox.hpp/cpp`) — cubemap loader and skybox rendering.

## Shaders

All shaders live in the `shaders/` folder.

- `basic.vert` / `basic.frag` — main scene shader: supports directional lighting, two spotlights, shadow mapping (PCF), texturing, and a localized fog effect centered on the hat.
- `depth.vert` / `depth.frag` — depth-only pass shader used to render the shadow map from the light's point of view.
- `rain.vert` / `rain.frag` — fullscreen rain overlay shader (animated procedural streaks).
- `skyboxShader.vert` / `skyboxShader.frag` — cube-map sampler for skybox rendering.

## Scene objects

Models loaded from `models/` (subfolders per object):
- Hat, Rabbit, Ferris Wheel, Wheel, Swing, Playground, IceCream, LeftHands, RightHands, Scene, MoreTrees, etc.

Each `gps::Model3D` contains one or more `gps::Mesh` objects with their own VAO/VBO/EBO and textures.

## Features & Behavior

- Shadow mapping with a high-resolution depth texture (2048x2048).
- Directional lighting + two spotlights targeted between hat and rabbit.
- Localized ellipsoidal fog centered around the hat with animated wobble and swirl.
- Rain overlay as a transparent fullscreen pass.
- Skybox drawn last using a cubemap.
- Simple animations: clap animation for hands, swing oscillation, rabbit appear/hide.
- Cinematic camera sequence that guides the camera through scene focuses.

## Controls

- ESC — exit.
- W / S — move forward / backward.
- A / D — strafe left / right.
- Arrow Up / Arrow Down — move up / move down.
- Q / E — rotate scene angle (affects model rotation used in some transforms).
- P — toggle clap animation (hands).
- C — start cinematic camera presentation (multi-phase camera movement and actions).
- I — toggle rabbit appearance (instant show/hide).
- Render mode keys:
  - `7` or `F7` — Solid (filled polygons).
  - `8` or `F8` — Wireframe.
  - `9` or `F9` — Polygonal / Flat shading.
  - `0` or `F10` — Smooth shading.
- Mouse movement: when the cursor is disabled, moving the mouse rotates the camera (mouse look).

## Build (Windows / Visual Studio)

1. Ensure dependencies are available and installed (GLFW, GLEW, GLM). The project expects the include and lib paths to be configured in the Visual Studio project.
2. Open `ForestFestivalGraphics.sln` in Visual Studio, select platform `x64` and configuration `Debug` (or `Release`), then build and run.

From a Developer Command Prompt you can build with MSBuild:

```powershell
msbuild ForestFestivalGraphics.sln /t:Build /p:Configuration=Debug /p:Platform=x64
```

If MSBuild is not on PATH, open the "Developer Command Prompt for VS" which sets up the environment.

## Run

- Ensure the `models/`, `shaders/`, and `skybox/` folders are available relative to the executable (the project already copies them into the `x64/Debug/` folder in the provided solution).
- Run the produced executable (e.g., `ForestFestivalGraphics.exe`) from the build output directory.

## Third-party components

- tinyobjloader (tiny_obj_loader.h) — OBJ parsing.
- stb_image (stb_image.h / stb_image.cpp) — image loading.
- GLFW — windowing and input.
- GLEW — OpenGL extension loading (non-macOS builds).
- GLM — math library for vectors and matrices.

## Notes on the repository

- The project contains a set of assets under `models/`, `shaders/`, and `skybox/`. Keep their relative layout when running the executable.
- Debug prints and development logging have been removed from the committed code; add temporary logs locally if needed for debugging.

## Next steps I can help with

- Generate a per-file API reference or inline documentation.
- Produce a small diagram of render passes and data flow.
- Create a CONTRIBUTORS or LICENSE file.

---

If you want the README in Romanian or with more details (example uniform descriptions, per-model bounding box values, or suggested shader tuning parameters), tell me which parts to expand.