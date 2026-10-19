"""Validate R39 capture call sites against the supplied native DLLs."""
import argparse
import hashlib
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'research/native-integration-audit/tools'))
from pe_tools import PE

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('renderer', type=Path)
p.add_argument('d3d', type=Path)
a = p.parse_args()
r, d = PE(a.renderer), PE(a.d3d)
assert hashlib.sha256(r.data).hexdigest() == 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
assert hashlib.sha256(d.data).hexdigest() == 'cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
sites = [(0x314ae, 0x5dfda0, '?unlock@DynamicBuffer@d3d@@SAXAEBUSlot@12@_K@Z', 0x215c0)]
sites += [(at, 0x5dfe80, '?unlock@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@PEAX@Z', 0x26010)
          for at in (0x124092, 0x1240ab, 0x1240bc, 0x1240d5, 0x1240e6, 0xa8998)]
for at, slot, name, target in sites:
    raw = r.data[r.off(at):r.off(at)+6]
    assert raw[:2] == b'\xff\x15'
    assert at + 6 + struct.unpack_from('<i', raw, 2)[0] == slot
    assert r.imports[slot] == name
    assert d.exports[name] == target
    print(f'{at:08x}: {raw.hex()} -> IAT {slot:08x} -> D3D {target:08x}')
print('All seven exact native upload call sites verified.')
