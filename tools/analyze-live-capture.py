"""Inspect live output bytes; black pixels are NOT a coverage mask."""
from pathlib import Path
import json,sys,hashlib
import numpy as np
from PIL import Image,ImageDraw
root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
m=json.loads((root/'metadata.json').read_text());assert m['schema']=='control-rr-live-g15'
w,h=m['width'],m['height'];assert isinstance(w,int) and isinstance(h,int) and 0<w<=8192 and 0<h<=8192 and w*h<=8388608
arrays={};hashes={}
for name,dtype in [('normal-roughness.rgba32f','<f4'),('specular.rgba32f','<f4'),('diffuse.rgba16f','<f2')]:
 p=root/name;assert p.stat().st_size==w*h*4*np.dtype(dtype).itemsize,name
 raw=p.read_bytes();hashes[name]=hashlib.sha256(raw).hexdigest();arrays[name]=np.frombuffer(raw,dtype=dtype).reshape(h,w,4).astype(np.float32)
n=arrays['normal-roughness.rgba32f'];s=arrays['specular.rgba32f'];d=arrays['diffuse.rgba16f']
report={'frame':m['frame'],'width':w,'height':h,'jitter':[m['jitter_x'],m['jitter_y']],'pixels':w*h,
 'nonfinite_normal_components':int((~np.isfinite(n)).sum()),'nonfinite_specular_components':int((~np.isfinite(s)).sum()),'nonfinite_diffuse_components':int((~np.isfinite(d)).sum()),
 'normal_length_outside_0_99_to_1_01':int((np.abs(np.linalg.norm(n[:,:,:3].astype(np.float64),axis=2)-1)>.01).sum()),
 'roughness_outside_0_to_1':int(((n[:,:,3]<0)|(n[:,:,3]>1)).sum()),'diffuse_alpha_not_one':int((d[:,:,3]!=1).sum()),
 'specular_alpha_zero':int((s[:,:,3]==0).sum()),'specular_alpha_other_than_zero_or_one':int(((s[:,:,3]!=0)&(s[:,:,3]!=1)).sum()),
 'diffuse_rgb_all_zero':int((d[:,:,:3]==0).all(axis=2).sum()),'accepted_batches':m['accepted_batches'],'rejected_batches':m['rejected_batches'],
 'coverage_proven':False,'pixel_reference_comparison':'NOT_AVAILABLE: native inputs were not captured by this live-output capture','sha256':hashes}
(out/'statistics.json').write_text(json.dumps(report,indent=2)+'\n')
thumbw=960;thumbh=max(1,round(h*thumbw/w));canvas=Image.new('RGB',(thumbw*2,(thumbh+32)*2),(20,20,20));draw=ImageDraw.Draw(canvas)
views=[('Diffuse (display gamma only)',np.clip(d[:,:,:3],0,1)**(1/2.2)),('World normals',n[:,:,:3]*.5+.5),('Specular (display gamma only)',np.clip(s[:,:,:3],0,1)**(1/2.2)),('Roughness',np.repeat(n[:,:,3:4],3,axis=2))]
for i,(label,data) in enumerate(views):
 rgb=(np.clip(np.nan_to_num(data),0,1)*255).astype('uint8');im=Image.fromarray(rgb).resize((thumbw,thumbh));x=(i%2)*thumbw;y=(i//2)*(thumbh+32);draw.text((x+8,y+8),label,fill='white');canvas.paste(im,(x,y+32))
canvas.save(out/'live-overview.png');print(json.dumps(report,indent=2))
