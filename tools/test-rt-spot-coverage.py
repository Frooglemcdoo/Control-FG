"""Check the shipped coverage instructions and single-application visibility."""
import base64
import math
import struct
from pathlib import Path
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'assets/rt-spot/rt_spot_light.dxbc.b64').read_text())
assert data[4:20]==checksum(data)
count=struct.unpack_from('<I',data,28)[0]
offset=next(o for o in struct.unpack_from('<'+'I'*count,data,32) if data[o:o+4]==b'SHEX')
size=struct.unpack_from('<I',data,offset+4)[0]
w=struct.unpack_from('<'+'I'*(size//4),data,offset+8)
assert w[1707]&2047==55 # Native map selection retained.
assert w[1716:1723]==(0x07000033,0x00100012,0,0x00100006,0,0x00100aa6,7)
assert w[1723:1728]==(0x05000036,0x00100042,7,0x4001,0x3f800000)
assert w[1728]&2047==57 # Native zero gate follows combined coverage.
# Execute the two emitted instructions over the full visibility range.
for native in (0,.1,.25,.5,.75,1):
    for rt in (0,.1,.25,.5,.75,1):
        registers={0:[native,0,0,0],7:[0,0,rt,.37]}
        for pos in (1716,1723):
            op=w[pos]&2047;mask=(w[pos+1]>>4)&15;dst=w[pos+2]
            if op==51:
                value=min(registers[w[pos+4]][(w[pos+3]>>4)&3],registers[w[pos+6]][(w[pos+5]>>4)&3])
            elif op==54:value=struct.unpack('<f',struct.pack('<I',w[pos+4]))[0]
            else:raise AssertionError(op)
            for c in range(4):
                if mask&(1<<c):registers[dst][c]=value
        final=registers[0][0]*registers[7][2]
        assert math.isclose(final,min(native,rt))
        assert registers[7][3]==.37 # Point visibility untouched.
        if native==rt:assert final==native # No double darkening.
print('PASS shipped coverage merge: native-only, RT-only, overlap, lit, dark and point preservation')
