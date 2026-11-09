"""Measure spatial bright samples in recovered same-frame GI/input boundaries.
Thresholds describe contrast only, not estimator correctness or temporal variance.
"""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from scipy.ndimage import median_filter,binary_erosion


def audit(path):
 m=json.loads((path/'metadata.json').read_text());h,w=m['height'],m['width'];hashes={}
 if not m['pre_gi_captured'] or m['pre_gi_frame']!=m['frame'] or not m['gi_boundary']['valid'] or m['gi_boundary']['frame']!=m['frame']:
  raise ValueError('unmatched GI boundary')
 def read(name):
  b=(path/name).read_bytes()
  if len(b)!=h*w*8:raise ValueError('invalid extent: '+name)
  hashes[name]=hashlib.sha256(b).hexdigest();a=np.frombuffer(b,'<f2').reshape(h,w,4).astype('f4')
  if not np.isfinite(a).all():raise ValueError('nonfinite: '+name)
  return a
 pre=read('pre-filter-gi.rgba16f');after=read('gi-at-rr-input.rgba16f');color=read('input-color.rgba16f');raw=read('raw-albedo.rgba16f')
 luma=np.array([.2126,.7152,.0722],dtype='f4');g=pre[...,:3]@luma;c=color[...,:3]@luma
 gm=median_filter(g,size=5,mode='nearest');cm=median_filter(c,size=5,mode='nearest')
 # Neighborhood-relative candidates, deliberately not called erroneous fireflies.
 bright=(g>8*np.maximum(gm,1e-4))&(g>1e-3)
 valid=np.all((raw[...,:3]>=0)&(raw[...,:3]<=1),axis=2)
 hair=valid&(raw[...,3]>=-3)&(raw[...,3]<=-2)
 hair_interior=binary_erosion(hair,structure=np.ones((5,5),bool),border_value=0)
 regions={'whole_image':np.ones((h,w),bool),'tagged_hair_interior':hair_interior}
 results={}
 for name,mask in regions.items():
  candidates=bright&mask;ys,xs=np.where(candidates)
  score=g[ys,xs]/np.maximum(gm[ys,xs],1e-4);order=np.argsort(score)[-12:][::-1]
  results[name]={'eligible_pixels':int(mask.sum()),'gi_bright_candidates':int(candidates.sum()),'also_input_color_over_2x_median':int((candidates&(c>2*np.maximum(cm,1e-4))).sum()),'examples':[{'x':int(xs[i]),'y':int(ys[i]),'gi_luminance':float(g[ys[i],xs[i]]),'gi_neighbor_median':float(gm[ys[i],xs[i]]),'input_luminance':float(c[ys[i],xs[i]]),'input_neighbor_median':float(cm[ys[i],xs[i]])} for i in order]}
 return {'frame':m['frame'],'extent':[w,h],'stage':m['gi_boundary']['stage'],'gi_rgb_exactly_preserved':bool(np.array_equal(pre[...,:3],after[...,:3])),'regions':results,'hashes':hashes}

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 paths=sorted(a.root.glob('stationary-pair-*/*/metadata.json'))
 if not paths:p.error('no recovered captures')
 report={'schema':'gi-signal-origin-v1','thresholds':{'median_window':5,'gi_ratio':8,'gi_minimum':.001,'denominator_floor':.0001,'input_color_ratio':2},'frames':[audit(p.parent) for p in paths],'limitations':['Older R271 captures; not a controlled lamp-on/off pair.','Bright candidates are spatial contrast measurements, not proof of invalid lighting.','Hair mask is replay-tagged interior; excludes edges and partially tagged neighborhoods.','No per-pixel comparison with differently sized RR output or across jittered frames.','Cannot attribute candidates to a named light or compare with a different-session sparse ray trace.']}
 a.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
 for f in report['frames']:print(f['frame'],f['gi_rgb_exactly_preserved'],{k:{x:y for x,y in v.items() if x!='examples'} for k,v in f['regions'].items()})
