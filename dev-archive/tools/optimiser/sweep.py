# One setting at a time on the running game: frame time from the plugin's heartbeat, and how much the picture changed
# (the headset lies still, so two looks at the game window can be compared). Everything is put back after its test.
import sys, os, re, time, json, subprocess, urllib.request, urllib.parse
import numpy as np
from PIL import Image
LOG = os.path.expandvars(r"%APPDATA%/UnrealVRMod/SHProto-Win64-Shipping/log.txt")
MOM = os.path.expanduser("~/github-backups/_lanes-next-wt/plugins/lanes/tools/menu-o-matic/menu_o_matic.py")
OUT = sys.argv[1]; CANDS = json.load(open(sys.argv[2]))
API = "http://localhost:8899/api/console/"
def get(name):
    try:
        with urllib.request.urlopen(API + "cvar?name=" + urllib.parse.quote(name), timeout=5) as r:
            return json.load(r)["float"]
    except Exception:
        return None
def cmd(c):
    req = urllib.request.Request(API + "command", data=json.dumps({"command": c}).encode(), headers={"Content-Type": "application/json"})
    urllib.request.urlopen(req, timeout=5).read()
def ms(wait=6.0, take=10.5):
    time.sleep(wait); pos = os.path.getsize(LOG); time.sleep(take)
    with open(LOG, "r", errors="replace") as f:
        f.seek(pos); t = f.read()
    v = [float(x) for x in re.findall(r"alive: frame \d+, ([\d.]+) ms avg", t)]
    return sum(v) / len(v) if v else None
def look(tag):
    p = os.path.join(OUT, tag + ".png")
    subprocess.run([sys.executable, MOM, "look", "Silent Hill 2", p, "--scale", "0.5"], capture_output=True)
    return np.asarray(Image.open(p).convert("RGB"), dtype=np.float32)
def diff(a, b): return float(np.abs(a - b).mean())
os.makedirs(OUT, exist_ok=True)
res = open(os.path.join(OUT, "results.tsv"), "a")
def say(*a):
    line = "\t".join(str(x) for x in a); print(line, flush=True); res.write(line + "\n"); res.flush()
base = ms(0.5); a = look("base-a"); time.sleep(2); b = look("base-b"); noise = diff(a, b)
say("BASE", base, "picture noise", round(noise, 3))
n = 0
for name, val in CANDS:
    cur = get(name)
    if cur is None: say(name, "not in this build"); continue
    if abs(cur - float(val)) < 1e-6: say(name, "already", cur); continue
    ref = look("ref"); cmd(f"{name} {val}")
    m = ms(); pic = look("t-" + name.replace(".", "_")); d = diff(ref, pic)
    cmd(f"{name} {cur:g}")
    say(name, f"{cur:g}->{val}", "ms", None if m is None else round(m, 2), "delta", None if m is None or base is None else round(m - base, 2), "picture", round(d, 3), "x noise", round(d / max(noise, 1e-3), 1))
    n += 1
    if n % 8 == 0:
        base = ms(4.0); say("BASE", base)
say("END", ms(4.0))
