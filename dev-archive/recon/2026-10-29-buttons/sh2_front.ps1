# sh2_front.ps1 -- bring the REAL Silent Hill 2 window to the front and click into it (2026-10-29).
# The process's "main window" handle is the OpenXR Simulator's own window when the simulator runs, so this picks the
# largest visible window of the game process (the UnrealWindow, 1920x1080). The Alt key tap lets a background process
# take the foreground (Windows refuses SetForegroundWindow otherwise). The game only takes the pad while it is in front.
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Collections.Generic; using System.Text;
public class SHF {
 public delegate bool EnumProc(IntPtr h, IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint x, uint y, uint d, UIntPtr e);
 [DllImport("user32.dll")] public static extern void keybd_event(byte k, byte s, uint f, UIntPtr e);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [StructLayout(LayoutKind.Sequential)] public struct R { public int L,T,Rt,B; }
 public static List<IntPtr> Find(uint pid) { var list = new List<IntPtr>(); EnumWindows((h,l)=>{ uint p; GetWindowThreadProcessId(h, out p); if (p==pid && IsWindowVisible(h)) list.Add(h); return true; }, IntPtr.Zero); return list; }
}
"@
$p = Get-Process SHProto-Win64-Shipping -ErrorAction Stop
$best = [IntPtr]::Zero; $ba = 0
foreach ($h in [SHF]::Find($p.Id)) { $r = New-Object SHF+R; [SHF]::GetWindowRect($h, [ref]$r) | Out-Null; $a = ($r.Rt-$r.L)*($r.B-$r.T); if ($a -gt $ba) { $ba = $a; $best = $h } }
$r = New-Object SHF+R; [SHF]::GetWindowRect($best, [ref]$r) | Out-Null
[SHF]::keybd_event(0x12,0,0,[UIntPtr]::Zero); [SHF]::SetForegroundWindow($best) | Out-Null; [SHF]::keybd_event(0x12,0,2,[UIntPtr]::Zero)
$cx = [int](($r.L+$r.Rt)/2); $cy = [int](($r.T+$r.B)/2)
[SHF]::SetCursorPos($cx, $cy) | Out-Null; [SHF]::mouse_event(2,0,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds 80; [SHF]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
Start-Sleep -Milliseconds 300
"game window $($r.Rt-$r.L)x$($r.B-$r.T) at $($r.L),$($r.T); in front = " + ([SHF]::GetForegroundWindow() -eq $best)
