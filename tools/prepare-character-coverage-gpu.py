"""Wrap the existing emitted GPU fixture with the candidate's exact guard."""
import numpy as np
from character_surface import root, decode, words, transform_shex, dst
from character_diffuse import ins, cb
from character_coverage_guard import guard_instructions

base_path=root/'build/diffuse-fixture-base.dxbc'
filtered_path=root/'build/diffuse-fixture-filtered.dxbc'
base=base_path.read_bytes();filtered=filtered_path.read_bytes()
bw=words(base);fw=words(filtered);info=decode(base_path)
load=next(i for i in info['instructions'] if i['name']=='ld' and i['operands'][-1].startswith('t[') and '[3].' in i['operands'][-1])
# Existing GPU preparation inserts a coordinate snapshot before the native load.
# Its fixed-length mov is outside the filter and must remain outside our guard.
at=load['dword']+(bw[load['dword']]>>24)
snapshot_length=5
start=at+snapshot_length
filter_length=len(fw)-len(bw)-snapshot_length
assert filter_length>0
fd=decode(filtered_path)
declaration=next(i['dword'] for i in fd['instructions'] if i['name']=='dcl_temps')
temporary=fw[declaration+1]
# Fixture F Position.z carries the saved native selector's uint bit pattern.
pre=ins('mov',dst(temporary,1),cb(0,3,0xaa))+guard_instructions(temporary)
post=ins('endif')
def patch(w):
    w[start+filter_length:start+filter_length]=post
    w[start:start]=pre
    w[declaration+1]+=1
filtered_path.write_bytes(transform_shex(filtered,patch))
for case in range(8):
    prefix=root/'build'/f'diffuse-fixture-{case}-'
    const=np.fromfile(str(prefix)+'cb.bin',np.float32)
    selector=0xffffffff if case in (1,5) else 1 if case==3 else 0
    const.view(np.uint32)[14]=selector
    const.tofile(str(prefix)+'cb.bin')
    if selector:
        x,y=const[12:14].astype(int)
        packed=np.fromfile(str(prefix)+'map.bin',np.uint32).reshape(5,5)[y//2,x//2]
        bits=(int(packed)>>((x%2)*8+(y%2)*16))&3
        sx,sy=x//2*2+(bits&1),y//2*2+(bits>>1)
        native=np.fromfile(str(prefix)+'diffuse.bin',np.float32).reshape(9,9,4)[sy,sx]
        native.tofile(str(prefix)+'expected.bin')
print('Prepared eight exact-guard WARP cases: valid, sentinel, unsupported selector and MSAA fallback')
