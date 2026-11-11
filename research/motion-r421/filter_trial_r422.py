"""Approximate R422 surface-weight replay: identity source mapping, not live native-map reconstruction or RR. No quality guarantee."""
from pathlib import Path
exec(Path(__file__).with_name('render_buffers.py').read_text().split('for label,roi in rois.items():')[0])
from scipy.ndimage import median_filter
for label,roi in {'hair':(250,460,560,680),'jaw':(980,680,1020,705)}.items():
 for f in sorted((root/label).glob('rr-input*/')):
  x,y,X,Y=roi;rgb=read(f,'lighting-diffuse').astype('float32');normal=read(f,'normal-roughness')[:,:,:3].astype('float32');g2=read(f,'native-gbuffer2');g1=read(f,'native-gbuffer1')
  center=rgb[y:Y,x:X,:3];n=normal[y:Y,x:X];total=center*4;weight=np.full(center.shape[:2],4.,dtype='float32')
  for dy in [-2,0,2]:
   for dx in [-2,0,2]:
    if dx==0 and dy==0:continue
    sn=normal[y+dy:Y+dy,x+dx:X+dx];tap=rgb[y+dy:Y+dy,x+dx:X+dx,:3]
    same=(g2[y:Y,x:X,2:]==g2[y+dy:Y+dy,x+dx:X+dx,2:]).all(axis=-1)&((g1[y:Y,x:X,3]&1)==(g1[y+dy:Y+dy,x+dx:X+dx,3]&1))
    w=np.clip(np.sum(n*sn,axis=-1)*20-19,0,1)**2*(2 if dx==0 or dy==0 else 1)*same
    total+=tap*w[:,:,None];weight+=w
  result=total/weight[:,:,None]
  before=center@np.array([.2126,.7152,.0722]);after=result@np.array([.2126,.7152,.0722]);err=lambda z:np.sqrt(np.mean((z-median_filter(z,size=5))**2))
  print(label,f.name,'mean ratio',after.mean()/before.mean(),'RMS localresidual ratio',err(after)/err(before),'peak ratio',after.max()/before.max(),'weights',np.percentile(weight,[0,50,100]))
