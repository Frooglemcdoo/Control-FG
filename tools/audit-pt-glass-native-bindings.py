#!/usr/bin/env python3
"""Verify glass binding evidence against the user's native DLL; no DLL is committed."""
import argparse, hashlib, json, pathlib, sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'research/native-integration-audit/tools'))
from pe_tools import PE
p=argparse.ArgumentParser();p.add_argument('d3d_dll');args=p.parse_args()
native=PE(args.d3d_dll)
sha=hashlib.sha256(native.data).hexdigest()
if sha!='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5':
    raise SystemExit('Unsupported native DLL; no ABI assumptions accepted')
# Match instruction evidence as well as the full-file identity. These checks
# deliberately fail if a future audit silently changes the stated offsets.
cases={
 'root_factory':(0x18c10,0x18f90,['[rbp+0x74],0x5','[rbp+0x88],0x24','[rbp+0x9c],0x8','[rbp-0x64],0x40','[rbp-0x4c],0x3b920','[rsp+0x50],0x8']),
 'root_install':(0x2b00d,0x2b04d,['call   0x180018c10','[rdi+0x70],rbx']),
 'table_cycle':(0x1bd50,0x1be30,['edx,0x31','call   0x18001aa00']),
 'table_allocate':(0x1aa00,0x1acf0,['[rcx+0x18]','[r15],rax','[r12],rax','[r13+0x0],rax','0x3b920']),
 'root_bind':(0x1c180,0x1c2fc,['gs:0x58','[rbp+0x30]','[rbp+0x38]','edx,0x6','edx,0x7','[rbp+0x118]']),
 'constant_populate':(0x12e50,0x13120,['gs:0x58','[rax+0x40]','[r9+rax*8+0xf8]','[rax+0x88]','[rax+0xc0]']),
}
results={}
for name,(a,b,tokens) in cases.items():
    asm=native.disasm(a,b)
    missing=[t for t in tokens if t not in asm]
    if missing:raise SystemExit(f'{name}: missing evidence {missing}')
    results[name]={'rva':hex(a),'end':hex(b),'sha256':hashlib.sha256(native.data[native.off(a):native.off(b)]).hexdigest()}
print(json.dumps({'native_sha256':sha,'verified_regions':results,'root_table':{'cbv_b4_to_b8':0,'srv_t0_to_t35':5,'uav_u0_to_u7':41,'count':49},'material_prefix':244000,'compiler_tls_vector_offset':88},indent=2))
