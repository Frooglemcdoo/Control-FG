"""Replay emitted R421/R423 filter on captured RR-boundary inputs, sample-zero assumption.
This does not reproduce per-draw resource timing or RR history/output.
"""
import ast,json,sys
from pathlib import Path
import numpy as np
repo=Path(__file__).resolve().parents[2];sys.path.insert(0,str(repo/'tools'))
from character_diffuse import block,opnames
from character_surface import src
module=ast.parse((repo/'tools/test-character-diffuse.py').read_text());ns={'np':np,'opnames':opnames}
for name in ('execute','normal','mapped','oracle'):
 fun=next(n for n in module.body if isinstance(n,ast.FunctionDef) and n.name==name)
 exec(compile(ast.Module(body=[fun],type_ignores=[]),'interpreter','exec'),ns)
code=block(16,0,src(1),[0x101e46,0],0)
for frame,points in [(12238,[(347,632),(304,815),(850,1007)]),(12239,[(295,659),(312,818),(858,1001)])]:
 f=next(Path(sys.argv[1]).glob(f'rr-input-*-{frame}'));m=json.loads((f/'metadata.json').read_text())
 def read(n,dtype):
  r=next(r for r in m['resources'] if r['file'].startswith(n+'.'))
  return np.fromfile(f/r['file'],dtype).reshape(r['height'],r['width'],-1)
 resources={0:read('native-gbuffer1','u1').astype(np.float32)/255,1:read('native-gbuffer2','u1').astype(np.float32)/255,3:read('lighting-diffuse','<f2').astype(np.float32),11:np.tile(read('native-reconstruction-map','<u4'),(1,1,4)).view(np.float32)}
 for x,y in points:
  regs=np.zeros((64,4),np.float32);regs[0,:3]=resources[3][y,x,:3];regs[1]=np.array([x,y,0,0],np.uint32).view(np.float32)
  result,reads=ns['execute'](code,regs,(x+.5,y+.5),resources,0,1)
  expected=ns['oracle'](resources,(x,y),(x,y),1)
  assert np.allclose(result[0,:3],expected,rtol=1e-5,atol=1e-6)
  c=regs[0,:3];eligible=[(sx,sy) for slot,sx,sy in reads if slot==3]
  print(frame,(x,y),'input',c.tolist(),'filter_output',result[0,:3].tolist(),'material_compatible_neighbor_reads',len(eligible))
  weights=[]
  for sx,sy in eligible:
   tap=resources[3][sy,sx,:3];v=np.clip(1-np.max(abs(tap-c)/(.5*np.maximum(abs(tap),abs(c))+.0001)),0,1)
   weights.append(float(v))
  print('radiance_gate_weights',weights)
