"""draw.py <right hip|right shoulder|left shoulder|left hip> -- Silent Hill 2 on the OpenXR Simulator with VR Bench: the
right hand goes to one of the mod's holster spots (settings.h HOLSTER_SPOTS, measured from the headset, here the
simulator's default head at 0, 1.7, 0), closes the right grip for 0.4 s (the draw, or the put-back when that weapon is
out), opens it and comes back in front of the chest. The same call again puts the weapon back.
One writer at a time for VR Bench's input. Read the header, do not run it with --help. 2026-11-19."""
import math, os, sys, time
sys.path.insert(0, os.path.expanduser("~/github-backups/_lanes-next-wt/plugins/lanes/tools/vrbench"))
import vb_input as I, vb_settings as S

HEAD = (0.0, 1.7, 0.0)
SPOTS = {"right hip": (20, -35, 10), "right shoulder": (15, -10, -10), "left shoulder": (-15, -10, -10), "left hip": (-20, -35, 10)}   # right, up, forward (cm)
name = " ".join(sys.argv[1:]).strip().lower()
if name not in SPOTS:
    sys.exit(__doc__)
r, u, f = SPOTS[name]
spot = [HEAD[0] + r / 100.0, HEAD[1] + u / 100.0, HEAD[2] - f / 100.0]   # room: -z is forward
REST_R, REST_L = [0.15, 1.30, -0.20], [-0.20, 1.20, -0.25]


def put(pos, grip, secs):
    t0 = time.time()
    while time.time() - t0 < secs:
        w = 0.002 * math.sin((time.time() - t0) * 40.0)
        inp = I.Input().mode(poses="set", buttons="set")
        p = I.pose_from_ypr([pos[0] + w, pos[1], pos[2]], 0, 0)
        inp.hand(S.HAND_RIGHT, grip=p, aim=p, squeeze=grip, trigger=0.0)
        p = I.pose_from_ypr([REST_L[0] + w, REST_L[1], REST_L[2]], 0, 0)
        inp.hand(S.HAND_LEFT, grip=p, aim=p, squeeze=0.0, trigger=0.0)
        I.write(inp)
        time.sleep(0.01)


put(REST_R, 0.0, 0.5)
put(spot, 0.0, 0.8)
put(spot, 1.0, 0.4)
put(spot, 0.0, 0.4)
put(REST_R, 0.0, 1.0)
print("draw: right grip pressed at the %s" % name)
