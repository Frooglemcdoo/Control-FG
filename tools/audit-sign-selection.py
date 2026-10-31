"""Validate targeted selection callsites against the supplied Steam renderer."""
import hashlib,struct,sys
from pathlib import Path
b=Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(b).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
pe=struct.unpack_from('<I',b,0x3c)[0];count=struct.unpack_from('<H',b,pe+6)[0];size=struct.unpack_from('<H',b,pe+20)[0];sections=pe+24+size
segments=[]
for i in range(count):
 vsize,rva,rawsize,raw=struct.unpack_from('<IIII',b,sections+i*40+8);segments.append((rva,max(vsize,rawsize),raw))
def data(rva,n):
 for start,size,raw in segments:
  if start<=rva and rva+n<=start+size:return b[raw+rva-start:raw+rva-start+n]
 raise ValueError(hex(rva))
for site,target in [(0x16923e,0x15a5e0),(0x15aaab,0x15ab30),(0x15aac2,0x15ab30)]:
 code=data(site,5);assert code[0]==0xe8 and site+5+struct.unpack_from('<i',code,1)[0]==target
 print(f'PASS call {site:x} -> {target:x}')
for site,expected in [(0x168f00,'4c89a980000000'),(0x1691c5,'4903fe'),(0x169255,'48ff8588000000'),(0x15aa00,'8993c4000000'),(0x15aa13,'4489bbc8000000'),(0x15aa20,'8993c0000000'),(0x15aa4d,'483d00009001'),(0x15aa90,'3b0572816a00752f')]:
 assert data(site,len(bytes.fromhex(expected))).hex()==expected
 print(f'PASS selection evidence {site:x}')
