"""sheet.py <label> <from> <to> <step> [cols] [thumb width] : a contact sheet of the stills t<from>..t<to>."""
import os, sys
from PIL import Image, ImageDraw
S=os.path.dirname(os.path.abspath(__file__)); lab=sys.argv[1]; a,b,st=int(sys.argv[2]),int(sys.argv[3]),int(sys.argv[4])
cols=int(sys.argv[5]) if len(sys.argv)>5 else 6; tw=int(sys.argv[6]) if len(sys.argv)>6 else 320
ims=[]
for t in range(a,b+1,st):
    p=os.path.join(S,"runs",lab,f"t{t:03d}.jpg")
    if os.path.exists(p):
        im=Image.open(p); im.thumbnail((tw,tw)); ImageDraw.Draw(im).text((4,2),f"{lab} +{t}s",fill=(255,255,0)); ims.append(im)
if not ims: print("no stills"); sys.exit(1)
w,h=ims[0].size; rows=(len(ims)+cols-1)//cols; sh=Image.new("RGB",(cols*w,rows*h))
for i,im in enumerate(ims): sh.paste(im,((i%cols)*w,(i//cols)*h))
out=os.path.join(S,"runs",f"sheet-{lab}-{a}-{b}.jpg"); sh.save(out,quality=80); print(out, sh.size)
