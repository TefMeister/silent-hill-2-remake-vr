"""Cumulative tuning pass: each step adds settings on top of the last, 30 s each, labelled in the perf log."""
import os, time, subprocess, sys

DATA = os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\data")
HERE = os.path.dirname(os.path.abspath(__file__))
HOLD_S = 32

STEPS = [
    ("s0 baseline max", []),
    ("s1 camera effects off", ["r.DepthOfFieldQuality 0", "r.FilmGrain 0", "r.SceneColorFringeQuality 0",
                               "r.Tonemapper.GrainQuantization 0", "r.MotionBlurQuality 0", "r.LensFlareQuality 0"]),
    ("s2 shorter shadows", ["r.Shadow.DistanceScale 0.6", "r.Shadow.MaxResolution 1024"]),
    ("s3 coarser fog grid", ["r.VolumetricFog.GridPixelSize 16", "r.VolumetricFog.GridSizeZ 64"]),
    ("s4 lumen gather downsample 32", ["r.Lumen.ScreenProbeGather.DownsampleFactor 32"]),
    ("s5 lumen reflections off", ["r.Lumen.Reflections.Allow 0"]),
    ("s6 ssr + ao off", ["r.SSR.Quality 0", "r.AmbientOcclusionLevels 0"]),
    ("s7 view distance 0.7", ["r.ViewDistanceScale 0.7"]),
]

only = sys.argv[1:]  # optional: run only these step indexes
for i, (label, cmds) in enumerate(STEPS):
    if only and str(i) not in only:
        continue
    body = f"id{time.time():.0f}_{i}\nlabel {label}\n" + "".join(f"cmd {c}\n" for c in cmds)
    with open(os.path.join(DATA, "sh2_perf_cmd.txt"), "w") as f:
        f.write(body)
    print(time.strftime("%H:%M:%S"), label, flush=True)
    time.sleep(HOLD_S)
    subprocess.run(["python", os.path.join(HERE, "sh2.py"), "shot", f"tune_{i}"], capture_output=True)
with open(os.path.join(DATA, "sh2_perf_cmd.txt"), "w") as f:
    f.write(f"id{time.time():.0f}_end\nlabel end\n")
print("done", flush=True)
