-- sh2_perf.lua - performance probe for the headset tuning pass (2026-09-28).
-- 1. Records every engine tick's frame time and writes a summary line every WINDOW_S seconds to
--    sh2_perf_log.txt (UEVR profile data folder): average fps, the slowest 1% as fps, the single
--    slowest frame, and how many frames missed the 72 fps budget.
-- 2. Reads sh2_perf_cmd.txt every second. When its first line (an id) changes, runs the rest:
--      label <text>          tag the following summaries
--      cmd <console command> run an engine console command (e.g. r.MotionBlurQuality 0)
--      mod <key> <value>     set a UEVR option (e.g. VR_RenderingMethod 1)
-- Probe only: archive it once the tuning pass is done.

local api = uevr.api

local WINDOW_S = 10.0
local BUDGET_MS = 1000.0 / 72.0
local CMD_EVERY_S = 1.0

local label = "start"
local samples = {}
local window_t = 0.0
local cmd_t = 0.0
local last_id = nil

local function log(line)
    local f = io.open("sh2_perf_log.txt", "a")
    if f then f:write(os.date("%H:%M:%S "), line, "\n"); f:close() end
end

local function summarise()
    local n = #samples
    if n == 0 then return end
    local sum = 0.0
    local over = 0
    for i = 1, n do
        sum = sum + samples[i]
        if samples[i] > BUDGET_MS then over = over + 1 end
    end
    table.sort(samples)
    local p99 = samples[math.max(1, math.floor(n * 0.99))]
    local worst = samples[n]
    log(string.format("[%s] n=%d avg=%.1ffps low1%%=%.1ffps worst=%.1fms over72=%d (%.1f%%)",
        label, n, 1000.0 * n / sum, 1000.0 / p99, worst, over, 100.0 * over / n))
    samples = {}
end

local function run_commands()
    local f = io.open("sh2_perf_cmd.txt", "r")
    if not f then return end
    local id = f:read("*l")
    if id == nil or id == last_id then f:close(); return end
    last_id = id
    for line in f:lines() do
        local verb, rest = line:match("^(%S+)%s+(.+)$")
        if verb == "label" then
            summarise()
            label = rest
            log("LABEL " .. rest)
        elseif verb == "cmd" then
            local ok, err = pcall(function() api:execute_command(rest) end)
            log((ok and "CMD " or "CMD FAILED ") .. rest .. (ok and "" or (" : " .. tostring(err))))
        elseif verb == "mod" then
            local key, val = rest:match("^(%S+)%s+(%S+)$")
            local ok, err = pcall(function() uevr.params.vr.set_mod_value(key, val) end)
            log((ok and "MOD " or "MOD FAILED ") .. rest .. (ok and "" or (" : " .. tostring(err))))
        end
    end
    f:close()
end

uevr.sdk.callbacks.on_pre_engine_tick(function(engine, delta)
    samples[#samples + 1] = delta * 1000.0
    window_t = window_t + delta
    cmd_t = cmd_t + delta
    if cmd_t >= CMD_EVERY_S then
        cmd_t = 0.0
        pcall(run_commands)
    end
    if window_t >= WINDOW_S then
        window_t = 0.0
        pcall(summarise)
    end
end)

log("perf probe loaded")
