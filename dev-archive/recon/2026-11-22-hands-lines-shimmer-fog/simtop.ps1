param([int]$top=1,[int]$w=3440,[int]$h=1440)
Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class ST{ public delegate bool P(IntPtr h,IntPtr l);
[DllImport("user32.dll")]public static extern bool EnumWindows(P p,IntPtr l);
[DllImport("user32.dll")]public static extern int GetWindowText(IntPtr h,StringBuilder s,int n);
[DllImport("user32.dll")]public static extern bool IsWindowVisible(IntPtr h);
[DllImport("user32.dll")]public static extern bool GetClientRect(IntPtr h,out R r);
[DllImport("user32.dll")]public static extern bool ClientToScreen(IntPtr h,ref PT p);
[DllImport("user32.dll")]public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")]public static extern bool SetWindowPos(IntPtr h,IntPtr a,int x,int y,int cx,int cy,uint f);
public struct R{public int L,T,Rr,B;} public struct PT{public int X,Y;} }
"@
[ST]::SetProcessDPIAware()|Out-Null
$found=@()
[ST]::EnumWindows({param($h,$l) $s=New-Object Text.StringBuilder 200; [ST]::GetWindowText($h,$s,200)|Out-Null; if([ST]::IsWindowVisible($h) -and $s.ToString() -match '^OpenXR Simulator'){ $script:found+=,$h }; $true},[IntPtr]::Zero)|Out-Null
foreach($f in $found){ $ins=[IntPtr]::new($(if($top -eq 1){-1}else{-2})); [ST]::SetWindowPos($f,$ins,0,0,$w,$h,0x10)|Out-Null
 $r=New-Object ST+R; [ST]::GetClientRect($f,[ref]$r)|Out-Null; $p=New-Object ST+PT; [ST]::ClientToScreen($f,[ref]$p)|Out-Null; "client $($p.X) $($p.Y) $($r.Rr) $($r.B)" }
