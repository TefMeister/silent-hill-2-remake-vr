# skin.py <incident_dir> [--full]: per frame and eye, the x of the skin-coloured blobs (hands) and the room's shift.
# Prints, per eye: room shift per frame; the hand's x per frame; and the hand's alternation (mean |x[n]-mean(x[n-1],x[n+1])|).
import glob, os, sys, re
import numpy as np, cv2
d = sys.argv[1]
fr = sorted({int(re.search(r"frame(\d+)_color_L", f).group(1)) for f in glob.glob(os.path.join(d, "frame*_color_L.bmp"))})
def load(n, e): return cv2.imread(os.path.join(d, "frame%d_color_%s.bmp" % (n, e)))
res = {}
for e in "LR":
    prev = None; room = []; hx = []; cnt = []
    for n in fr:
        bgr = load(n, e); H, W = bgr.shape[:2]
        g = bgr.astype(np.float32).mean(axis=2)
        hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
        m = ((hsv[..., 0] >= 3) & (hsv[..., 0] <= 22) & (hsv[..., 1] >= 50) & (hsv[..., 2] >= 90)).astype(np.uint8)
        m[: int(H * 0.45)] = 0
        ys, xs = np.nonzero(m)
        hx.append(xs.mean() if len(xs) > 15 else float("nan")); cnt.append(len(xs))
        if prev is not None:
            hh = int(H * 0.45)
            (dx, dy), _ = cv2.phaseCorrelate(prev[:hh], g[:hh], cv2.createHanningWindow((W, hh), cv2.CV_32F))
            room.append(dx)
        prev = g
    hx = np.array(hx); room = np.array(room)
    alt = np.nanmean(np.abs(hx[1:-1] - (hx[:-2] + hx[2:]) / 2))
    print(e, "room/frame mean %.2f sd %.2f px" % (room.mean(), room.std()), "| skin px", int(np.mean(cnt)))
    print(e, "hand x:", " ".join("%.1f" % v for v in hx))
    print(e, "hand step:", " ".join("%+.1f" % v for v in np.diff(hx)))
    print(e, "ALTERNATION %.2f px  (= %.2f of the room's per-frame shift)" % (alt, alt / max(1e-6, abs(room.mean()))))
print("size", W, H, "frames", len(fr))
