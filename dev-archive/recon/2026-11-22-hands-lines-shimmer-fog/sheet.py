"""sheet.py <clip> <x0> <y0> <w> <h> [first=1] [count=8] [zoom=3] [gamma=0.6]: consecutive frames' crop, enlarged (nearest), in a row sheet; distinct frames only"""
import sys, glob, numpy as np
from PIL import Image
c=sys.argv[1]; x0,y0,w,h=map(int,sys.argv[2:6]); first=int(sys.argv[6]) if len(sys.argv)>6 else 1; cnt=int(sys.argv[7]) if len(sys.argv)>7 else 8
zoom=int(sys.argv[8]) if len(sys.argv)>8 else 3; g=float(sys.argv[9]) if len(sys.argv)>9 else 0.6
fs=sorted(glob.glob('clips/%s/[0-9]*.png'%c)); out=[]; prev=None; names=[]
for f in fs[first-1:]:
    a=np.asarray(Image.open(f).convert('RGB'))[y0:y0+h,x0:x0+w].astype(np.float32)
    if prev is not None and np.abs(a-prev).mean()<0.02: continue
    prev=a; out.append(a); names.append(f[-7:-4])
    if len(out)>=cnt: break
cols=min(4,len(out)); rows=(len(out)+cols-1)//cols
S=Image.new('RGB',(cols*w*zoom+ (cols-1)*4, rows*h*zoom+(rows-1)*4),(255,0,255))
for i,a in enumerate(out):
    im=Image.fromarray((np.clip((a/255)**g,0,1)*255).astype('uint8')).resize((w*zoom,h*zoom),Image.NEAREST)
    S.paste(im,((i%cols)*(w*zoom+4),(i//cols)*(h*zoom+4)))
S.save('clips/%s/sheet_%d_%d.png'%(c,x0,y0)); print(names, S.size)
