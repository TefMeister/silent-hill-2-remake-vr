# grid.py <out> <label:key=val,key=val;scale> ... : for each case set the UEVR settings and the warp scale file, turn, take 2 captures; one grid picture
import json, os, sys, time, urllib.request, glob, subprocess
VB = sorted(glob.glob(os.path.expanduser('~/.claude/plugins/cache/*/*/*/tools/vrbench')))[-1]; sys.path.insert(0, VB)
import vb_sim
from PIL import Image, ImageDraw
B = os.path.expanduser('~/github-backups/silent-hill-2-remake-vr/dev-archive/recon/2026-10-29-buttons')
SC = os.path.join(os.environ["APPDATA"], "UnrealVRMod", "SHProto-Win64-Shipping", "sh2vr_warpscale.txt")
def post(path, body):
    r = urllib.request.Request("http://127.0.0.1:8899" + path, json.dumps(body).encode(), {"Content-Type": "application/json"})
    return json.loads(urllib.request.urlopen(r, timeout=5).read())
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "shots"); rows = []
for case in sys.argv[2:]:
    label, rest = case.split(":", 1); sets, scale = rest.split(";")
    for kv in [x for x in sets.split(",") if x]:
        k, v = kv.split("="); post("/api/vr/settings", {"key": k, "value": v})
    open(SC, "w").write(scale + "\n"); time.sleep(2.2)
    subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(B, "sh2_front.ps1")], capture_output=True)
    post("/api/input/gamepad", {"rightStick": {"x": 0.45, "y": 0}, "duration": 4}); time.sleep(1.5)
    tiles = []
    for i in range(2):
        p = os.path.join(D, "g_%s_%d.png" % (label, i))
        if vb_sim.screenshot(p, 5.0):
            im = Image.open(p); W, H = im.size; tiles.append(im.crop((0, int(H * 0.35), W, int(H * 0.95))).resize((W // 3, int(H * 0.6) // 3)))
    row = Image.new("RGB", (sum(t.width + 4 for t in tiles), tiles[0].height + 14)); x = 0
    for t in tiles: row.paste(t, (x, 14)); x += t.width + 4
    ImageDraw.Draw(row).text((4, 1), case, fill=(255, 255, 0)); rows.append(row); time.sleep(3.5)
sheet = Image.new("RGB", (max(r.width for r in rows), sum(r.height for r in rows))); y = 0
for r in rows: sheet.paste(r, (0, y)); y += r.height
sheet.save(sys.argv[1]); print(sheet.size)
