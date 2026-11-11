import numpy as np,json
from pathlib import Path
from PIL import Image,ImageDraw
import os
root=Path(os.environ['CONTROL_R421_CAPTURE_ROOT'])
rois={'jaw':(880,550,1080,730),'hair':(170,390,720,870)}
names=['pre-filter-gi','lighting-diffuse','lighting-specular','input-color','rr-output','diffuse','specular','normal-roughness','raw-albedo','hair-responsivity']
def read(f,n):
 m=json.loads((f/'metadata.json').read_text());r=next(r for r in m['resources'] if r['file'].startswith(n+'.'))
 dtype='<f4' if r['file'].endswith('r32f') else 'u1' if r['file'].endswith('rgba8') else '<f2'
 return np.memmap(f/r['file'],dtype=dtype,mode='r').reshape(r['height'],r['width'],-1)
def crop(a,roi):
 scale=a.shape[1]/2560;x,y,X,Y=[round(v*scale) for v in roi];return a[y:Y,x:X]
def im(a,mode):
 if a.shape[2]==1:a=np.repeat(a,3,axis=2)
 a=a[:,:,:3]
 if mode=='normal-roughness':a=a*.5+.5
 elif mode not in ['diffuse','specular','raw-albedo','hair-responsivity']:
  a=np.maximum(a,0)*8;a=a/(1+a)
 return Image.fromarray((np.clip(a,0,1)**(1/2.2)*255).astype('uint8'))
for label,roi in rois.items():
 fs=sorted((root/label).glob('rr-input*/'))
 for n in names:
  imgs=[im(crop(read(f,n),roi),n) for f in fs]
  out=Image.new('RGB',(800,430));d=ImageDraw.Draw(out)
  for i,img in enumerate(imgs):out.paste(img.resize((400,400)),(i*400,30));d.text((i*400,8),f'{label} {fs[i].name} {n}',fill='white')
  out.save(root/label/(n+'.png'))
