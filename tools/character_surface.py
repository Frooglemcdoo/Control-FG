"""Audited whole-character lighting boundary diagnostics; no unknown live patching."""
import base64,hashlib,json,re,struct,sys
from pathlib import Path
from dxbc_transform import transform_shex
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
NAMES=['both-shadows-neutral','rig-off','rig-only','flat-before-fog','flat-final','fog-off']
def dst(r,mask):return [0x100002|(mask<<4),r]
def imm(v):return [0x4001,struct.unpack('<I',struct.pack('<f',v))[0]]
def mov(d,s):return [((1+len(d)+len(s))<<24)|54]+d+s
def words(b):
 for off in struct.unpack_from('<'+'I'*struct.unpack_from('<I',b,28)[0],b,32):
  if b[off:off+4]==b'SHEX':return list(struct.unpack_from('<'+'I'*(struct.unpack_from('<I',b,off+4)[0]//4),b,off+8))
 raise ValueError('Missing SHEX')
def reg(s):
 m=re.fullmatch(r'r\[(\d+)\]\.[xyzw]+',s);assert m,s;return int(m[1])
def patches(info):
 x=info['instructions']
 hits=[n for n,i in enumerate(x) if i['name']=='ld' and i['operands'][-1]=='t[2][2].ywxz'];assert len(hits)==1
 n=hits[0];a=x[n:n+6]
 assert [i['name'] for i in a]==['ld','mul','mul','mad','add','dp3']
 shadow=reg(a[0]['operands'][0]);diffuse=reg(a[1]['operands'][2]);specular=reg(a[2]['operands'][2]);buffer=reg(a[4]['operands'][2]);color=reg(a[4]['operands'][0])
 assert color==0 and a[4]['operands'][1]=='r[0].xyzx'
 media=[n for n,i in enumerate(x) if i['name']=='sample_l' and 't[9][10].xyzw' in i['operands']];assert len(media)==1
 m=media[0];assert x[m]['operands'][0]=='r[1].xyzw'
 assert x[m+1]['operands']==['o[0].xyz','r[0].xyzx','r[1].wwww','r[1].xyzx']
 atmos=[i for i in x if i['name']=='mad' and i['operands']==['r[0].xyz','r[0].xyzx','r[1].xyzx','r[2].xyzx']];assert len(atmos)==1
 assert x[-1]['name']=='ret'
 return [
  {a[1]['dword']:mov(dst(shadow,3),imm(1))},
  {a[0]['dword']:mov(dst(diffuse,7),imm(0))+mov(dst(specular,7),imm(0))},
  {a[4]['dword']:mov(dst(buffer,7),imm(0))},
  {a[5]['dword']:mov(dst(color,7),imm(.18))},
  {x[-1]['dword']:mov([0x102072,0],imm(.18))},
  {atmos[0]['dword']:mov(dst(1,7),imm(1))+mov(dst(2,7),imm(0)),x[m+1]['dword']:mov(dst(1,7),imm(0))+mov(dst(1,8),imm(1))}
 ]
def main():
 folder=root/'build';folder.mkdir(exist_ok=True);research=root/'research/character-surface-r346'
 families=json.loads((research/'families.json').read_text());shaders=[];audit=[]
 for family in families:
  data=base64.b64decode((research/(family['name']+'.dxbc.b64')).read_text());assert hashlib.sha256(data).hexdigest()==family['sha']
  p=folder/('character-'+family['name']+'.dxbc');p.write_bytes(data);old=decode(p);patchset=patches(old);shaders.append(data)
  for index,patch in enumerate(patchset):
   def apply(w):
    for offset,ins in sorted(patch.items(),reverse=True):w[offset:offset]=ins
   result=transform_shex(data,apply);assert result[4:20]==checksum(result)
   p=folder/('character-'+family['name']+'-'+NAMES[index]+'.dxbc');p.write_bytes(result);new=decode(p)
   assert old['bindings']==new['bindings'] and old['constant_buffers']==new['constant_buffers']
   a=words(result)
   for offset,ins in sorted(patch.items()):
    assert a[offset:offset+len(ins)]==ins;del a[offset:offset+len(ins)]
   a[1]=len(a);assert a==words(data)
   shaders.append(result)
  audit.append({'family':family,'patches':patchset})
 lines=['#pragma once','// Generated whole-character diagnostics. Alpha, depth and coverage stay native.']
 for i,b in enumerate(shaders):
  lines+=['inline constexpr unsigned char kCharacterSurface'+str(i)+'[] = {']
  lines+=[','.join(map(str,b[j:j+24]))+',' for j in range(0,len(b),24)];lines+=['};']
 ref=lambda i:'{kCharacterSurface'+str(i)+',sizeof(kCharacterSurface'+str(i)+')}'
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterOriginals[] = {'+','.join(ref(i*7) for i in range(6))+'};']
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterVariants[6][6] = {']
 lines+=['{'+','.join(ref(i*7+j) for j in range(1,7))+'},' for i in range(6)];lines+=['};']
 (folder/'character_surface.generated.h').write_text('\n'.join(lines)+'\n')
 (folder/'character-surface-audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 print('PASS six native families, 36 stage variants; exact bindings and full original instruction recovery; alpha/depth/coverage preserved')
if __name__=='__main__':main()
