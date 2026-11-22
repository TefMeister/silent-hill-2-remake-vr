"""flick.py <name> [seconds=2] [boxes: label=x,y,w,h ...]: record the simulator's RIGHT eye (1712x1381 on screen) at 60 a second, lossless;
back-and-forth flicker per pixel (min of the two neighbouring frame differences, mean over time); prints the mean in each box; saves flick.png and mid.png"""
import sys, os, subprocess, glob, numpy as np
from PIL import Image
n=sys.argv[1]; t=sys.argv[2] if len(sys.argv)>2 and '=' not in sys.argv[2] else '2'
boxes=[a.split('=') for a in sys.argv[2:] if '=' in a]
d='flk/'+n; os.makedirs(d,exist_ok=True)
for f in glob.glob(d+'/f*.png'): os.remove(f)
subprocess.run(['ffmpeg','-y','-loglevel','error','-f','gdigrab','-framerate','60','-draw_mouse','0','-offset_x','1720','-offset_y','51','-video_size','1712x1381','-t',t,'-i','desktop','-c:v','libx264rgb','-qp','0','-preset','ultrafast',d+'/clip.mkv'])
subprocess.run(['ffmpeg','-y','-loglevel','error','-i',d+'/clip.mkv',d+'/f%03d.png'])
fs=sorted(glob.glob(d+'/f*.png'))
A=np.stack([np.asarray(Image.open(f).convert('L'),dtype=np.float32) for f in fs])
for f in fs[1:]: os.remove(f)
dd=np.abs(np.diff(A,axis=0)); keep=dd.mean(axis=(1,2))>0.01
A=A[np.concatenate([[True],keep])]; dd=np.abs(np.diff(A,axis=0))
fl=np.minimum(dd[:-1],dd[1:]).mean(0)
Image.fromarray(np.clip(fl/3.0*255,0,255).astype('uint8')).save(d+'/flick.png')
os.replace(fs[0],d+'/mid.png')
out='%-16s frames %d | whole %.3f'%(n,len(A),fl.mean())
for lab,b in boxes:
    x,y,w,h=map(int,b.split(',')); out+=' | %s %.3f'%(lab,fl[y:y+h,x:x+w].mean())
print(out)
