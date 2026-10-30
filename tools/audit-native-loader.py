"""Read-only upstream loader audit against the supplied Steam runtime."""
import argparse,hashlib,json,struct
import pefile,capstone
p=argparse.ArgumentParser();p.add_argument('runtime');a=p.parse_args()
data=open(a.runtime,'rb').read();digest=hashlib.sha256(data).hexdigest()
assert digest=='a79234e4b34c2488deaac3459e8054be206b5af78a37723d834c1cbd826ce484'
pe=pefile.PE(data=data);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
calls={0xfa30f:0xff080,0xff15c:0xfa320,0xff280:0xfa320,0xfcae3:0xfb440,0xfcaea:0xfbae0,0xfca82:0xfc5e0,0xfca89:0xfbdc0,0xfc853:0x18f760,0x18e4f5:0x196640,0xfb8f3:0xfaa00,0x18be62:0x1951c0,0x18bf0e:0x1951c0,0x18c2c2:0x1951c0}
for site,target in calls.items():
 b=pe.get_data(site,5);assert b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target
points=[0xff0a7,0xff14f,0xff273,0xfa345,0xfa353,0xfa361,0xfa36f,0xfa37c,0xfa38f,0xfa39c,
        0xfc5af,0xfc5b6,0xfb244,0xfb28e,0xfb5b0,0xfb5c3,0xfb5da,0xfb5f6,0xfb789,0xfb83a,
        0xfb8f3,0xfb90f,0xfb922,0xfb958,0xfb9d0,0xfb9de,0xfd186,0xfd18c,0xfd1aa,0xfd1af,0xfd1ba]
for site,expected in {0x1933f1:'ff1519bd0700',0xfb6b4:'ff15b63a1100',0xfb80a:'ff1560391100'}.items():
 assert pe.get_data(site,6).hex()==expected
points += [0x18f923,0x18f927,0x18f92e,0x18f932,0x18f935,0x196660,0x196664,0x196672,0xfb35a,0xfb3f2,0xfb3fc]
output={'runtime_sha256':digest,'calls':{hex(k):hex(v) for k,v in calls.items()},'evidence':[]}
for site in points:
 i=next(cs.disasm(pe.get_data(site,15),site));output['evidence'].append({'rva':hex(site),'bytes':i.bytes.hex(),'instruction':i.mnemonic+' '+i.op_str})
assert next(x for x in output['evidence'] if x['rva']=='0xfc5b6')['instruction']=='mov qword ptr [rdi + 0x178], rax'
print(json.dumps(output,indent=2))
