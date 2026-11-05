# threads2.ps1: the busiest threads of the game, each named by the module its start address lies in.
param([string]$Proc = 'SHProto-Win64-Shipping', [int]$Seconds = 10, [int]$Top = 10)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class TS {
  [DllImport("kernel32.dll")] public static extern IntPtr OpenThread(int access, bool inherit, int id);
  [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
  [DllImport("ntdll.dll")] public static extern int NtQueryInformationThread(IntPtr h, int cls, out IntPtr info, int len, IntPtr ret);
  public static long Start(int id) {
    IntPtr h = OpenThread(0x0040, false, id);   // THREAD_QUERY_INFORMATION
    if (h == IntPtr.Zero) return 0;
    IntPtr a; int r = NtQueryInformationThread(h, 9, out a, IntPtr.Size, IntPtr.Zero);   // ThreadQuerySetWin32StartAddress
    CloseHandle(h);
    return r == 0 ? a.ToInt64() : 0;
  }
}
'@
$p = Get-Process $Proc -ErrorAction Stop
$mods = $p.Modules | ForEach-Object { [pscustomobject]@{ Name = $_.ModuleName; Base = $_.BaseAddress.ToInt64(); End = $_.BaseAddress.ToInt64() + $_.ModuleMemorySize } }
function ModOf([long]$a) { foreach ($m in $mods) { if ($a -ge $m.Base -and $a -lt $m.End) { return ('{0}+0x{1:X}' -f $m.Name, ($a - $m.Base)) } }; return ('0x{0:X}' -f $a) }
$st = Join-Path $env:LOCALAPPDATA 'OpenXR-Simulator/runtime_status.json'
$f0 = (Get-Content $st -Raw | ConvertFrom-Json).frame_count
$t0 = @{}; foreach ($t in $p.Threads) { try { $t0[$t.Id] = $t.TotalProcessorTime.TotalMilliseconds } catch {} }
Start-Sleep -Seconds $Seconds
$f1 = (Get-Content $st -Raw | ConvertFrom-Json).frame_count
$frames = [math]::Max(1, $f1 - $f0)
$p = Get-Process $Proc
$rows = foreach ($t in $p.Threads) {
  try { $now = $t.TotalProcessorTime.TotalMilliseconds } catch { continue }
  if (-not $t0.ContainsKey($t.Id)) { continue }
  [pscustomobject]@{ Busy = [math]::Round(100 * ($now - $t0[$t.Id]) / ($Seconds * 1000), 1); MsPerFrame = [math]::Round(($now - $t0[$t.Id]) / $frames, 2); Id = $t.Id; Start = (ModOf ([TS]::Start($t.Id))) }
}
"frames {0} in {1} s = {2:N1} fps" -f $frames, $Seconds, ($frames / $Seconds)
$rows | Sort-Object Busy -Descending | Select-Object -First $Top | Format-Table -AutoSize | Out-String -Width 140
