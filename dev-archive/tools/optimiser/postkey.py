# post one key to the game's window (found from its process), no focus needed
import sys, ctypes, ctypes.wintypes as w, subprocess
u = ctypes.windll.user32
out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq SHProto-Win64-Shipping.exe", "/FO", "CSV", "/NH"], capture_output=True, text=True).stdout
pid = int(out.split(",")[1].strip('"'))
found = []
def cb(h, l):
    p = w.DWORD(); u.GetWindowThreadProcessId(h, ctypes.byref(p))
    if p.value == pid and u.IsWindowVisible(h): found.append(h)
    return True
u.EnumWindows(ctypes.WINFUNCTYPE(ctypes.c_bool, w.HWND, w.LPARAM)(cb), 0)
vk = int(sys.argv[1], 0)
for h in found:
    u.PostMessageW(h, 0x100, vk, 0); u.PostMessageW(h, 0x101, vk, 0xC0000001)
print("posted to", len(found), "window(s)")
