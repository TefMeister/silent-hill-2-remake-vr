# Reader: uevr-mcp's per-frame file opens (the breadcrumb writer)

Background reader, 2026-11-05, from source only (github.com/elliotttate/uevr-mcp, master 8cf1053 of 2026-06-14, and
tag v1.0.0), plus the installed copy's version files and one read of the breadcrumb file it leaves behind. Nothing
running was touched. Follows `2026-11-05-reader-simulator-and-uevr-frame-waits.md`; the live sampling that found
uevr_mcp.dll in CreateFileW/CreateDirectoryW is the session's own measurement, not mine.

## 1. The code path: a "crash breadcrumb" JSON file rewritten at every callback boundary

`plugin/src/diagnostics.cpp` keeps a one-record JSON ("which callback is active right now") and writes it to disk so
a crash can be read back afterwards. Path: `api->get_persistent_dir(L"uevr_mcp_last_breadcrumb.json")` (line 56),
i.e. UEVR's per-game folder under the roaming app-data `UnrealVRMod\<game>\`. The MCP server's only reader is the
HTTP route (`Tools.cs:86-89` -> `/api/diagnostics/breadcrumb` -> the in-memory `get_breadcrumb()`); nothing in
`mcp-server/` ever opens the file from disk `[inferred-static, grep of mcp-server]`.

The writer, `write_breadcrumb_locked` (lines 237-262), runs with `m_mutex` held and does, every time it is called:
```
std::filesystem::create_directories(m_breadcrumb_path.parent_path(), ec);          // CreateDirectoryW
std::ofstream out(m_breadcrumb_path, std::ios::binary | std::ios::trunc);          // CreateFileW, truncate
out << m_breadcrumb_state.dump(2); out.flush();                                     // write + close
```
It is called from `callback_enter` (line 111) and `callback_success` (line 150), which the RAII helper
`Diagnostics::ScopedCallback` fires on entry and exit of a scope. Per frame those scopes are `[inferred-static]`:
- game thread, inside `on_pre_engine_tick` (main.cpp 98-121): `ScopedCallback on_frame_diag("lua_on_frame")`
  (lua_engine.cpp:861) and `ScopedCallback tick_diag("property_watch_tick")` (property_watch.cpp:327) = 4 writes;
  more per Lua callback/timer/coroutine and per watched property when any exist (lua_engine.cpp 869/902/962,
  property_watch.cpp 356);
- present (RHI) thread, `on_present` (main.cpp:125): `ScopedCallback diag("on_present", renderer)` = 2 writes.
So in the installed build each frame opens the same file about six times, four on the game thread and two on the
present thread, each open preceded by a directory create. That is the CreateFileW/CreateDirectoryW the sampling saw.
The lock: both threads take `std::lock_guard<std::mutex> lock(m_mutex)` (diagnostics.cpp 85, 118, 155) and the file
I/O happens inside it, so the present thread sits on the game thread's write and vice versa; MSVC's std::mutex is an
SRW lock, which is the 29 % SRW-lock share on the RHI thread `[inferred-static]`.
Live trace of it on this PC `[measured 2026-11-05, n=1]`: the file in the SH2 UEVR folder was last written 19:09:17
(the minute the dll came out of the profile), `"sequence": 32150`, `"callback": "property_watch_tick"`,
`"status": "idle"`, `"threadId": 18612`: the game-thread writer, as predicted.

Not the cause: `PipeServer::log` only appends to an in-memory deque (pipe_server.cpp 70-92); `ScreenshotCapture::
on_present` returns at once unless a capture was requested (screenshot_capture.cpp 687-689); the HTTP server and
the named pipe (`\\.\pipe\UEVR_MCP`) live on their own threads; no other per-frame file or directory call exists in
`plugin/src` (grep for CreateFile/ofstream/filesystem/fopen) `[inferred-static]`.

## 2. Installed 1.0.0 versus master: the fix is upstream, unreleased

Installed: `VERSION` = 1.0.0, `lanes-release.txt` = v1.0.0; the only GitHub release is `UevrMcp-v1.0.0.zip`,
built 2026-05-15 `[verified-live 2026-11-05]`. Master has one commit after it that matters here:
`8cf1053 2026-06-14 "Throttle MCP breadcrumb diagnostics"` (no body). Its whole change to the writer:
```
-    const bool state_changed = callback != m_last_breadcrumb_callback || status != m_last_breadcrumb_status;
-    if (!force && !state_changed && now - m_last_breadcrumb_flush < BREADCRUMB_FLUSH_INTERVAL) {
+    if (!force && now - m_last_breadcrumb_flush < BREADCRUMB_FLUSH_INTERVAL) {
```
`BREADCRUMB_FLUSH_INTERVAL` is 750 ms in both. In 1.0.0 the throttle never engages, because every enter/exit flips
the callback name or the status ("active" <-> "idle"), so `state_changed` is true on every call and the file is
rewritten each time. On master it is at most one write per 750 ms, plus a forced write on `initialize()` and on
`callback_failure` (which is the case the file exists for). The rest of master since v1.0.0 (render_routes.cpp,
game_patches, the RenderDoc / stereo-forensics server tools, 25 files) adds tools, nothing per frame.
No switch exists in either version: no env var, config file, "idle" mode or route disables the breadcrumb
(grep for getenv/GetEnvironmentVariable/config in diagnostics.cpp, main.cpp, http_server.cpp, pipe_server.cpp:
nothing) `[inferred-static]`.

## 3. Smallest change

1. **Zero code, already done by the session:** keep `uevr_mcp.dll` out of the UEVR plugins folder except in a session
   that needs the MCP. `[measured 2026-11-05, n=1: 19 -> 12-13 ms]`
2. **One line, already upstream:** build `plugin/` from master (or apply the two-line diff above to v1.0.0's
   diagnostics.cpp) and drop the new `uevr_mcp.dll` in. Writes fall from ~6 per frame to ~1.3 per second. The server
   (`UevrMcpServer.exe`) needs no change: it never reads the file and the HTTP contract is untouched. `[inferred-static]`
3. **If keeping it on permanently (my suggestion, untested):** also move the disk write out from under `m_mutex`
   (copy the JSON string under the lock, write after unlocking) or onto a worker thread, and call
   `create_directories` once in `initialize()` rather than per write; then even the 750 ms write cannot stall the
   present thread. A `ReadDirectoryChangesW`/timer design is not needed: nobody polls this file, it is only written. `[hypothesis]`
Worth telling the author: the 1.0.0 zip ships the unthrottled writer; a 1.0.1 release of the June commit would fix
every user. Star + follow per the thanks rule (elliotttate taught us this one).
