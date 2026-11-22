"""zoom.py <clip> <x0> <y0> <w> <h> <frame> [frame...]: those frames' crop stacked, 3x nearest"""
import sys, numpy as np
from PIL import Image
c=sys.argv[1]; x0,y0,w,h=map(int,sys.argv[2:6])
ims=[np.asarray(Image.open('clips/%s/%s.png'%(c,f)).convert('RGB'))[y0:y0+h,x0:x0+w] for f in sys.argv[6:]]
S=np.concatenate(ims,0).astype(np.float32)
Image.fromarray((np.clip((S/255)**0.7,0,1)*255).astype('uint8')).resize((w*3,h*3*len(ims)),Image.NEAREST).save('clips/%s/zoom.png'%c)
