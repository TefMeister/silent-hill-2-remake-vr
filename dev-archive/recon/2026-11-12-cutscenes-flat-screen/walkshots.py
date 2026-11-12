import ctypes, ctypes.wintypes as wt, sys, time, subprocess, os
from PIL import Image
S=os.path.dirname(os.path.abspath(__file__)); u=ctypes.windll.user32
def win():
    out=[]
    P=ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h,_):
        n=u.GetWindowTextLengthW(h)
        if u.IsWindowVisible(h) and n:
            b=ctypes.create_unicode_buffer(n+1); u.GetWindowTextW(h,b,n+1)
            if b.value.startswith("Silent Hill 2"): out.append(h)
        return True
    u.EnumWindows(P(cb),0); return out[0]
h=win(); keys=[0x41,0x53,0x44,0x57]   # A S D W, each held ~1.2 s
ims=[]
for i,vk in enumerate(keys):
    sc=u.MapVirtualKeyW(vk,0); u.PostMessageW(h,0x100,vk,1|(sc<<16))
    for j in range(2):
        time.sleep(0.45); subprocess.run([sys.executable,S+"/sh2.py","shot",S+"/ws%d.png"%(i*2+j)],capture_output=True)
    u.PostMessageW(h,0x101,vk,0xC0000001|(sc<<16))
im=Image.new("RGB",(1200,340))
for k in range(8):
    a=Image.open(S+"/ws%d.png"%k); a.thumbnail((300,170)); im.paste(a,((k%4)*300,(k//4)*170))
im.point(lambda v:min(255,int(v*1.8))).save(S+"/"+sys.argv[1])
