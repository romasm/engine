Compile the Engine project. If Visual Studio has the solution open, trigger the build through VS COM automation to avoid file lock conflicts. Otherwise fall back to MSBuild.

## How to compile

Run the compilation script:
```bash
powershell -ExecutionPolicy Bypass -File "c:\Users\roman\Dropbox\Engine\build_output\compile.ps1"
```

Use `timeout: 300000` (5 minutes) since builds can take a while.

The script (`build_output/compile.ps1`):
1. Checks if `devenv.exe` is running
2. If VS is open with `Engine.sln` — builds via COM automation (`VisualStudio.DTE.17.0`), captures errors from Error List
3. If VS is closed or has a different solution — falls back to MSBuild directly
4. Configuration defaults to `Development|x64`

## After build

- If succeeded, report success to the user
- If failed via MSBuild, read `build_output/build_output.txt` with the Read tool to see errors
- If failed via VS COM, read `build_output/build_output.txt` for Error List items

## Troubleshooting
- Corrupted PDB (`LNK4020`): delete `source/obj/` directory and rebuild
- PCH locked by stale compiler: `powershell -Command "Stop-Process -Name 'cl' -Force"`

$ARGUMENTS
