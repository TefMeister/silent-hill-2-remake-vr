# sh2_key.ps1 <vk> [<vk> ...] -- post a key press (WM_KEYDOWN, then WM_KEYUP) to every visible window of the game
# (2026-10-31). The plugin's desk-test keys (F5 left grip, F6 right grip, F7 trigger ...) read WM_KEYDOWN in
# on_message, so the game does not have to be in front. Numbers: F3 0x72, F5 0x74, F6 0x75, F7 0x76.
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Keys)
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Collections.Generic;
public class SHK {
 public delegate bool EnumProc(IntPtr h, IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 public static List<IntPtr> Find(uint pid) { var list = new List<IntPtr>(); EnumWindows((h,l)=>{ uint p; GetWindowThreadProcessId(h, out p); if (p==pid && IsWindowVisible(h)) list.Add(h); return true; }, IntPtr.Zero); return list; }
}
"@
$p = Get-Process SHProto-Win64-Shipping -ErrorAction Stop
$wins = [SHK]::Find($p.Id)
foreach ($k in $Keys) {
    $vk = [Convert]::ToInt32($k, 16)
    foreach ($h in $wins) {
        [SHK]::PostMessage($h, 0x100, [IntPtr]$vk, [IntPtr]1) | Out-Null
        [SHK]::PostMessage($h, 0x101, [IntPtr]$vk, [IntPtr]0xC0000001) | Out-Null
    }
    Write-Output ("posted 0x{0:X2} to {1} window(s)" -f $vk, $wins.Count)
}
