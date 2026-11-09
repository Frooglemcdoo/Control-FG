"""Verify the supported native DLL layout; does not modify the DLL. Python stdlib."""
from pathlib import Path
import hashlib,struct,json,sys
p=Path(sys.argv[1]);data=p.read_bytes();digest=hashlib.sha256(data).hexdigest()
assert digest=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5','Unsupported native DLL'
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0]
sections=[]
for i in range(count):
 offset=pe+24+optional+40*i
 virtual_size,virtual_address,raw_size,raw_offset=struct.unpack_from('<IIII',data,offset+8)
 sections.append((virtual_address,raw_size,raw_offset))
def at(rva,size):
 for va,n,offset in sections:
  if va<=rva and rva+size<=va+n:return data[offset+rva-va:offset+rva-va+size]
 raise ValueError('RVA outside raw sections')
checks={
 'auto_backbuffer':(0x317d0,'488b0d5155100048638138010000488b84c140010000c3'),
 'present_native_target':(0x31c20,'488b050151100048638838010000488b8cc840010000e8258a0000'),
 'present_swapchain_call':(0x31cd2,'488b8978010000488b018bd7ff5040'),
 'advance_after_present':(0x31db2,'488b0d6f4f10008b8138010000ffc025010000807d07ffc883c8feffc0898138010000'),
 'barrier_native_resource':(0x3a691,'488b81880000004889442430')}
for name,(rva,hexbytes) in checks.items():assert at(rva,len(bytes.fromhex(hexbytes)))==bytes.fromhex(hexbytes),name
print(json.dumps({'status':'PASS','sha256':digest,'checks':{k:hex(v[0]) for k,v in checks.items()},'interpretation':'Native target device+0x138 -> device+0x140+index*8 -> texture+0x88, transitioned before swapchain Present and index incremented after return. Source identity is read inside wrapper Present.'},indent=2))
