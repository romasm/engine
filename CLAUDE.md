# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A 3D graphics engine built in C++17 targeting Windows/DirectX 11.3, featuring an Entity-Component-System (ECS) architecture, Lua scripting, Bullet Physics, and async asset loading.

## Build System

**Prerequisite: Compile third-party libraries first**
1. `thirdparty/Assimp/` → run `create_project.bat` → compile `Assimp.sln` (Debug & Release)
2. `thirdparty/BulletPhysics/` → run `create_project.bat` → compile `Bullet3Solution.sln` (Debug & Release)

**Generate Visual Studio 2022 solution:**
```bat
create_project.bat
```
This runs Premake5 to produce `Engine.sln`. Open it in VS2022 and build the **Core** project.

**Known build issue:** Remove `HVisuals.h/.cpp` and `VoxelRender.h/.cpp` from the Core project before building — these are unfinished and won't compile.

**Build configurations** (defined in `premake5.lua`):
- `Debug` — symbols, no optimization; defines `_EDITOR`, `WIN64`, `_DEV`, `_DEBUG`
- `Development` — optimized + symbols + LTO; defines `_EDITOR`, `WIN64`, `_DEV`
- `Release` — optimized + LTO; defines `_EDITOR`, `WIN64`

Output: `build/bin/core_d.exe` / `core_dev.exe` / `core.exe`

**Running the engine:**
```bat
build\launch_dev.bat      # Development build with console
build\launch_debug.bat    # Debug build
build\launch_release.bat  # Release build
```

Command-line options: `-c` (show console), `-s <script.lua>` (custom entry script)

## Architecture

### Startup & Main Loop

`source/main.cpp` → `source/MainLoop.h` drives the frame loop:
1. `JobSystem::Tick()`
2. `ResourceProcessor::Tick()` (async asset loading)
3. Lua `Main:onTick()` callback
4. `WindowsMgr::Tick()` (input/events)
5. `WorldMgr::UpdateWorlds()` (ECS update)
6. `HUD::Update()` / `HUD::Draw()`
7. `Window::Swap()` (present frame)

### ECS (`source/ECS/`)

- **Entity** — index + generation handle; managed by `EntityMgr`
- **World** (`World.h/.cpp`) — container for all systems; call `World::UpdateSystems()` each frame
- **Systems** — `TransformSystem`, `CameraSystem`, `StaticMeshSystem`, `PhysicsSystem`, `SkeletonSystem`, `LightSystem`, `ScriptSystem`, `VisibilitySystem`, `TriggerSystem`, `CollisionSystem`, `LineGeometrySystem`, `ShadowSystem`, `EnvProbSystem`, `ControllerSystem`
- **SceneGraph** — hierarchical parent/child relationships separate from raw transform data

### Rendering (`source/Render/`)

DirectX 11.3 device is owned by `Render.h/.cpp`. Key classes:
- `Material` / `MaterialMgr` — material definitions and shader assignment
- `GIMgr` — Global Illumination manager
- `RenderTarget` — render-to-texture
- `Buffer` — GPU buffers (vertex, index, constant, structured)
- `ShaderCodeMgr` — HLSL compilation, disk caching, hot-reload
- `RenderState` — blend/depth/rasterizer state management
- `Compute.h` — compute shader helpers
- `Frustum.h` — frustum culling

### Resource Management (`source/Managers/`)

`ResourceProcessor` handles async loading via the `JobSystem`. Mesh loading uses Assimp; texture loading uses DirectXTex. Shaders are compiled and cached to disk by `ShaderCodeMgr`.

### Scripting (`source/System/LuaVM.h`)

LuaJIT via LuaBridge. Engine calls `Main:Start()` once and `Main:onTick()` every frame. C++ types exposed to Lua include Log, Profiler, EngineSettings, and VolumePainter. Per-entity scripting runs through `ScriptSystem`.

### Configuration

Runtime settings in `build/config/engine_settings.cfg`: resolution, vsync, FPS cap (60), FOV, near/far planes, bloom, tone mapping, SMAA, AO. Keybindings in `build/config/keymaps/`.

## Live Engine Communication (DebugServer)

In `Debug` and `Development` builds (`_DEV` defined), the engine runs an embedded HTTP server on `http://127.0.0.1:7777`. Use it to inspect and control a running engine instance without restarting.

**Endpoints:**

| Method | Path | Description |
|---|---|---|
| `GET` | `/` | HTML index listing all endpoints |
| `GET` | `/log` | Last 256 log entries as `[{prefix, text}]` JSON |
| `GET` | `/logfile` | Path to the current log file on disk as `{"path":"..."}` |
| `POST` | `/exec` | Execute Lua code (raw body); runs on main thread next frame |

**Log prefixes:** `": "` info · `"ERROR: "` · `"WARNING: "` · `"_LUA: "` · `"_LUA_ERROR: "`

**Workflow for exec + verify:**
```bash
# Execute Lua (queued, runs next frame)
curl -X POST http://127.0.0.1:7777/exec -d 'error("my_test")'

# Get log file path, then search it (file has full unbounded history)
LOG=$(curl -s http://127.0.0.1:7777/logfile | python -c "import sys,json; print(json.load(sys.stdin)['path'])")
sleep 1
grep "my_test" "C:/Users/roman/Dropbox/Engine/build/$LOG"
```

**Source:** `source/System/DebugServer.h/.cpp` — only compiled under `#ifdef _DEV`.
**Invoke the `/engine` skill** to interact with a running engine instance interactively.

## Code Style

Use full words in variable and function names — no abbreviations or single-letter names. Code should read clearly without needing comments to explain what it does. Add comments only when the reasoning behind a decision isn't obvious from the code itself.

## Key Paths

| Purpose | Path |
|---|---|
| Engine source | `source/` |
| Precompiled headers | `source/stdafx.h` |
| Common data types | `source/Common/DataTypes.h` |
| Path constants | `source/Common/Pathes.h` |
| Third-party deps | `thirdparty/` |
| Runtime assets | `build/resources/` |
| Engine config | `build/config/engine_settings.cfg` |
| Build scripts | `build/launch_*.bat` |

## Third-Party Libraries

All external libraries must be placed in `thirdparty/`. Never add library source or binaries anywhere else in the repository.

| Library | Role |
|---|---|
| Assimp | 3D model loading (FBX, OBJ, glTF, …) — compiled locally |
| Bullet Physics | Physics simulation — compiled locally |
| DirectXTK | DirectX Toolkit helpers |
| DirectXTex | Texture processing |
| LuaJIT + LuaBridge | Lua scripting runtime and C++ binding |
| Dirent | POSIX directory iteration shim for Windows |
