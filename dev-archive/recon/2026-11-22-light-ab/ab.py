import starts as S, os, subprocess, time, json, sys, numpy as np, glob
from PIL import Image
PROF=os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping")
open(os.path.join(PROF,"sh2vr_notune.txt"),"w").write("light A/B test 2026-11-22, remove after\n")
BASE={"r.Lumen.Reflections.DownsampleFactor":2,"r.Lumen.Reflections.ScreenSpaceReconstruction.NumSamples":3,"r.Lumen.Reflections.MaxRoughnessToTrace":0.3,
      "r.AOGlobalDistanceField":0,"r.DistanceFieldAO":0,"r.Lumen.ScreenProbeGather.DownsampleFactor":32,"r.Lumen.HardwareRayTracing":0,"r.Lumen.Reflections.HardwareRayTracing":0}
V=[("A_b038",{}),("B_globalfield",{"r.AOGlobalDistanceField":1}),("C_plus_dfao",{"r.AOGlobalDistanceField":1,"r.DistanceFieldAO":1}),
   ("D_plus_probes16",{"r.AOGlobalDistanceField":1,"r.DistanceFieldAO":1,"r.Lumen.ScreenProbeGather.DownsampleFactor":16}),
   ("E_game_own_rt_off",{"r.AOGlobalDistanceField":1,"r.DistanceFieldAO":1,"r.Lumen.ScreenProbeGather.DownsampleFactor":16,"r.Lumen.Reflections.DownsampleFactor":1,"r.Lumen.Reflections.ScreenSpaceReconstruction.NumSamples":5,"r.Lumen.Reflections.MaxRoughnessToTrace":0.4}),
   ("F_game_own_rt_on",{"r.AOGlobalDistanceField":1,"r.DistanceFieldAO":1,"r.Lumen.ScreenProbeGather.DownsampleFactor":16,"r.Lumen.Reflections.DownsampleFactor":1,"r.Lumen.Reflections.ScreenSpaceReconstruction.NumSamples":5,"r.Lumen.Reflections.MaxRoughnessToTrace":0.4,"r.Lumen.HardwareRayTracing":1,"r.Lumen.Reflections.HardwareRayTracing":1}),
   ("A2_b038_again",{})]
time.sleep(6)
for name,chg in V:
    vals=dict(BASE); vals.update(chg)
    for k,v in vals.items(): S.http("/api/console/cvar",{"name":k,"value":str(v)})
    time.sleep(12)
    back={k:json.loads(S.http("/api/console/cvar?name="+k) or "{}").get("float") for k in chg}
    d=os.path.join("ab",name); os.makedirs(d,exist_ok=True)
    for f in glob.glob(d+"/*.png"): os.remove(f)
    subprocess.run(["ffmpeg","-y","-loglevel","error","-f","gdigrab","-framerate","30","-offset_x","744","-offset_y","152","-video_size","1920x1080","-t","1.5","-i","desktop",d+"/%03d.png"])
    fs=sorted(glob.glob(d+"/*.png")); A=np.stack([np.asarray(Image.open(f).convert("RGB"),dtype=np.float32) for f in fs])
    g=A.mean(axis=3); dd=np.abs(np.diff(g,axis=0)); fl=np.minimum(dd[:-1],dd[1:]).mean()
    m=A.mean(axis=0); Image.fromarray(m.astype("uint8")).save(os.path.join("ab",name+".png"))
    print("%-20s frames %d brightness %.1f (top third %.1f, bottom third %.1f) flicker %.2f read back %s"%(name,len(fs),g.mean(),g[:,:360].mean(),g[:,720:].mean(),fl,back),flush=True)
    for f in fs: os.remove(f)
os.remove(os.path.join(PROF,"sh2vr_notune.txt"))
print("tuning file removed; the plugin puts b038's values back within 5 s")
