"""speck.py <clip> [x0 y0 w h]: per distinct frame, how many pixels stand out from their 5x5 median by more than 14 (of 255) in the hand area; prints mean, median, top and the worst frame"""
import sys, glob, numpy as np
from PIL import Image
import cv2
def median_filter(a,k): return cv2.medianBlur(a,k)
c=sys.argv[1]; x0,y0,w,h=(list(map(int,sys.argv[2:6])) if len(sys.argv)>5 else [1100,150,620,300])
prev=None; res=[]
for f in sorted(glob.glob('clips/%s/[0-9]*.png'%c)):
    a=np.asarray(Image.open(f).convert('L'),dtype=np.float32)[y0:y0+h,x0:x0+w]
    if prev is not None and np.abs(a-prev).mean()<0.02: continue
    mv = 0 if prev is None else np.abs(a-prev).mean()
    prev=a; res.append((int((np.abs(a-median_filter(a,5))>14).sum()), f[-7:-4], mv))
v=np.array([r[0] for r in res]); worst=sorted(res)[-3:]
print('%-12s frames %d | mean %.0f | quarter %.0f median %.0f three-quarter %.0f top %d | extra over the clean frames %.0f %s| worst %s'%(c,len(v),v.mean(),np.percentile(v,25),np.median(v),np.percentile(v,75),v.max(),v.mean()-np.percentile(v,20),'' if v.max()-v.min()>300 else 'NOT MOVING ',[w[1] for w in worst]))
