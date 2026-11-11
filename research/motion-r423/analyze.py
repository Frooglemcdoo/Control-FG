"""Inspect R423 map and character lighting; no inference of per-draw map lifetime."""
import sys,json,hashlib
from pathlib import Path
import numpy as np
from scipy.ndimage import median_filter
root=Path(sys.argv[1]);rois={'hair':(210,470,600,720),'neck':(300,815,470,900),'arm':(790,960,870,1010)}
for f in sorted(p for p in root.glob('rr-input*') if p.is_dir()):
 m=json.loads((f/'metadata.json').read_text())
 def read(name):
  r=next(r for r in m['resources'] if r['file'].split('.')[0]==name)
  dtype='<u4' if r['file'].endswith('r32u') else '<f4' if r['file'].endswith('r32f') else 'u1' if r['file'].endswith('rgba8') else '<f2'
  return np.memmap(f/r['file'],dtype=dtype,mode='r').reshape(r['height'],r['width'],-1)
 a={n:read(n) for n in ['input-color','lighting-diffuse','lighting-specular','pre-filter-gi','gi-at-rr-input','diffuse','specular','normal-roughness','raw-albedo','native-reconstruction-map','input-motion']}
 p=a['native-reconstruction-map'][...,0]; unique,counts=np.unique(p,return_counts=True)
 print('FRAME',m['frame'],'map',[(hex(int(v)),int(c)) for v,c in zip(unique,counts)])
 for parity in range(4):
  shift=(parity&1)*8+(parity>>1)*16
  print('parity',parity,'sample0 selection',np.unique((p>>shift)&3).tolist(),'zero-coverage sentinel',np.unique((p>>((shift-2)&31))&3).tolist())
 print('GI exact RGB match',np.array_equal(a['pre-filter-gi'][...,:3],a['gi-at-rr-input'][...,:3]))
 for label,(x,y,X,Y) in rois.items():
  print('ROI',label,(x,y,X,Y))
  c={n:v[y:Y,x:X].astype('float32') for n,v in a.items() if n!='native-reconstruction-map'}
  residual={n:v[...,:3]-median_filter(v[...,:3],size=(5,5,1)) for n,v in c.items() if v.shape[-1]>=3}
  peak=np.unravel_index(residual['input-color'][...,0].argmax(),(Y-y,X-x))
  print('red_peak',x+peak[1],y+peak[0])
  for n in ['input-color','lighting-diffuse','lighting-specular','pre-filter-gi','diffuse','specular','normal-roughness','raw-albedo']:print(n,c[n][peak].tolist())
  print('diffuse*albedo/input', (c['lighting-diffuse'][peak][:3]*c['diffuse'][peak][:3]/np.maximum(c['input-color'][peak][:3],1e-10)).tolist())
  gi=c['pre-filter-gi'][...,:3]@np.array([.2126,.7152,.0722]);med=median_filter(gi,size=5)
  print('GI lum peak ratio',float(gi[peak]/max(med[peak],1e-10)))
  for n in ['pre-filter-gi','lighting-diffuse','lighting-specular']:
   print('red residual correlation',n,float(np.corrcoef(residual[n][...,0].ravel(),residual['input-color'][...,0].ravel())[0,1]))
  print('motion pixels p50/p95',np.percentile(np.linalg.norm(c['input-motion']*np.array(m['motion_scale']),axis=-1),[50,95]).tolist())
