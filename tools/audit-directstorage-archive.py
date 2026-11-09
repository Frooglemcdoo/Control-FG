"""Read-only ABI/callsite audit; input is the user's original runtime DLL."""
import argparse
import hashlib
import json
import struct
import pefile
import capstone

p = argparse.ArgumentParser()
p.add_argument('runtime')
args = p.parse_args()
data = open(args.runtime, 'rb').read()
assert hashlib.sha256(data).hexdigest() == 'a79234e4b34c2488deaac3459e8054be206b5af78a37723d834c1cbd826ce484'
pe = pefile.PE(data=data)
base = pe.OPTIONAL_HEADER.ImageBase
imports = {i.address-base: i.name for entry in pe.DIRECTORY_ENTRY_IMPORT for i in entry.imports}
assert pe.get_data(0x19671b,7).hex() == '48ffa090000000'
assert pe.get_data(0x1676e6,9).hex() == '65488b042558000000'
assert pe.get_data(0x196692,8).hex() == '4c8db4cfa0020000'
assert pe.get_data(0x1933f1, 6).hex() == 'ff1519bd0700'
assert 0x1933f7 + struct.unpack('<i', pe.get_data(0x1933f3, 4))[0] == 0x20f110
assert pe.get_data(0x19351c, 6).hex() == 'ff15eebb0700'
assert 0x193522 + struct.unpack('<i', pe.get_data(0x19351e, 4))[0] == 0x20f110
assert imports[0x20f110] == b'ReadFile'
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
points = [0x19668a,0x196692,0x1966b6,0x1966bc,0x1966d1,0x1966ef,0x19671b,0x1676e6,0x167701,0x167707,0x167722,0x167731,0x167739,0x1933dd, 0x1933e2, 0x1933e7, 0x1933ea, 0x1933ed, 0x1933f1, 0x193464, 0x193468, 0x1934e1, 0x1934e6, 0x1934eb, 0x1934f0, 0x1934f3, 0x1934f8,
          0x193503, 0x193508, 0x19350d, 0x193512, 0x193515, 0x193518,
          0x19351c, 0x19352e, 0x19358b, 0x19358f]
evidence = []
for rva in points:
    ins = next(cs.disasm(pe.get_data(rva, 15), rva))
    evidence.append({'rva': hex(rva), 'instruction': ins.mnemonic+' '+ins.op_str})
print(json.dumps({'runtime_sha256': hashlib.sha256(data).hexdigest(),
                  'scope': 'WinFileSystem async cache and synchronous observation only',
                  'native_offset': 'OVERLAPPED.Offset/OffsetHigh',
                  'native_event': 'FileEvent HANDLE, reset by caller before ReadFile',
                  'cache_rule': 'only completed successful raw archive bytes; misses use original ReadFile',
                  'retained_native_objects': [], 'evidence': evidence}, indent=2))
