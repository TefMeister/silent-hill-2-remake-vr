"""steps.py <flk name>: how flat-stepped the fog is in that recording's kept frame (share of pixels equal to all four neighbours)"""
import sys, numpy as np, cv2
a=cv2.imread('flk/%s/mid.png'%sys.argv[1]); out=sys.argv[1]+':'
for nm,(x,y,w,h) in {'fog over the road':(1000,430,600,200),'sky fog':(900,150,700,150),'pavement':(600,900,800,400)}.items():
    b=a[y:y+h,x:x+w,1].astype(np.int32)
    same=(b[1:-1,1:-1]==b[1:-1,:-2])&(b[1:-1,1:-1]==b[1:-1,2:])&(b[1:-1,1:-1]==b[:-2,1:-1])&(b[1:-1,1:-1]==b[2:,1:-1])
    out+=' %s %.0f%%'%(nm,same.mean()*100)
c=a[430:630,1000:1600,1]; cv2.imwrite('flk/%s/fogsteps.png'%sys.argv[1],cv2.resize(((c.astype(np.int32)%2)*255).astype('uint8'),(1200,400),interpolation=cv2.INTER_NEAREST))
print(out)
