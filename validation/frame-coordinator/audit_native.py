from pathlib import Path
import hashlib,json,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'native-history'))
from pe_tools import PE
here=Path(__file__).resolve().parent;renderer=PE(sys.argv[1]);d3d=PE(sys.argv[2]);checks=[]
def ck(name,value):
 assert value,name
 checks.append(name)
ck('renderer identity',hashlib.sha256(renderer.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433')
ck('d3d identity',hashlib.sha256(d3d.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5')
for rva,iat in [(0x11b697,0x5df758),(0x16786,0x5dfcc0),(0x1300e0,0x5df898),(0x13f574,0x5df898)]:
 b=renderer.data[renderer.off(rva):renderer.off(rva)+6]
 ck(f'indirect call {rva:X}',b[:2]==b'\xff\x15' and rva+6+struct.unpack('<i',b[2:])[0]==iat)
b=renderer.data[renderer.off(0x12b8d7):renderer.off(0x12b8d7)+5]
ck('reflection filter direct call',b[0]==0xe8 and 0x12b8dc+struct.unpack('<i',b[1:])[0]==0x15bd0)
regions=[(renderer,0x1300b9,0x1300e6,'aa-caller-0',['[rsp+0x40],rbp','[rsp+0x38],rbp','[rsp+0x30],rbp']),
 (renderer,0x13f55b,0x13f57a,'aa-caller-1',['[rsp+0x40],r12','[rsp+0x38],r12','[rsp+0x30],r12']),
 (d3d,0x1fa20,0x1fa60,'aa-guide-transitions',['[rbp+0x348]','[rbp+0x350]','[rbp+0x340]','0x18003a520']),
 (d3d,0x1fd0e,0x1fd9f,'aa-guide-resources',['[rax+0x88]','[rbp+0x348]','[rbp+0x350]','[rbp+0x340]']),
 (d3d,0x51d30,0x51d82,'native-pointer-getter',['[rax+0x40]','mov    r8,rdi','mov    rdx,rsi']),
 (renderer,0x137bb1,0x137c96,'rt-option-updates',['0x180912d00','0x180914260','0x180912df0','0x180913060'])]
for pe,a,b,name,tokens in regions:
 s=pe.disasm(a,b)
 for token in tokens:ck(name+': '+token,token in s)
 (here/(name+'.asm')).write_text(s)
(here/'native-audit.json').write_text(json.dumps({'status':'PASS','checks':checks,'method':'Static exact-DLL instruction audit; Windows and GPU execution not performed'},indent=2)+'\n')
print('PASS',len(checks),'native boundary assertions')
