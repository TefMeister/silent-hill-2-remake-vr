# /gr: which runs first on the pad, Lua or C++? C++, and Lua always last

From: `/gr`, 2026-10-26 (estate sweep, FULL pass on this project).

**The open question (board, `[PD]` "THE BUTTON LAYER"):** *"Which runs first, Lua or C++, is not known
`[hypothesis either way]`: measure it before building."*

**Answer** `[inferred-static 2026-10-26]`, from UEVR's public source (praydog master and PureDark's AFW fork, same
lines): one list of pad callbacks, called front to back each poll; C++ plugins register at start-up, sorted by file
name (`sh2r` → `sh2vr` → `uevr_mcp` → `uevr_utils`); the Lua side registers on UEVR's first frame and re-registers at
the END after every "Reset scripts". So `sh2vr.dll` sees the real controller first and `main.lua`'s writes (stick
down → B, stick press → LB) land after ours, every poll. Consistent with dossier §3b's live finding that the MCP's
simulated pad arrives after the plugin's callback (`uevr_mcp` sorts after `sh2vr`). Full write-up:
[`external-research/topics/2026-10-26-uevr-runs-cpp-plugins-before-lua-on-the-pad.md`](../../external-research/topics/2026-10-26-uevr-runs-cpp-plugins-before-lua-on-the-pad.md).

**Suggested dossier change:** a line in §4 (or a new "UEVR plugin facts" note): *pad callbacks run C++ plugins first
(file-name order), Lua last, re-appended on every script reset; the button layer must switch the Lua pack's pad writes
off rather than try to undo them from C++.* Still worth the cheap live confirmation in the topic (one log line per
side per poll) before building on it.
