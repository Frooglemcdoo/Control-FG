from pathlib import Path
import hashlib,json,sys
from pe_tools import PE
root=Path(__file__).resolve().parents[1]
p=Path(sys.argv[1])
want='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
assert hashlib.sha256(p.read_bytes()).hexdigest()==want
pe=PE(p)
checks=[(0x261a0,'48890e'),(0x261ab,'ff5058'),(0x261b3,'48894648'),
        (0x261ba,'4c896628'),(0x261be,'48895e30'),(0x26101,'894620'),
        (0x26b84,'4d8b4d00'),(0x26b8c,'498b1424'),(0x26b93,'41ff5278')]
for rva,hexbytes in checks:
 assert pe.data[pe.off(rva):pe.off(rva)+len(hexbytes)//2].hex()==hexbytes,hex(rva)
ranges=[(0x26101,0x261c6),(0x26ad6,0x26b97),(0x25899,0x258f6),(0x25d1a,0x25d52),(0x269c2,0x269fc)]
report={'status':'PASS','dll_sha256':want,'checks':[{'rva':hex(a),'bytes':b} for a,b in checks],
 'conclusion':'NativeBuffer+0 is resource, +0x48 GPU VA, +0x20 native state bookkeeping. Mode2 uses separate staging allocation.',
 'limits':'No live identity, state transition, CPU data freshness, or GPU lifetime proof.',
 'excerpts':[{'rva':hex(a),'text':pe.disasm(a,b)} for a,b in ranges]}
(root/'validation/part1/native-resource-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: hash-locked native resource and mode2 path instruction audit')
