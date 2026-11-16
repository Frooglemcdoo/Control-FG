"""Project dominant hits using captured native view positions, not fitted camera data."""
import json,sys
from pathlib import Path
import numpy as np
from scipy.ndimage import binary_erosion,median_filter
root=Path(sys.argv[1]);here=Path(__file__).parent;d=json.loads((here/'results.json').read_text());u=np.memmap(next((root/'logs').glob('*.bin')),'<u4','r',offset=16,shape=(262144,11,64));f=u.view('<f4');p=root/'full-screen-rr'/f'rr-input-c1-{d["metadata"]["pid"]}-{d["metadata"]["engine_frame"]}';m=json.loads((p/'metadata.json').read_text());P=np.array(m['view_to_clip']).reshape(4,4);lookup={tuple(xy):i for i,xy in enumerate(u[:,10,:2])};rows=[]
for e in d['examples']:
 r=f[lookup[tuple(e['pixel'])],e['dominant_ray']];direction=r[32:35]-r[48:51];distance=np.linalg.norm(direction);direction/=distance;clip=np.r_[r[40:43],1]@P;screen=(clip[:2]/clip[3]*[.5,-.5]+.5)*[m['width'],m['height']]
 rows.append({'pixel':e['pixel'],'secondary_projected_pixel':screen.tolist(),'secondary_in_view':bool(clip[3]>0 and 0<=screen[0]<m['width'] and 0<=screen[1]<m['height']),'world_distance':float(distance),'primary_cosine':float(direction@r[52:55]),'secondary_facing_cosine':float(-direction@r[36:39]),'secondary_normal_length':float(np.linalg.norm(r[36:39]))})
g=np.fromfile(p/'pre-filter-gi.rgba16f','<f2').reshape(m['height'],m['width'],4).astype(float);tag=np.fromfile(p/'raw-albedo.rgba16f','<f2').reshape(m['height'],m['width'],4)[:,:,3];lum=g[:,:,:3]@np.array([.2126,.7152,.0722]);med=median_filter(lum,size=5);mask=binary_erosion(tag==1,iterations=3);x,y,w,h=d['metadata']['roi'];roi=np.zeros_like(mask);roi[y:y+h,x:x+w]=True;mask&=roi
clamps={}
for cap in [2,4,10]:
 clipped=np.minimum(lum,cap*med);clamps[str(cap)]={'skin_gi_energy_removed_fraction':float((lum[mask]-clipped[mask]).sum()/lum[mask].sum()),'affected_pixels':int(np.sum((lum>cap*med)&mask))}
print(json.dumps({'geometry':rows,'clamp_loss':clamps},indent=2))
