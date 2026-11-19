"""twohand_hold.py <seconds> [mode=still|circle|rturn|wave] [r=0.06] [hz=0.5] [lg=1] -- Silent Hill 2 on the OpenXR
Simulator with VR Bench: a long gun held in two hands, for the "gun curls up by itself" fault (Tefa 2026-11-19).

The right controller level and forward at chest height (NO right grip: in this mod the right grip is the aim stance),
the left controller 38 cm further along the barrel. The first second has the left grip open, then it closes (that edge
is what starts two hands in twohand.cpp) and stays closed for <seconds>; then it opens and the tool holds 1 s more.
  still   both hands still (a 2 mm wobble: a controller held perfectly still goes to sleep)
  circle  the LEFT hand goes round a circle of radius r in the plane across the barrel: the gun's direction sweeps a cone
  rturn   the RIGHT controller rolls and pitches +-20 deg in place, the left hand still
  wave    both hands swing left-right together, 15 cm
lg=0 never closes the left grip (the control run: the gun in the right hand alone).
One writer at a time for VR Bench's input: stop any other hold tool first. Poses are in ROOM space, metres; the
simulator's head stays at its default (0, 1.7, 0). Read the header, do not run it with --help."""
import math, os, sys, time
sys.path.insert(0, os.path.expanduser("~/github-backups/_lanes-next-wt/plugins/lanes/tools/vrbench"))
import vb_input as I, vb_settings as S

secs = float(sys.argv[1])
opt = {"mode": "still", "r": "0.06", "hz": "0.5", "lg": "1"}
for a in sys.argv[2:]:
    k, v = a.split("=", 1)
    opt[k] = v
mode, r, hz, use_lg = opt["mode"], float(opt["r"]), float(opt["hz"]), opt["lg"] != "0"

RIGHT = [0.15, 1.30, -0.20]
LEFT = [0.13, 1.32, -0.58]
WOBBLE_M, WOBBLE_RATE = 0.002, 40.0


def put(t, lg):
    w = WOBBLE_M * math.sin(t * WOBBLE_RATE)
    ph = 2 * math.pi * hz * t
    rp, lp = list(RIGHT), list(LEFT)
    ryaw = rpitch = rroll = 0.0
    if mode == "circle":
        lp[0] += r * math.cos(ph); lp[1] += r * math.sin(ph)
    elif mode == "rturn":
        rroll = 20 * math.sin(ph); rpitch = 20 * math.sin(ph * 0.7)
    elif mode == "wave":
        s = 0.15 * math.sin(ph)
        rp[0] += s; lp[0] += s
    inp = I.Input().mode(poses="set", buttons="set")
    p = I.pose_from_ypr([rp[0] + w, rp[1], rp[2]], ryaw, rpitch, rroll)
    inp.hand(S.HAND_RIGHT, grip=p, aim=p, squeeze=0.0, trigger=0.0)
    p = I.pose_from_ypr([lp[0] + w, lp[1], lp[2]], 0, 0)
    inp.hand(S.HAND_LEFT, grip=p, aim=p, squeeze=1.0 if lg else 0.0, trigger=0.0)
    I.write(inp)
    time.sleep(0.01)


t0 = time.time()
while time.time() - t0 < 1.0: put(0.0, False)
t0 = time.time()
while time.time() - t0 < secs: put(time.time() - t0, use_lg)
t0 = time.time()
while time.time() - t0 < 1.0: put(0.0, False)
print("twohand_hold: %s for %.0f s, left grip %s" % (mode, secs, "closed" if use_lg else "open"))
