"""sh2cut.py <label> [watch seconds] : one run of the opening cutscene on the simulator, then the flicker incidents by reason.
Starts the game through the mod's launcher, New Game by keys, watches the cutscene, closes the game the clean way."""
import ctypes, ctypes.wintypes as wt, glob, json, os, re, subprocess, sys, time

S = os.path.dirname(os.path.abspath(__file__))
label = sys.argv[1]
watch = float(sys.argv[2]) if len(sys.argv) > 2 else 100
LAUNCHER = r"D:\SH2 builds\releases\Silent Hill 2 VR 0.1.1\SH2VR.exe"
PROF = os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping")
LOG = os.path.join(PROF, "log.txt")
INC = os.path.expandvars(r"%LOCALAPPDATA%\OpenXR-Simulator\flicker_incidents")
u = ctypes.windll.user32

def running(name=b"SHProto"):
    return name in subprocess.check_output("tasklist")

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

def loglines(word, since):
    return [l for l in open(LOG, errors="replace").read().splitlines() if word in l and l[1:24] > since]

if running():
    print("a game is already running: stop"); sys.exit(1)
before = set(os.listdir(INC)) if os.path.isdir(INC) else set()
since = time.strftime("%Y-%m-%d %H:%M:%S")
subprocess.Popen([LAUNCHER, "play", "--sim", "--no-install"], cwd=os.path.dirname(LAUNCHER))
time.sleep(62); key(13); time.sleep(8)
key(0x28); time.sleep(1); key(13); time.sleep(4)
for _ in range(4): key(0x28); time.sleep(0.5)
key(13); time.sleep(32); key(13)
t0 = time.time(); began = False
while time.time() - t0 < 40:
    if loglines("cutscene began", since): began = True; break
    time.sleep(0.5)
tb = time.time()
while began and time.time() - tb < watch and running(): time.sleep(1)
alive = running()
new = sorted(set(os.listdir(INC)) - before)
reasons = {}
frames = []
for s in new:
    for d in sorted(os.listdir(os.path.join(INC, s))):
        try:
            r = json.load(open(os.path.join(INC, s, d, "incident.json"))).get("reason", "?")
        except Exception:
            r = "?"
        reasons[r] = reasons.get(r, 0) + 1
        if r.startswith("ASYM"): frames.append(d.split("_")[1])
warn = len(loglines("vtable is already hooked", since))
h = win()
if h: u.PostMessageW(h, 0x10, 0, 0)
for _ in range(40):
    if not running(): break
    time.sleep(1)
if running(b"CrashReportClient"): subprocess.run("taskkill /IM CrashReportClient.exe /F", capture_output=True)
time.sleep(3)
print(f"RUN {label} | cutscene began {began} | game alive at the end {alive} | incidents {reasons} | asymmetric at {frames} | hooked-warnings {warn} | session {new}", flush=True)
