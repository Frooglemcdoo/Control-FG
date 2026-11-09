from pathlib import Path
import json,numpy as np
from scipy.ndimage import gaussian_filter
import argparse
parser=argparse.ArgumentParser(description='Audit restored plant/cabinet RR input captures; fixed normalized scene ROIs, no material-ID assumptions.')
parser.add_argument('capture',type=Path)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=args.capture
report=[]
for mode in ['clamp-only','both']:
 for p in sorted((root/mode).glob('*')):
  m=json.loads((p/'metadata.json').read_text());h,w=m['height'],m['width']
  def load(role):
   r=next(r for r in m['resources'] if r['file'].startswith(role+'.'));dtype='<f2' if r['dxgi_format']==10 else 'u1';return np.memmap(p/r['file'],dtype=dtype,mode='r',shape=(r['height'],r['width'],4))
  g1=load('native-gbuffer1');g2=load('native-gbuffer2');normal=load('normal-roughness');raw=load('raw-albedo');dif=load('diffuse')
  ids=g2[...,2].astype('u4')*256+g2[...,3];valid=((raw[...,:3]>=0)&(raw[...,:3]<=1)).all(-1)
  region=np.zeros((h,w),bool);region[int(h*.49):int(h*.76),int(w*.49):int(w*.62)]=1
  bad=region&~valid
  kinds,counts=np.unique(ids[bad],return_counts=True);order=np.argsort(-counts)
  materials=[]
  for i in order[:5]:
   mask=region&(ids==kinds[i]);values=dif[mask][:,:3].astype('f4')
   materials.append({'material_id':int(kinds[i]),'pixels':int(mask.sum()),'missing_replay':int((mask&~valid).sum()),'diffuse_rgb_min':values.min(0).tolist(),'diffuse_rgb_max':values.max(0).tolist()})
  # Decode the independently captured native packed normal at the exact same pixel.
  a=g1.astype('f4');nx=(g1[...,0].astype('u4')*8+((g1[...,3]>>1)&7))/2047.;ny=(g1[...,1].astype('u4')*16+(g1[...,3]>>4))/4095.
  qx=1.3*(2*nx-1);qy=1.3*(2*ny-1);q2=qx*qx+qy*qy
  expected=np.stack([2*qx/(q2+1),2*qy/(q2+1),(q2-1)/(q2+1)],-1)
  delta=np.abs(normal[...,:3].astype('f4')-expected)
  rough_delta=np.abs(normal[...,3].astype('f4')-(1-a[...,2]/255))
  surfaces={}
  for name,(x0,y0,x1,y1) in {'planter_flat':(.52,.76,.57,.85),'rear_cabinet_flat':(.47,.51,.495,.57)}.items():
   sl=np.s_[int(h*y0):int(h*y1),int(w*x0):int(w*x1),:]
   stats={}
   for role in ['raw-gi','lighting-diffuse','lighting-specular','input-color']:
    c=load(role)[sl].astype('f4');l=c[...,:3]@np.array([.2126,.7152,.0722]);hp=l-gaussian_filter(l,1)
    stats[role]={'mean':float(l.mean()),'spatial_highpass_rms':float(np.sqrt(np.mean(hp**2)))}
   rr=load('rr-output');hh,ww=rr.shape[:2];c=rr[int(hh*y0):int(hh*y1),int(ww*x0):int(ww*x1),:3].astype('f4');l=c@np.array([.2126,.7152,.0722]);hp=l-gaussian_filter(l,1.5);stats['rr-output']={'mean':float(l.mean()),'spatial_highpass_rms':float(np.sqrt(np.mean(hp**2)))}
   surfaces[name]=stats
  result={'mode':mode,'frame':m['frame'],'plant_region_materials':materials,'plant_normal_max_error':float(delta[region].max()),'plant_roughness_max_error':float(rough_delta[region].max()),'surface_metrics':surfaces}
  report.append(result)
args.output.write_text(json.dumps(report,indent=2)+'\n')
print('Audited',len(report),'frames; saved',args.output)
