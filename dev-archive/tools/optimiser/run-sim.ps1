# run-sim.ps1 <label> [-SyncMode n] [-Mcp on|off|keep]: one measured run on the OpenXR Simulator, by keys, no MCP needed.
#   Closes a running game, sets UEVR's VR_SynchronizationMode (0 Early / 1 Late / 2 Very Late) in the profile's
#   config.txt, parks or restores uevr_mcp.dll in the profile's plugins folder, starts the game through our launcher
#   (play --sim --no-install), reads the plugin's heartbeat at the main menu, drives Continue by Enter keys, reads the
#   heartbeat in gameplay and the graphics card's load, prints one summary line, and LEAVES THE GAME RUNNING.
# Made 2026-11-05 (the optimiser, step 2). Steam must be running. The Continue save must be the prison armory spot.
param(
    [Parameter(Mandatory = $true)][string]$Label,
    [int]$SyncMode = -1,
    [ValidateSet('on', 'off', 'keep')][string]$Mcp = 'keep',
    [string]$Launcher = 'D:\SH2 builds\releases\Silent Hill 2 VR 0.1.1\SH2VR.exe',
    [string]$Park = 'D:\SH2 builds\_parked-plugins'
)
$prof = "$env:APPDATA\UnrealVRMod\SHProto-Win64-Shipping"
$log = "$prof\log.txt"
Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public static class RS { [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l); }'

function Close-Game {
    $p = Get-Process SHProto-Win64-Shipping -ErrorAction SilentlyContinue
    if (-not $p) { return }
    [void][RS]::PostMessage($p.MainWindowHandle, 0x0010, [IntPtr]0, [IntPtr]0)   # WM_CLOSE: the game's own quit path
    for ($i = 0; $i -lt 30; $i++) { Start-Sleep 1; if (-not (Get-Process SHProto-Win64-Shipping -ErrorAction SilentlyContinue)) { break } }
    Start-Sleep 3
}
function Heartbeats([string]$since, [string]$pawn, [int]$n) {
    Get-Content $log | Where-Object { $_ -match 'sh2vr\] alive' -and $_ -match "pawn $pawn" -and $_.Substring(1, 23) -gt $since } |
        Select-Object -Last $n | ForEach-Object { [double]([regex]::Match($_, '([0-9.]+) ms avg').Groups[1].Value) }
}
function Enter($h) { [void][RS]::PostMessage($h, 0x100, [IntPtr]13, [IntPtr]0x001C0001); Start-Sleep -Milliseconds 120; [void][RS]::PostMessage($h, 0x101, [IntPtr]13, [IntPtr]([int64]0xC01C0001)) }

Close-Game
if ($SyncMode -ge 0) {
    $cfg = "$prof\config.txt"
    (Get-Content $cfg) -replace '^VR_SynchronizationMode=\d$', "VR_SynchronizationMode=$SyncMode" | Set-Content $cfg -Encoding ascii
}
if ($Mcp -eq 'off' -and (Test-Path "$prof\plugins\uevr_mcp.dll")) { New-Item -ItemType Directory -Force $Park | Out-Null; Move-Item "$prof\plugins\uevr_mcp.dll" "$Park\uevr_mcp.dll" -Force }
if ($Mcp -eq 'on' -and (Test-Path "$Park\uevr_mcp.dll")) { Move-Item "$Park\uevr_mcp.dll" "$prof\plugins\uevr_mcp.dll" -Force }
$mcpState = if (Test-Path "$prof\plugins\uevr_mcp.dll") { 'mcp on' } else { 'mcp off' }
$sync = (Select-String -Path "$prof\config.txt" -Pattern 'VR_SynchronizationMode=(\d)').Matches[0].Groups[1].Value
$since = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss.fff')

Set-Location (Split-Path $Launcher)
Start-Process -FilePath $Launcher -ArgumentList 'play', '--sim', '--no-install' -WindowStyle Hidden
Start-Sleep 60                                            # window, UEVR in, warning screen, the menu settles
$menu = Heartbeats $since 'none' 3
$h = (Get-Process SHProto-Win64-Shipping).MainWindowHandle
Enter $h; Start-Sleep 6                                   # the warning
Enter $h; Start-Sleep 95                                  # CONTINUE, the load
Enter $h; Start-Sleep 12                                  # PRESS ANY BUTTON TO PLAY
Enter $h; Start-Sleep 25                                  # once more: 2026-11-10 the first one came too early and every reading was taken on that screen; then settle
$game = Heartbeats $since 'yes' 3
$gpu = (& nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits -l 1 2>$null | Select-Object -First 5 | Measure-Object -Average).Average
Stop-Process -Name nvidia-smi -Force -ErrorAction SilentlyContinue
$m = if ($menu) { '{0:N1}' -f ($menu | Measure-Object -Average).Average } else { '?' }
$g = if ($game) { '{0:N1}' -f ($game | Measure-Object -Average).Average } else { '?' }
$vr = if ((Get-Content $log | Where-Object { $_ -match 'sh2vr\] alive' } | Select-Object -Last 1) -match 'headset on') { 'VR ON' } else { 'VR OFF: THE READING IS THE FLAT GAME' }
"RUN $Label | sync $sync | $mcpState | $vr | menu $m ms | game $g ms ($($game -join '/')) | gpu $([int]$gpu) %"
