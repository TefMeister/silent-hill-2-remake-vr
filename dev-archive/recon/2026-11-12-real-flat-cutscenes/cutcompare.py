"""cutcompare.py <label> plain|vr [seconds] [--keep-open]
One run of the opening cutscene, a still of the game window every second from the last key press.
plain = the game alone (no injector); vr = the mod's launcher on the OpenXR Simulator (installed build, nothing installed).
The save folder is copied away first and put back after (checked by hash)."""
import ctypes, ctypes.wintypes as wt, hashlib, os, shutil, subprocess, sys, time
from PIL import Image

S = os.path.dirname(os.path.abspath(__file__))
label, mode = sys.argv[1], sys.argv[2]
secs = int(sys.argv[3]) if len(sys.argv) > 3 and sys.argv[3].isdigit() else 250
keep = "--keep-open" in sys.argv
TEST_DLL = sys.argv[sys.argv.index("--dll") + 1] if "--dll" in sys.argv else None   # a test build: in for this run only, the installed one back after
PLUGIN = os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\plugins\sh2vr.dll")
def fhash(p): return hashlib.sha256(open(p, "rb").read()).hexdigest()[:16]
OUT = os.path.join(S, "runs", label); os.makedirs(OUT, exist_ok=True)
LAUNCHER = r"D:\SH2 builds\releases\Silent Hill 2 VR 0.1.1\SH2VR.exe"
EXE = r"C:\Steam\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe"
SAVES = os.path.expandvars(r"%LOCALAPPDATA%\SilentHill2\Saved\SaveGames")
KEEP = r"D:\SH2 builds\_save-backups\2026-11-12c-real-flat-" + label
LOG = os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\log.txt")
u = ctypes.windll.user32; g = ctypes.windll.gdi32; u.SetProcessDPIAware()

def running(name=b"SHProto"): return name in subprocess.check_output("tasklist")
def tree_hash(root):
    h = hashlib.sha256()
    for d, _, fs in sorted(os.walk(root)):
        for f in sorted(fs):
            p = os.path.join(d, f); h.update(os.path.relpath(p, root).encode()); h.update(open(p, "rb").read())
    return h.hexdigest()[:16]
def win():
    out = []
    P = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        n = u.GetWindowTextLengthW(h)
        if u.IsWindowVisible(h) and n:
            b = ctypes.create_unicode_buffer(n + 1); u.GetWindowTextW(h, b, n + 1)
            if b.value.startswith("Silent Hill 2"): out.append(h)
        return True
    cbp = P(cb); u.EnumWindows(cbp, 0)
    return out[0] if out else None
def key(vk):
    h = win()
    if not h: return
    sc = u.MapVirtualKeyW(vk, 0)
    u.PostMessageW(h, 0x100, vk, 1 | (sc << 16)); time.sleep(0.12); u.PostMessageW(h, 0x101, vk, 0xC0000001 | (sc << 16))
class BI(ctypes.Structure): _fields_=[("s",wt.DWORD),("w",ctypes.c_long),("h",ctypes.c_long),("p",wt.WORD),("b",wt.WORD),("c",wt.DWORD),("si",wt.DWORD),("x",ctypes.c_long),("y",ctypes.c_long),("u",wt.DWORD),("i",wt.DWORD)]
def shot(name, size=640):
    h = win()
    if not h: return None
    r = wt.RECT(); u.GetClientRect(h, ctypes.byref(r)); w, hh = r.right, r.bottom
    if w < 10 or hh < 10: return None
    dc = u.GetDC(h); m = g.CreateCompatibleDC(dc); bm = g.CreateCompatibleBitmap(dc, w, hh); g.SelectObject(m, bm); u.PrintWindow(h, m, 3)
    bi = BI(40, w, -hh, 1, 32, 0, 0, 0, 0, 0, 0); buf = ctypes.create_string_buffer(w * hh * 4); g.GetDIBits(m, bm, 0, hh, buf, ctypes.byref(bi), 0)
    g.DeleteObject(bm); g.DeleteDC(m); u.ReleaseDC(h, dc)
    im = Image.frombuffer("RGBA", (w, hh), buf, "raw", "BGRA", 0, 1).convert("RGB"); im.thumbnail((size, size)); im.save(os.path.join(OUT, name), quality=85)
    return im
def loglines(word, since):
    try: return [l for l in open(LOG, errors="replace").read().splitlines() if word in l and l[1:20] >= since]
    except OSError: return []

if running(): print("a game is already running: stop"); sys.exit(1)
if os.path.isdir(KEEP): shutil.rmtree(KEEP)
shutil.copytree(SAVES, KEEP); h0 = tree_hash(SAVES)
since = time.strftime("%Y-%m-%d %H:%M:%S")
if TEST_DLL:
    installed = fhash(PLUGIN); shutil.copy2(PLUGIN, PLUGIN + ".installed-kept"); shutil.copy2(TEST_DLL, PLUGIN)
    print("test build in:", fhash(PLUGIN), "(installed build kept:", installed + ")", flush=True)
