"""shader_compile.py: compile handpaste.cpp's pass (the HLSL inside it) with the same D3DCompile call the plugin makes at
game start, so a broken shader is found at the desk, not as "handpaste: switched itself off" in the headset.
python -I dev-archive/native/tests/shader_compile.py"""
import ctypes, os, re, sys
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "handpaste.cpp"), encoding="utf-8").read()
hlsl = re.search(r'const char\* SHADER = R"\((.*?)\)";', src, re.S).group(1).encode()
d3dc = ctypes.WinDLL("d3dcompiler_47.dll")
class Blob(ctypes.Structure): pass
fails = 0
for entry, target in ((b"vs", b"vs_5_0"), (b"ps", b"ps_5_0")):
    code = ctypes.c_void_p(); err = ctypes.c_void_p()
    hr = d3dc.D3DCompile(hlsl, len(hlsl), b"handpaste", None, None, entry, target, 0, 0, ctypes.byref(code), ctypes.byref(err))
    if hr != 0:
        fails += 1
        msg = ""
        if err.value:
            vt = ctypes.cast(ctypes.cast(err, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
            ptr = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(vt[3])(err)
            size = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(vt[4])(err)
            msg = ctypes.string_at(ptr, size).decode(errors="replace")
        print("FAIL", entry.decode(), hex(hr & 0xffffffff), msg)
    else:
        print("PASS", entry.decode(), "compiles")
sys.exit(1 if fails else 0)
