# sh2_sim_launch.ps1 -- start Silent Hill 2 for a headset-less run (2026-10-31): the OpenXR Simulator plays the
# headset, the Steam identity is set so Continue finds the save, a window is asked for. Then, from the session:
#   1. inject UEVR through the UEVR MCP (uevr_setup_game, the UEVR AFW build's UEVRBackend.dll, launchIfMissing false)
#   2. sh2_front.ps1, the pad A twice (warning screen, then CONTINUE on the main menu)
#   3. after the load: sim_ctl.py one trigger pull (the game reads the pad only after a VR controller input)
#   ⚠️ NEVER press A after the load: the save spot stands at the save point, A opens the save menu.
param([string]$Sim = 'D:\claude video game stuff\tools\OpenXR-Simulator\openxr_simulator.json',
      [string]$Exe = 'C:\Steam\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe')
if (Get-Process SHProto-Win64-Shipping -ErrorAction SilentlyContinue) { Write-Output 'already running'; exit 1 }
$env:XR_RUNTIME_JSON = $Sim; $env:SteamAppId = '2124490'; $env:SteamGameId = '2124490'
Start-Process -FilePath $Exe -ArgumentList '-windowed', '-ResX=1280', '-ResY=720' -WorkingDirectory (Split-Path $Exe)
for ($i = 0; $i -lt 60; $i++) { Start-Sleep -Milliseconds 500; $p = Get-Process SHProto-Win64-Shipping -ErrorAction SilentlyContinue; if ($p -and $p.MainWindowHandle -ne 0) { break } }
Start-Sleep -Seconds 10
Write-Output ("started, pid {0}" -f (Get-Process SHProto-Win64-Shipping).Id)
