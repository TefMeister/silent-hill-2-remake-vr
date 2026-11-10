"""cvar_ab.py -- A/B/A on engine settings while the game runs in VR (the optimiser, 2026-11-10).

    python cvar_ab.py "label: name=value name=value" ["label2: ..."] ...

For each test: the frame time as it is (A), the settings written to the profile's sh2vr_devcvars.txt (the plugin applies
them and logs each value before -> after), the frame time with them (B), the old values written back, the frame time again
(A2). Frame time = the plugin's heartbeat, the average of three 5 s readings after a settle. Prints one line per test.
A setting the build does not have is said. The game must be in gameplay, standing still, with the b004+ plugin.
"""
import os
import re
import sys
import time

PROFILE = os.path.join(os.environ["APPDATA"], "UnrealVRMod", "SHProto-Win64-Shipping")
LOG = os.path.join(PROFILE, "log.txt")
FILE = os.path.join(PROFILE, "sh2vr_devcvars.txt")
SETTLE_S = 6
READINGS = 3


def log_from(mark):
    with open(LOG, "rb") as f:
        f.seek(mark)
        return f.read().decode("utf-8", "replace")


def frame_ms():
    time.sleep(SETTLE_S)
    mark = os.path.getsize(LOG)
    vals = []
    t0 = time.time()
    while len(vals) < READINGS and time.time() - t0 < 60:
        time.sleep(1)
        vals = [float(x) for x in re.findall(r"sh2vr\] alive: frame \d+, ([0-9.]+) ms avg, headset on", log_from(mark))]
    return sum(vals[:READINGS]) / max(1, len(vals[:READINGS])) if vals else float("nan")


def apply(settings):
    """write the file, wait for the plugin's lines; {name: value before} and the names it did not find"""
    mark = os.path.getsize(LOG)
    with open(FILE, "w", encoding="ascii") as f:
        f.write("# written by cvar_ab.py %s\n" % time.strftime("%H:%M:%S"))
        for k, v in settings.items():
            f.write(f"{k} {v}\n")
    t0 = time.time()
    text = ""
    while "devcvars: file read" not in text and time.time() - t0 < 10:
        time.sleep(0.5)
        text = log_from(mark)
    before = {m[0]: m[1] for m in re.findall(r"devcvars: (\S+) (\S+) -> \S+", text)}
    missing = re.findall(r"devcvars: (\S+) is not in this build", text)
    return before, missing


def main():
    for spec in sys.argv[1:]:
        label, _, rest = spec.partition(":")
        settings = dict(p.split("=", 1) for p in rest.split())
        a = frame_ms()
        before, missing = apply(settings)
        b = frame_ms()
        apply(before)
        a2 = frame_ms()
        base = (a + a2) / 2
        note = f" | NOT IN THIS BUILD: {' '.join(missing)}" if missing else ""
        was = " ".join(f"{k}={v}" for k, v in before.items())
        print(f"{label.strip():34s} A {a:5.2f}  B {b:5.2f}  A2 {a2:5.2f}  change {b - base:+6.2f} ms ({(b - base) / base * 100:+5.1f} %) | was: {was}{note}", flush=True)
    try:
        os.remove(FILE)
    except OSError:
        pass


if __name__ == "__main__":
    main()
