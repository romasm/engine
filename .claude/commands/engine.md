You are interacting with a live running instance of the Engine via its DebugServer HTTP API on http://127.0.0.1:7777.

## Your capabilities

**Read the log** — `GET /log` returns the last 256 entries as JSON. For full history use the log file:
```bash
curl -s http://127.0.0.1:7777/logfile
# returns {"path":"../stats/log_....log"} — resolve relative to build/bin/
```

**Execute Lua** — `POST /exec` with raw Lua in the body. Execution is queued and runs on the engine main thread next frame. There is no synchronous return value — verify results by reading the log file afterward.
```bash
curl -s -X POST http://127.0.0.1:7777/exec -d '<lua code here>'
sleep 1
grep "<search_term>" "/c/Users/roman/Dropbox/Engine/build/<log_path>"
```

**Known Lua limitations:**
- `LOG(...)` is a C++ macro — not available in Lua
- To produce visible log output from exec, use `error("message")` which appears as `_LUA_ERROR:` in the log
- Registered Lua API: `Util.Log.Size()`, `Util.Log.Text(i)`, `Util.Log.Prefix(i)`, `Util.Log.ResetUpdates()`

## Workflow

1. Check the engine is running: `curl -s http://127.0.0.1:7777/`
2. Get the log file path: `curl -s http://127.0.0.1:7777/logfile`
3. Note the current end of the log file (line count or last few lines) so you can see only new output
4. POST your Lua code to `/exec`
5. Wait ~1 second (one or more frames), then grep the log file for new output
6. Report findings to the user

## When the engine is not running

If curl fails to connect, tell the user to start the engine:
```bat
build\launch_dev.bat
```
The DebugServer only runs in Debug/Development builds (`_DEV` defined).

## User's request

$ARGUMENTS
