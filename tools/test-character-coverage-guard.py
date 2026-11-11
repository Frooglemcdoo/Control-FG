"""Offline candidate validates native coverage before existing reconstruction."""
import ast, base64, json, tempfile
from pathlib import Path
import numpy as np
from character_surface import root, decode, words
from character_diffuse import opnames, block
from character_surface import src
from character_coverage_guard import guarded_diffuse

source = Path(__file__).with_name('test-character-diffuse.py')
function = next(n for n in ast.parse(source.read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='execute')
namespace = {'np':np,'opnames':opnames}
exec(compile(ast.Module(body=[function],type_ignores=[]),str(source),'exec'),namespace)
execute = namespace['execute']
checks=0
with tempfile.TemporaryDirectory() as temporary:
    for family in json.loads((root/'research/character-surface-r346/families.json').read_text()):
        data=base64.b64decode((root/'research/character-surface-r346'/f"{family['name']}.dxbc.b64").read_bytes())
        p=Path(temporary)/'native.dxbc';p.write_bytes(data);info=decode(p)
        guarded,meta=guarded_diffuse(data,info)
        q=Path(temporary)/'candidate.dxbc';q.write_bytes(guarded);candidate=decode(q)
        assert info['bindings']==candidate['bindings'] and info['constant_buffers']==candidate['constant_buffers']
        temp=meta['temporary']
        output=0;coord=1
        code=meta['guard']+block(temp-13,output,src(coord),[0x101e46,0],0)+meta['end']
        resources={0:np.tile(np.array([.5,.5,.5,128/255],np.float32),(10,10,1)),1:np.tile(np.array([.2,.3,10/255,20/255],np.float32),(10,10,1)),3:np.full((10,10,4),.8,np.float32),11:np.zeros((5,5,4),np.float32)}
        resources[3][4,4,:3]=1
        for selector in (0,1,0xffffffff):
            regs=np.zeros((128,4),np.float32);regs[0,:3]=1
            regs[coord]=np.array([4,4,0,0],np.uint32).view(np.float32)
            regs[temp,0]=np.uint32(selector).view(np.float32)
            result,reads=execute(code,regs,(4.5,4.5),resources,0,1)
            if selector==0:
                assert len([r for r in reads if r[0]==3])==8
                assert np.all(result[0,:3]<.95)
            else:
                assert not reads and np.array_equal(result[0],regs[0])
            checks+=1
print(f'PASS {checks} emitted guard cases; 12 exact reversible candidate transforms and unchanged resource bindings')
print('Offline only: production generator untouched; no GPU/PSO validation or in-game fix claim.')
