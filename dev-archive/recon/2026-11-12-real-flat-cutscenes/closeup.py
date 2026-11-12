"""closeup.py <save backup folder name> : close a run left open by cutcompare --keep-open, the game's own way, then put
the installed plugin (if a test build was put in) and the saves back, and say so."""
import ctypes, ctypes.wintypes as wt, hashlib, os, shutil, subprocess, sys, time
u = ctypes.windll.user32
PLUGIN = os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\plugins\sh2vr.dll")
SAVES = os.path.expandvars(r"%LOCALAPPDATA%\SilentHill2\Saved\SaveGames")
KEEP = r"D:\SH2 builds\_save-backups" + "\\" + sys.argv[1]
def running(n=b"SHProto"): return n in subprocess.check_output("tasklist")
out = []
P = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
def cb(h, _):
    n = u.GetWindowTextLengthW(h)
    if u.IsWindowVisible(h) and n:
        b = ctypes.create_unicode_buffer(n + 1); u.GetWindowTextW(h, b, n + 1)
        if b.value.startswith("Silent Hill 2"): out.append(h)
    return True
f = P(cb); u.EnumWindows(f, 0)
if out: u.PostMessageW(out[0], 0x10, 0, 0)
for _ in range(45):
    if not running(): break
    time.sleep(1)
if running(b"CrashReportClient"): subprocess.run("taskkill /IM CrashReportClient.exe /F", capture_output=True)
time.sleep(4)
if running(): print("GAME STILL UP: nothing put back"); sys.exit(2)
if os.path.exists(PLUGIN + ".installed-kept"):
    shutil.copy2(PLUGIN + ".installed-kept", PLUGIN); os.remove(PLUGIN + ".installed-kept")
print("installed plugin:", hashlib.sha256(open(PLUGIN, "rb").read()).hexdigest()[:16])
shutil.rmtree(SAVES); shutil.copytree(KEEP, SAVES); print("saves put back from", sys.argv[1])
