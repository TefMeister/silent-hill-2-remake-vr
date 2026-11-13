"""odd.py <video> [bucket seconds] : one-frame wrong pictures per time bucket (a frame unlike both neighbours while they agree)."""
import subprocess, sys, numpy as np
v=sys.argv[1]; bucket=int(sys.argv[2]) if len(sys.argv)>2 else 20; W,H=160,90
p=subprocess.Popen(["ffmpeg","-v","error","-i",v,"-vf",f"scale={W}:{H}","-f","rawvideo","-pix_fmt","gray","-"],stdout=subprocess.PIPE)
prev2=prev=None; n=0; big=[]
while True:
    b=p.stdout.read(W*H)
    if len(b)<W*H: break
    cur=np.frombuffer(b,np.uint8).astype(np.int16)
    if prev2 is not None:
        a=np.abs(prev-prev2).mean(); bb=np.abs(prev-cur).mean(); c=np.abs(prev2-cur).mean()
        if min(a,bb)-c>5: big.append(n-1)
    prev2,prev=prev,cur; n+=1
h=np.histogram(np.array(big)/60, bins=range(0,int(n/60)+bucket,bucket))[0] if big else []
print("seconds",round(n/60),"odd",len(big)); print("per",bucket,"s:",list(map(int,h)))
