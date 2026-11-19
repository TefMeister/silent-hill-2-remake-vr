"""press.py <rt|lt|rg|lg|a|b|x|y|menu> [seconds=0.4] -- Silent Hill 2 on the OpenXR Simulator with VR Bench: one press of
one VR controller button with both hands resting in front of the chest (rt/lt triggers, rg/lg grips, a/b on the right
controller, x/y/menu on the left). While VR Bench is on, the simulator's own controller file (sim_ctl.py) does not reach
the game: use this instead, e.g. `press.py rt` at PRESS ANY BUTTON TO PLAY. One writer at a time. 2026-11-19."""
import math, os, sys, time
sys.path.insert(0, os.path.expanduser("~/github-backups/_lanes-next-wt/plugins/lanes/tools/vrbench"))
import vb_input as I, vb_settings as S

if len(sys.argv) < 2 or sys.argv[1] not in ("rt", "lt", "rg", "lg", "a", "b", "x", "y", "menu"):
    sys.exit(__doc__)
what = sys.argv[1]
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 0.4
REST_R, REST_L = [0.15, 1.30, -0.20], [-0.20, 1.20, -0.25]


def put(on, dur):
    t0 = time.time()
    while time.time() - t0 < dur:
        w = 0.002 * math.sin((time.time() - t0) * 40.0)
        inp = I.Input().mode(poses="set", buttons="set")
        r = {"squeeze": 0.0, "trigger": 0.0, "a": False, "b": False}
        l = {"squeeze": 0.0, "trigger": 0.0, "x": False, "y": False, "menu": False}
        if on:
            if what == "rt": r["trigger"] = 1.0
            elif what == "rg": r["squeeze"] = 1.0
            elif what == "lt": l["trigger"] = 1.0
            elif what == "lg": l["squeeze"] = 1.0
            elif what in r: r[what] = True
            else: l[what] = True
        p = I.pose_from_ypr([REST_R[0] + w, REST_R[1], REST_R[2]], 0, 0)
        inp.hand(S.HAND_RIGHT, grip=p, aim=p, **r)
        p = I.pose_from_ypr([REST_L[0] + w, REST_L[1], REST_L[2]], 0, 0)
        inp.hand(S.HAND_LEFT, grip=p, aim=p, **l)
        I.write(inp)
        time.sleep(0.01)


put(False, 0.3); put(True, secs); put(False, 0.5)
print("press: %s for %.1f s" % (what, secs))
