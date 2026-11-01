from pathlib import Path
import json,numpy as np
from PIL import Image,ImageDraw
import argparse
parser=argparse.ArgumentParser(description='Measure the R263 stationary projector capture. Fixed scene ROIs; not a general noise-quality metric.')
parser.add_argument('capture',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
root=args.capture
frames=sorted(root.glob('stationary-pair-*/*/metadata.json'))
def load(p,role):
 m=json.loads(p.read_text());r=next(r for r in m['resources'] if r['file'].startswith(role+'.'));f=r['dxgi_format'];dt,ch=('<f2',4) if f==10 else ('<f2',2) if f==34 else ('<f4',1) if f in (39,41) else ('u1',4)
 return np.memmap(p.parent/r['file'],dtype=dt,mode='r',shape=(r['height'],r['width'],ch))
def display(a,exposure=1):
 a=np.maximum(a[...,:3].astype('f4')*exposure,0);a=a/(1+a);a=np.power(a,1/2.2);return Image.fromarray((np.clip(a,0,1)*255).astype('u1'))

from scipy.ndimage import gaussian_filter
rois={'floor_near_projector':(.48,.46,.53,.62),'floor_left':(.23,.57,.34,.69),'pedestal':(.55,.53,.62,.62),'board':(.74,.24,.79,.29)}
roles=['pre-filter-gi','input-color','rr-output','lighting-diffuse','lighting-specular']
report=[]
def crop(a,box):
 h,w=a.shape[:2];x0,y0,x1,y1=box;return a[int(h*y0):int(h*y1),int(w*x0):int(w*x1)].astype('f4')
for p in frames:
 m=json.loads(p.read_text());out={'frame':m['frame'],'tick':m['evaluation_tick_ms'],'exposure':m['pre_exposure'],'reset':m['reset'],'rois':{}}
 for name,box in rois.items():
  stats={}
  for role in roles:
   a=crop(load(p,role),box)[...,:3];lum=a@np.array([.2126,.7152,.0722]);hp=lum-gaussian_filter(lum,1.5 if role=='rr-output' else 1)
   stats[role]={'mean':float(lum.mean()),'hp_rms':float(np.sqrt(np.mean(hp**2))),'max':float(lum.max())}
  mv=crop(load(p,'input-motion'),box)*[2560,1440];raw=crop(load(p,'raw-albedo'),box);nr=crop(load(p,'normal-roughness'),box);g1=crop(load(p,'native-gbuffer1'),box).astype('u4')
  nx=(g1[...,0]*8+((g1[...,3]>>1)&7))/2047.;ny=(g1[...,1]*16+(g1[...,3]>>4))/4095.;qx=1.3*(2*nx-1);qy=1.3*(2*ny-1);q2=qx*qx+qy*qy;expected=np.stack([2*qx/(q2+1),2*qy/(q2+1),(q2-1)/(q2+1)],-1)
  stats.update(motion_max_pixels=float(abs(mv).max()),raw_missing_fraction=float(np.mean((raw[...,:3]<0).any(-1))),normal_max_error=float(abs(nr[...,:3]-expected).max()),roughness_max_error=float(abs(nr[...,3]-(1-g1[...,2]/255.)).max()),roughness_range=[float(nr[...,3].min()),float(nr[...,3].max())])
  out['rois'][name]=stats
 report.append(out)
(args.output/'metrics.json').write_text(json.dumps(report,indent=2))
canvas=Image.new('RGB',(1500,4*290));d=ImageDraw.Draw(canvas)
for row,p in enumerate(frames[::2]):
 for col,role in enumerate(roles):
  a=crop(load(p,role),rois['floor_near_projector']);im=display(a,80);im.thumbnail((290,260));canvas.paste(im,(col*300,row*290+24));d.text((col*300+4,row*290+4),f'{json.loads(p.read_text())["frame"]} {role}',fill='white')
canvas.save(args.output/'floor-sequence.jpg')
# Absolute equality applies only to RGB: the bypass preserves the native RGB/red-alpha ABI.
boundaries=[]
for p in frames:
 m=json.loads(p.read_text());a=load(p,'pre-filter-gi')[...,:3];b=load(p,'gi-at-rr-input')[...,:3]
 boundaries.append({'frame':m['frame'],'early_late_rgb_equal':bool(np.array_equal(a,b)),
  'early_gi_nonfinite':int((~np.isfinite(a)).sum()),'jitter':m['jitter'],
  'rr_success':m['rr_result']==1,'reset':m['reset'],'boundary':m['gi_boundary']})
(args.output/'boundaries.json').write_text(json.dumps(boundaries,indent=2)+'\n')
# Screen-space correlations are descriptive, not a reprojection correctness test.
correlations={}
for role in ['pre-filter-gi','input-color','rr-output']:
 arrays=[crop(load(p,role),rois['floor_near_projector'])[...,:3]@np.array([.2126,.7152,.0722]) for p in frames]
 hp=[a-gaussian_filter(a,3 if role=='rr-output' else 2) for a in arrays]
 correlations[role]=[{'frames':[json.loads(frames[i].read_text())['frame'],json.loads(frames[j].read_text())['frame']],
  'highpass_correlation':float(np.corrcoef(hp[i].ravel(),hp[j].ravel())[0,1])}
  for i,j in [(0,1),(2,3),(4,5),(6,7),(0,2),(2,4),(4,6),(0,6)]]
(args.output/'correlations.json').write_text(json.dumps(correlations,indent=2)+'\n')
print('Analyzed',len(frames),'frames; results:',args.output)
