"""List the console settings (cvars) a UE5 executable contains, filtered to the VR-performance ones."""
import re, sys, json

EXE = sys.argv[1] if len(sys.argv) > 1 else "SHProto-Win64-Shipping.exe"
d = open(EXE, "rb").read()

names = set()
for m in re.finditer(rb"((?:[a-zA-Z]\x00){1,6}\.\x00(?:[A-Za-z0-9_.]\x00){2,80})", d):
    s = m.group(1).decode("utf-16le")
    if re.match(r"^(r|sg|fx|foliage|grass|a)\.[A-Za-z]", s):
        names.add(s)
for m in re.finditer(rb"\b((?:r|sg)\.[A-Za-z][A-Za-z0-9_.]{2,80})", d):
    names.add(m.group(1).decode())

WANT = {
    "motion blur": [r"MotionBlur"],
    "depth of field": [r"DepthOfField", r"\.DOF"],
    "film grain": [r"Grain"],
    "colour fringing": [r"SceneColorFringe", r"ChromaticAberration"],
    "lens flares": [r"LensFlare"],
    "vignette": [r"Vignette"],
    "bloom": [r"^r\.Bloom"],
    "lumen (lighting)": [r"^r\.Lumen\.(DiffuseIndirect|Reflections)\.(Allow|Quality)", r"^r\.Lumen\.(HardwareRayTracing|ScreenProbeGather\.(DownsampleFactor|RadianceCache))$", r"^r\.DynamicGlobalIlluminationMethod", r"^r\.ReflectionMethod"],
    "volumetric fog": [r"^r\.VolumetricFog(\.GridPixelSize|\.GridSizeZ|$)", r"^r\.VolumetricCloud$"],
    "shadows": [r"^r\.Shadow\.(DistanceScale|MaxResolution|Virtual\.Enable|CSM\.MaxCascades)$"],
    "screen-space reflections / AO": [r"^r\.SSR\.Quality$", r"^r\.AmbientOcclusionLevels$"],
    "resolution / upscaling": [r"^r\.ScreenPercentage$", r"^r\.TemporalAA\.Upscaler$", r"^r\.AntiAliasingMethod$", r"^r\.NGX\.DLSS\.Enable", r"^r\.FidelityFX\.FSR2?\.Enabled"],
    "translucency / particles": [r"^r\.SeparateTranslucency", r"^fx\.MaxCPUParticlesPerEmitter"],
    "view distance": [r"^r\.ViewDistanceScale$", r"^r\.StaticMeshLODDistanceScale$", r"^foliage\.LODDistanceScale$"],
    "panini (flat-screen lens)": [r"Panini"],
}
out = {}
for k, pats in WANT.items():
    hits = sorted(n for n in names if any(re.search(p, n) for p in pats))
    out[k] = hits
print(json.dumps(out, indent=1))
print("total cvar-like names:", len(names), file=sys.stderr)
