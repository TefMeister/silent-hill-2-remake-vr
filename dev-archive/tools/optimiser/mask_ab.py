"""mask_ab.py -- A/B/A on pieces hung on the headset (the eye-corner job, 2026-11-10).

    python mask_ab.py "label: x y z pitch yaw roll sx sy sz; x y z ..." ...

Each test: the frame time without pieces (A), with them (B), without again (A2). The pieces go into the profile's
sh2vr_mask.txt (the b005+ plugin builds them on the headset); the file is deleted between tests and at the end.
"""
import os
import sys
import time

import cvar_ab as C

FILE = os.path.join(C.PROFILE, "sh2vr_mask.txt")


def put(lines):
    mark = os.path.getsize(C.LOG)
    if lines:
        with open(FILE, "w", encoding="ascii") as f:
            f.write("# written by mask_ab.py\n" + "\n".join(lines) + "\n")
    elif os.path.exists(FILE):
        os.remove(FILE)
    time.sleep(3)
    return [ln.split("[sh2vr] ", 1)[-1].strip() for ln in C.log_from(mark).splitlines() if "cornermask:" in ln]


def main():
    for spec in sys.argv[1:]:
        label, _, rest = spec.partition(":")
        lines = [p.strip() for p in rest.split(";") if p.strip()]
        put([])
        a = C.frame_ms()
        said = put(lines)
        b = C.frame_ms()
        put([])
        a2 = C.frame_ms()
        base = (a + a2) / 2
        print(f"{label.strip():38s} A {a:5.2f}  B {b:5.2f}  A2 {a2:5.2f}  change {b - base:+6.2f} ms ({(b - base) / base * 100:+5.1f} %) | {'; '.join(said)[:90]}", flush=True)


if __name__ == "__main__":
    main()
