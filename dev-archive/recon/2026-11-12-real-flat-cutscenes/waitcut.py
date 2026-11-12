"""waitcut.py <seconds after the cutscene began> : returns when the newest 'cutscene began' line in the injector's log is that old."""
import os, sys, time, datetime
LOG = os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\log.txt")
want = float(sys.argv[1]); start = time.time()
while time.time() - start < 400:
    try: ls = [l for l in open(LOG, errors="replace").read().splitlines() if "cutscene began" in l]
    except OSError: ls = []
    if ls:
        t = datetime.datetime.strptime(ls[-1][1:20], "%Y-%m-%d %H:%M:%S")
        age = (datetime.datetime.now() - t).total_seconds()
        if 0 <= age < want + 20:
            if age >= want: print("cutscene is", round(age), "s in"); sys.exit(0)
    time.sleep(0.5)
print("no cutscene line"); sys.exit(1)
