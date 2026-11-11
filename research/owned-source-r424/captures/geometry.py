from pathlib import Path
import json,numpy as np
import argparse
parser=argparse.ArgumentParser();parser.add_argument("capture_root",type=Path)
root=parser.parse_args().capture_root
for label in ['hair','jaw']:
 d=np.load(root/f'{label}-trace-arrays.npz');s=d['summary'];r=d['ray'];pre=d['pre'];tags=d['tags'];findings=json.loads((root/f'{label}-findings.json').read_text());x,y,w,h=findings['roi'];folder=sorted((root/label/'full-screen-rr').iterdir())[0];g=np.fromfile(folder/'native-gbuffer2.rgba8','u1').reshape(1440,2560,4);ids=(g[...,2].astype('u4')<<8)|g[...,3]
 nearest=s[...,4:7].astype('f2');ulp=np.abs(nearest.view('u2').astype('i4')-pre.astype('f2').view('u2').astype('i4'));print(label,'ULPmax',ulp.max(),'fraction_le1',np.mean(ulp<=1),'source>=pre',np.mean(s[...,4:7]>=pre))
 reg='hair_inner' if label=='hair' else 'jaw_neck_box'
 for p in findings['regions'][reg]['red_peaks']:
  X,Y=p['pixel'];rr=r[Y-y,X-x];n=rr[52:55];sn=rr[36:39];v=rr[32:35]-rr[48:51];v/=np.linalg.norm(v)
  print(p['pixel'],'primarymaterial',ids[Y,X],'hitmaterial',rr.view('u4')[35],'cosprim',np.dot(n,v),'coshit',np.dot(sn,-v),'Nlengths',np.linalg.norm(n),np.linalg.norm(sn),'tag',tags[Y-y,X-x],'componentFraction', np.array([s[Y-y,X-x,o:o+3]@np.array([.2126,.7152,.0722]) for o in [16,20,24]])/(s[Y-y,X-x,4:7]@np.array([.2126,.7152,.0722])))
