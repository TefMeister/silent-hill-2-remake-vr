"""pair.py <incident_dir>: THE MADE-UP EYE AGAINST THE DRAWN EYE, frame by frame (2026-11-18).
With the frame warp one eye of every frame is really drawn and the other is made up. The hand's place in the left
picture minus its place in the right one (the skin-coloured pixels' centre) is the hand's stereo offset: it changes
slowly. If the made-up eye's hand is not where the game drew it, that difference jumps one way on even frames and
the other way on odd ones. So its frame-to-frame FLIP measures the carry's error alone: the hand's own motion, however
jerky, is in both eyes and cancels. Prints the flip in pixels of the capture and how many frames had the hand in both eyes."""
import glob, os, re, sys
import numpy as np, cv2
d = sys.argv[1]
fr = sorted({int(re.search(r"frame(\d+)_color_L", f).group(1)) for f in glob.glob(os.path.join(d, "frame*_color_L.bmp"))})
def centre(n, e):
    bgr = cv2.imread(os.path.join(d, "frame%d_color_%s.bmp" % (n, e)))
    hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
    m = (hsv[..., 0] >= 3) & (hsv[..., 0] <= 22) & (hsv[..., 1] >= 50) & (hsv[..., 2] >= 90)
    ys, xs = np.nonzero(m)
    return (xs.mean(), ys.mean(), len(xs), bgr.shape[1]) if len(xs) > 25 else None
dx = []; dy = []; W = 0
for n in fr:
    a, b = centre(n, "L"), centre(n, "R")
    if a and b: dx.append(a[0] - b[0]); dy.append(a[1] - b[1]); W = a[3]
    else: dx.append(np.nan); dy.append(np.nan)
dx = np.array(dx); dy = np.array(dy)
fx = np.abs(dx[1:-1] - (dx[:-2] + dx[2:]) / 2); fy = np.abs(dy[1:-1] - (dy[:-2] + dy[2:]) / 2)
ok = int(np.sum(~np.isnan(fx)))
print("PAIR flip x %.2f px, y %.2f px (capture %d px wide; %d of %d frames with the hand in both eyes)" % (np.nanmean(fx) if ok else float("nan"), np.nanmean(fy) if ok else float("nan"), W, ok, len(fr)))
print("PAIR offsets x:", " ".join("%.1f" % v for v in dx))