if mode == "vr":
    subprocess.Popen([LAUNCHER, "play", "--sim", "--no-install"], cwd=os.path.dirname(LAUNCHER))
else:
    env = dict(os.environ, SteamAppId="2124490", SteamGameId="2124490"); env.pop("XR_RUNTIME_JSON", None)
    subprocess.Popen([EXE, "-windowed"], cwd=os.path.dirname(EXE), env=env)
# A picture check before each key (2026-11-12: a lost first Enter left the warning up and every later key landed one
# step off: Credits instead of New Game, twice; Tefa saw it).
def region(im, x0, y0, x1, y1):
    w, h = im.size; px = list(im.crop((int(x0 * w), int(y0 * h), int(x1 * w), int(y1 * h))).getdata()); n = max(1, len(px))
    return tuple(sum(p[i] for p in px) / n for i in range(3))
def is_warning(im): return im is not None and sum(region(im, 0.44, 0.62, 0.56, 0.82)) / 3 > 50      # the white code square
def red(im, x0, y0, x1, y1):   # the share of clearly red pixels in a part of the picture
    w, h = im.size; px = list(im.crop((int(x0 * w), int(y0 * h), int(x1 * w), int(y1 * h))).getdata())
    return sum(1 for p in px if p[0] > 45 and p[0] > 1.5 * p[1]) / max(1, len(px))
# the red bar on CONTINUE / on START GAME; with the injector the window draws the menu in its upper two thirds
def is_menu(im): return im is not None and max(red(im, 0.02, 0.24, 0.12, 0.27), red(im, 0.03, 0.495, 0.15, 0.53)) > 0.3
def is_start(im): return im is not None and max(red(im, 0.35, 0.73, 0.65, 0.78), red(im, 0.35, 0.87, 0.65, 0.90)) > 0.3
def bail(why):
    print("ROUTE STOPPED:", why, flush=True)
    h = win()
    if h: u.PostMessageW(h, 0x10, 0, 0)
    for _ in range(45):
        if not running(): break
        time.sleep(1)
    time.sleep(4)
    if not running():
        shutil.rmtree(SAVES); shutil.copytree(KEEP, SAVES)
        if TEST_DLL: shutil.copy2(PLUGIN + ".installed-kept", PLUGIN); os.remove(PLUGIN + ".installed-kept")
        print("closed; saves and the installed build put back")
    sys.exit(3)
time.sleep(50)
for _ in range(30):
    if is_warning(shot("r0-before-warning.jpg")): break
    time.sleep(2)
else: bail("the warning screen never showed")
time.sleep(4)
for _ in range(8):
    key(13); time.sleep(5)
    if is_menu(shot("r1-main-menu.jpg")): break
else: bail("the main menu never showed")
time.sleep(1); key(0x28); time.sleep(1); key(13); time.sleep(4)
for _ in range(4): key(0x28); time.sleep(0.5)
time.sleep(0.5)
if not is_start(shot("r2-new-game-settings.jpg")): bail("START GAME is not the marked line (see r2)")
key(13); time.sleep(32); shot("r3-press-any.jpg"); key(13)
t0 = time.time(); began = None; n = 0
while time.time() - t0 < secs and running():
    t = time.time() - t0
    shot(f"t{int(round(t)):03d}.jpg")
    if mode == "vr" and began is None and loglines("cutscene began", since): began = round(t, 1)
    n += 1
    time.sleep(max(0.0, t0 + n - time.time()))
print(f"RUN {label} {mode}: stills {n}, game up {running()}, cutscene-began line at +{began} s", flush=True)
for w in ("cutscreen:", "realcam:"): print("\n".join(l[:200] for l in loglines(w, since)[-40:]))
if keep: print("left open; saves NOT yet put back, backup in", os.path.basename(KEEP)); sys.exit(0)
h = win()
if h: u.PostMessageW(h, 0x10, 0, 0)
for _ in range(45):
    if not running(): break
    time.sleep(1)
if running(b"CrashReportClient"): subprocess.run("taskkill /IM CrashReportClient.exe /F", capture_output=True)
time.sleep(4)
if running(): print("GAME STILL UP: saves not put back"); sys.exit(2)
shutil.rmtree(SAVES); shutil.copytree(KEEP, SAVES)
if TEST_DLL:
    shutil.copy2(PLUGIN + ".installed-kept", PLUGIN); os.remove(PLUGIN + ".installed-kept")
    print("installed build back:", fhash(PLUGIN) == installed, fhash(PLUGIN))
print("saves put back, same as before:", tree_hash(SAVES) == h0)
