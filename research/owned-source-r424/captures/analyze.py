from pathlib import Path
import json,numpy as np
from scipy.ndimage import median_filter
import argparse
parser=argparse.ArgumentParser(description="Analyze extracted R424 hair/jaw captures; writes findings beside inputs.")
parser.add_argument("capture_root",type=Path)
root=parser.parse_args().capture_root
L=np.array([.2126,.7152,.0722])
for label in ['hair','jaw']:
 folder=root/label;report=json.loads((folder/'boundary-report.json').read_text());roi=report['source_join']['metadata']['roi'];x,y,w,h=roi
 frames=sorted((folder/'full-screen-rr').iterdir());m=json.loads((frames[0]/'metadata.json').read_text())
 def read(n,idx=0):
  meta=json.loads((frames[idx]/'metadata.json').read_text());r=next(r for r in meta['resources'] if r['file'].startswith(n+'.'))
  dtype='u1' if r['file'].endswith('rgba8') else '<u4' if r['file'].endswith('r32u') else '<f4' if r['file'].endswith('r32f') else '<f2'
  return np.fromfile(frames[idx]/r['file'],dtype=dtype).reshape(r['height'],r['width'],-1)
 b=next((folder/'logs').glob('*.bin'));u=np.fromfile(b,dtype='<u4',offset=16).reshape(-1,64);f=u.view('<f4');s=u[:,3]==2;r=u[:,3]==1
 xy=u[s,:2];localxy=xy-[x,y];a=np.zeros((h,w,64),np.float32);a[localxy[:,1],localxy[:,0]]=f[s]
 ray=np.zeros_like(a);xyR=u[r,:2]-[x,y];ray[xyR[:,1],xyR[:,0]]=f[r]
 pre=read('pre-filter-gi')[y:y+h,x:x+w,:3].astype('float32');gi=read('gi-at-rr-input')[y:y+h,x:x+w,:3].astype('float32');color=read('input-color')[y:y+h,x:x+w,:3].astype('float32');diff=read('lighting-diffuse')[y:y+h,x:x+w,:3].astype('float32');albedo=read('diffuse')[y:y+h,x:x+w,:3].astype('float32');tags=read('raw-albedo')[y:y+h,x:x+w,3].astype('float32')
 print(label,'rays',np.unique(u[s,2],return_counts=True),flush=True)
 print('source_to_preGI',{'half_exact_rgb_fraction':float(np.mean(np.all(a[...,4:7].astype('f2')==pre.astype('f2'),axis=-1))),'max_abs':float(np.max(abs(a[...,4:7]-pre))),'raw_to_rr_exact':bool(np.array_equal(pre,gi))},flush=True)
 stats={'frame':m['frame'],'roi':roi,'rays':np.unique(u[s,2]).tolist(),'source_pre_half_match':float(np.mean(np.all(a[...,4:7].astype('f2')==pre.astype('f2'),axis=-1)))}
 masks={'whole':np.ones((h,w),bool),'tag_positive':tags>0,'tag_negative':tags<0}
 if label=='jaw':
  yy,xx=np.mgrid[y:y+h,x:x+w];masks['jaw_neck_box']=(xx>=1045)&(xx<1155)&(yy>=700)&(yy<790)&(tags>0)
 else:
  raw=read('raw-albedo')[y:y+h,x:x+w,:3]
  masks['hair_inner']=(tags>=-3)&(tags<=-2)&np.isfinite(raw).all(-1)&(raw>=0).all(-1)&(raw<=1).all(-1)
 lum=a[...,4:7]@L;med=median_filter(lum,size=5);cres=color[...,0]-median_filter(color[...,0],size=5);ratio=lum/np.maximum(med,1e-8)
 stats['regions']={}
 for key,mask in masks.items():
  if not mask.any():continue
  top=mask&(lum>=np.percentile(lum[mask],99));totals=[float((a[...,off:off+3]@L)[top].sum()) for off in [16,20,24]]
  print(key,'pixels',int(mask.sum()),'luma pct',np.percentile(lum[mask],[50,90,99,100]).tolist(),'top1component fractions',(np.array(totals)/lum[top].sum()).tolist(),flush=True)
  order=np.argsort(np.where(mask,cres,-np.inf).ravel())[-10:][::-1];details=[]
  for index in order[:5]:
   j,i=np.unravel_index(index,(h,w));rec={'pixel':[x+int(i),y+int(j)],'red_residual':float(cres[j,i]),'source_rgb':a[j,i,4:7].tolist(),'pre_gi':pre[j,i].tolist(),'ratio_local_median':float(ratio[j,i]),'input':color[j,i].tolist(),'diffuse_times_albedo':(diff[j,i]*albedo[j,i]).tolist(),'volume':a[j,i,16:19].tolist(),'point':a[j,i,20:23].tolist(),'spot':a[j,i,24:27].tolist(),'secondary_material':int(ray[j,i].view('u4')[35]),'distance':float(ray[j,i,15]),'hit_world':ray[j,i,32:35].tolist(),'primary_origin':ray[j,i,48:51].tolist(),'branch':int(ray[j,i].view('u4')[19])};details.append(rec)
  stats['regions'][key]={'pixels':int(mask.sum()),'top1percent_component_luminance_fractions':(np.array(totals)/lum[top].sum()).tolist(),'red_peaks':details}
  if key in ['hair_inner','jaw_neck_box']:
   for p in details[:3]:print('peak',p['pixel'],'ratio',p['ratio_local_median'],'spot',p['spot'],'point',p['point'],'material',p['secondary_material'],'distance',p['distance'],flush=True)
 np.savez_compressed(root/f'{label}-trace-arrays.npz',summary=a,ray=ray,pre=pre,color=color,tags=tags)
 (root/f'{label}-findings.json').write_text(json.dumps(stats,indent=2)+'\n')
