# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A 3D graphics engine built in C++20 targeting Windows/DirectX 11.3, featuring an Entity-Component-System (ECS) architecture, Lua scripting, Bullet Physics, and async asset loading.

## Build System

**Prerequisite: Compile third-party libraries first**
1. `thirdparty/Assimp/` → run `create_project.bat` → compile `Assimp.sln` (Debug & Release)
2. `thirdparty/BulletPhysics/` → run `create_project.bat` → compile `Bullet3Solution.sln` (Debug & Release)

**Generate Visual Studio 2022 solution:**
```bat
create_project.bat
```
This runs Premake5 to produce `Engine.sln`. Open it in VS2022 and build the **Core** project.

**Known build issues — already handled in `premake5.lua`:**
- `HVisuals.h/.cpp` and `VoxelRender.h/.cpp` are excluded via `removefiles {}` (unfinished, won't compile)
- Project kind is `ConsoleApp` (entry point is `int main()`, not `WinMain`)
- Windows SDK version is `10.0.26100.0`
- `/IGNORE:4099` suppresses missing-PDB linker warning from Bullet libraries
- After editing `premake5.lua`, always re-run `create_project.bat` before building

**Build configurations** (defined in `premake5.lua`):
- `Debug` — symbols, no optimization; defines `_EDITOR`, `WIN64`, `_DEV`, `_DEBUG`
- `Development` — optimized + symbols + LTO; defines `_EDITOR`, `WIN64`, `_DEV`
- `Release` — optimized + LTO; defines `_EDITOR`, `WIN64`

Output: `build/bin/core_d.exe` / `core_dev.exe` / `core.exe`

**Compiling from Claude Code:**
```bash
# Option 1: synchronous (waits for result, use timeout: 300000)
powershell -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'c:\Users\roman\Dropbox\Engine\Engine.sln' /p:Configuration=Development /p:Platform=x64 /m:1 /v:minimal 2>&1 | Tee-Object -FilePath 'c:\Users\roman\Dropbox\Engine\build_output\build_output.txt'; exit $LASTEXITCODE"

# Option 2: background (fire-and-forget, then read the output file)
powershell -Command "& 'C:\...\MSBuild.exe' ... | Tee-Object -FilePath '...\build_output\build_output.txt'" &
```
- Build output files go in `build_output/` (e.g. `build_output/build_output.txt`)
- Always use `timeout: 300000` for synchronous builds (full rebuild takes ~2 min)
- If PCH is locked by a stale `cl.exe`: `powershell -Command "Stop-Process -Name 'cl' -Force"`
- After errors, read `build_output/build_output.txt` with `Read` tool to get full compiler output

**Running the engine:**
```bat
build\launch_dev.bat      # Development build with console
build\launch_debug.bat    # Debug build
build\launch_release.bat  # Release build
```

Command-line options: `-c` (show console), `-s <script.lua>` (custom entry script)

**Launching from Claude Code (bash terminal):**

The `.bat` files use `cd bin` + `START`, which requires a Windows console. From bash, use `Start-Process` with an explicit working directory instead:

```bash
# Launch Development build in background
powershell -Command "Start-Process 'c:\Users\roman\Dropbox\Engine\build\bin\core_dev.exe' -WorkingDirectory 'c:\Users\roman\Dropbox\Engine\build\bin' -ArgumentList '-c'"

# Wait for startup (~5-6 s), then check last log entries
sleep 6
powershell -Command "(Invoke-WebRequest 'http://127.0.0.1:7777/log' -UseBasicParsing).Content"
```

**CRITICAL — PowerShell variable pitfall:**
When passing a PowerShell script to bash via `powershell -Command "..."`, bash expands `$` **before** PowerShell sees it. Variable assignments like `$var = ...` inside the quoted string will silently fail or error.

Rules for Claude Code:
- **Never** write `powershell -Command "... $var = ..."` — bash eats the `$`
- Use pipeline chaining instead: `(Invoke-WebRequest ...).Content | ConvertFrom-Json | Select-Object -ExpandProperty path`
- Capture PowerShell output into bash variables with `$(...)`: `LOG=$(powershell -Command "...")`

```bash
# Correct: get log file path via pipeline (no PS variables)
powershell -Command "(Invoke-WebRequest 'http://127.0.0.1:7777/logfile' -UseBasicParsing).Content | ConvertFrom-Json | Select-Object -ExpandProperty path"

# Correct: capture into a bash variable, then use it
LOG_REL=$(powershell -Command "(Invoke-WebRequest 'http://127.0.0.1:7777/logfile' -UseBasicParsing).Content | ConvertFrom-Json | Select-Object -ExpandProperty path")
tail -20 "c:/Users/roman/Dropbox/Engine/build/$LOG_REL"
```

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

## Lua Testing

Lua tests live in `tests/` and run against a live engine via the DebugServer `/exec` endpoint.

**Test files:**

| File | What it tests |
|---|---|
| `tests/framework.lua` | Shared assertion helpers (`expect_eq`, `expect_true`, `expect_near`, etc.) |
| `tests/test_math.lua` | `Vector3`, `Quaternion`, `CMath` (30 tests) |
| `tests/test_engine_globals.lua` | Presence and types of all critical engine globals (28 tests) |
| `tests/test_time.lua` | `Get_time()` / `Get_dt()` — types, monotonicity, range (6 tests) |
| `tests/test_stringlist.lua` | `StringList` C++ userdata: add, get, size, isolation (9 tests) |

**Run all suites (engine must be running):**
```bash
bash tests/run_tests.sh
```

**Run a specific suite:**
```bash
bash tests/run_tests.sh test_math
```

**Adding a new test suite:**
1. Create `tests/test_<name>.lua`
2. Start with `dofile("tests/framework.lua")` and `suite("<name>")`
3. Use `expect_eq`, `expect_true`, `expect_near`, `expect_type`, etc.
4. Call `suite_done()` at the end
5. Run against the live engine to verify all pass before committing

**Important gotchas:**
- `Get_dt()` returns **milliseconds** (~16.7 ms at 60 fps cap), not seconds
- Do not call `StringList:Get(index)` with an out-of-bounds index — it crashes (UB in C++)
- SceneMgr/WorldMgr Lua methods use colon syntax: `SceneMgr:IsWorld()` not `SceneMgr.IsWorld()`
- Each exec is queued and runs on the next engine frame; allow ~2–4 s before reading the log
- The log path from `/logfile` is relative to `build/`; resolve with `build/stats/<filename>`

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
