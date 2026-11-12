import subprocess, time, os, sys
S=os.path.dirname(os.path.abspath(__file__)); py=sys.executable
def k(vk): subprocess.run([py, S+"/sh2.py", "key", str(vk)], capture_output=True)
def shot(n): print(n, subprocess.run([py, S+"/sh2.py", "shot", S+"/"+n], capture_output=True, text=True).stdout.strip(), flush=True)
log=os.path.expandvars(r"%APPDATA%\UnrealVRMod\SHProto-Win64-Shipping\log.txt")
def lines(word): return [l[:210] for l in open(log,errors="replace").read().splitlines() if word in l]
time.sleep(62); k(13); time.sleep(8); shot("r1.png")
k(0x28); time.sleep(1); k(13); time.sleep(4)
for i in range(4): k(0x28); time.sleep(0.5)
shot("r2.png"); k(13); time.sleep(32); shot("r3.png"); k(13)
t0=time.time(); seen=False
while time.time()-t0<40:
    if lines("cutscreen:"): seen=True; break
    time.sleep(0.5)
print("cutscreen line:", seen, round(time.time()-t0,1), flush=True)
time.sleep(5); shot("c1.png"); time.sleep(8); shot("c2.png")
print("\n".join(lines("cutscreen:")[-4:])); print(lines("alive")[-1][60:])
print("game", "up" if b"SHProto" in subprocess.check_output("tasklist") else "GONE")
