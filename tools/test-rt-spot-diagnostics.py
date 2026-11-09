"""Validate raw spotlight output patches and preserve point-lane operations."""
import base64
import struct
import sys
from pathlib import Path
from dxbc_checksum import checksum
from rt_spot_diagnostics import raw_history, raw_spatial
from rt_spot_filter import SPATIAL_SHAS

root = Path(__file__).resolve().parents[1]
def read(name):
    data=base64.b64decode((root/'assets/rt-spot'/name).read_text())
    assert data[4:20]==checksum(data)
    offsets=struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32)
    offset=next(x for x in offsets if data[x:x+4]==b'SHEX')
    size=struct.unpack_from('<I',data,offset+4)[0]
    return data,struct.unpack_from('<'+'I'*(size//4),data,offset+8)

data,words=read('rt_spot_raw_history.dxbc.b64')
assert words[316:321]==(0x05000036,0x001000d2,2,0x00100aa6,1)
for spot in [0.,.25,.75,1.]:
    for point in [0.,.25,1.]:
        for old in [0.,.5,1.]:
            lanes=[old,old,old,old];current=[0.,0.,spot,point]
            mask=(words[317]>>4)&15
            for lane in range(4):
                if mask&(1<<lane):lanes[lane]=current[(words[319]>>(4+2*lane))&3]
            assert lanes==[spot,old,spot,spot]
for i in range(2):
    spatial,w=read(f'rt_spot_raw_spatial{i}.dxbc.b64')
    assert w[1814:1819]==(0x05000036,0x00100012,1,0x00100006,4)
    assert w[1819]&2047==164
    # x-only MOV leaves point y exactly as computed by native filter.
    assert ((w[1815]>>4)&15)==1
    if len(sys.argv)>1:
        native=Path(sys.argv[1])/f'compute_cs-{SPATIAL_SHAS[i]}.dxbc'
        assert spatial==raw_spatial(native.read_bytes())
if len(sys.argv)>1:
    native=Path(sys.argv[1])/'compute_cs-feb37637481fed22a3cd4d256da613d1f61aec70fc11ff5e42aa2a585101fedb.dxbc'
    assert data==raw_history(native.read_bytes())
print('PASS shipped raw spotlight patches: current visibility output, point lanes preserved')
