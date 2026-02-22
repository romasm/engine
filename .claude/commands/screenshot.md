Take a screenshot of a running application window and analyze it visually.

## How to use

Run the PowerShell screenshot script, then read the resulting PNG with the Read tool.

**Default** — capture the engine window (`core_dev` process):
```bash
powershell -ExecutionPolicy Bypass -File 'c:\Users\roman\Dropbox\Engine\.claude\scripts\screenshot.ps1'
```

**By process name** (e.g. `notepad`, `devenv`):
```bash
powershell -ExecutionPolicy Bypass -File 'c:\Users\roman\Dropbox\Engine\.claude\scripts\screenshot.ps1' -ProcessName 'notepad'
```

**By window title substring** (e.g. `Visual Studio`, `Engine`):
```bash
powershell -ExecutionPolicy Bypass -File 'c:\Users\roman\Dropbox\Engine\.claude\scripts\screenshot.ps1' -WindowTitle 'Engine'
```

**Custom output path:**
```bash
powershell -ExecutionPolicy Bypass -File 'c:\Users\roman\Dropbox\Engine\.claude\scripts\screenshot.ps1' -OutputPath 'c:\Users\roman\Dropbox\Engine\build_output\screenshot.png'
```

## Workflow

1. Parse `$ARGUMENTS` to determine what window to capture. If no arguments given, default to the engine (`core_dev`).
2. Run the screenshot script with appropriate parameters.
3. Read the output PNG using the **Read** tool (it supports images) — default path is `$TEMP/engine_screenshot.png`, which resolves to something like `C:\Users\roman\AppData\Local\Temp\engine_screenshot.png`.
4. Describe what you see in the screenshot to the user.

## Interpreting arguments

| User says | Action |
|---|---|
| *(nothing)* | Capture `core_dev` engine window |
| `engine` | Capture `core_dev` engine window |
| a process name like `devenv` | Use `-ProcessName 'devenv'` |
| a window title like `"My App"` | Use `-WindowTitle 'My App'` |

## Notes

- If the window is minimized, the script will restore it automatically before capturing.
- Uses `PrintWindow` with `PW_RENDERFULLCONTENT` flag which captures DirectX rendered content.
- If the script fails, the engine or target app may not be running — inform the user.

## User's request

$ARGUMENTS
