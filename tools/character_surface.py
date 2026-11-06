"""Audited whole-character lighting boundary diagnostics; no unknown live patching."""
import base64,hashlib,json,re,struct,sys
from pathlib import Path
from dxbc_transform import transform_shex
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
NAMES=['filtered-volume','surface-normal','uniform-volume','normal-and-uniform-volume','rig-diffuse-only','rig-specular-only']
def dst(r,mask):return [0x100002|(mask<<4),r]
def imm(v):return [0x4001,struct.unpack('<I',struct.pack('<f',v))[0]]
def mov(d,s):return [((1+len(d)+len(s))<<24)|54]+d+s
def words(b):
 for off in struct.unpack_from('<'+'I'*struct.unpack_from('<I',b,28)[0],b,32):
  if b[off:off+4]==b'SHEX':return list(struct.unpack_from('<'+'I'*(struct.unpack_from('<I',b,off+4)[0]//4),b,off+8))
 raise ValueError('Missing SHEX')
def reg(s):
 m=re.fullmatch(r'r\[(\d+)\]\.[xyzw]+',s);assert m,s;return int(m[1])
def src(r,swizzle=0xe4):return [0x100006|(swizzle<<4),r]
def op(code,*operands):
 v=sum((list(x) for x in operands),[]);return [((1+len(v))<<24)|code]+v
def vec(*v):return [0x4002]+[struct.unpack('<I',struct.pack('<f',x))[0] for x in v]
def patches(info):
 x=info['instructions']
 hits=[n for n,i in enumerate(x) if i['name']=='ld' and i['operands'][-1]=='t[2][2].ywxz'];assert len(hits)==1
 n=hits[0];a=x[n:n+6]
 assert [i['name'] for i in a]==['ld','mul','mul','mad','add','dp3']
 diffuse=reg(a[1]['operands'][2]);specular=reg(a[2]['operands'][2])
 # Only skin/cloth consume bent normals. Hair keeps its authored anisotropic normal.
 normal={};bs=[b for b in info['bindings'] if b['name']=='g_tBentNormals']
 if bs:
  t='t[%d][%d].xyzw'%(bs[0]['identifier'],bs[0]['slot'])
  n=next(n for n,i in enumerate(x) if i['name']=='ld' and t in i['operands'])
  r=reg(x[n]['operands'][0]);assert x[n+1]['name']=='mad'
  assert x[n+1]['operands'][2:]==['l(2,2,2,0)','l(-1,-1,-1,0)']
  decoded=[i for i in x[:n] if i['name']=='div'][-1]
  assert decoded['operands']==['r[2].xyw','r[13].zwzy','r[13].xxxx']
  normal={x[n+2]['dword']:mov(dst(r,7),src(2,0x34))}
 n=next(n for n,i in enumerate(x) if i['name']=='sample_l' and 't[8][8].' in ' '.join(i['operands']))
 pair=x[n:n+2];assert pair[1]['name']=='sample_l' and pair[1]['operands'][2]=='t[8][9].xyzw'
 assert pair[0]['operands'][1]==pair[1]['operands'][1]=='r[5].xyzx'
 first=pair[0]['operands'][0];assert first in ('r[6].xyz','r[6].yzw')
 assert pair[1]['operands'][0]=='r[5].x'
 m=next(j for j in range(n+2,n+15) if x[j]['name']=='max')
 assert x[m]['operands']==['r[5].x','r[5].x','r[5].y']
 assert x[m+1]['name']=='mul' and x[m+1]['operands'][0]=='r[5].xyz'
 uniform={x[m+1]['dword']:mov(dst(6,14 if first.endswith('yzw') else 7),imm(1))+mov(dst(5,1),src(5,0x55))}
 return [{},normal,uniform,{**normal,**uniform},
         {a[0]['dword']:mov(dst(specular,7),imm(0))},
         {a[0]['dword']:mov(dst(diffuse,7),imm(0))}]
def filtered_volume(data,info):
 # Average eight trilinear reads at half-cell offsets in XYZ, BEFORE the native
 # chroma normalization and brightness floor. No temporal history or jitter added.
 w=words(data);x=info['instructions']
 first=next(i for i in x if i['name']=='sample_l' and 't[8][8].' in ' '.join(i['operands']))
 o=first['dword'];assert w[o]>>24==13 and w[o+13]>>24==13
 assert x[next(n for n,i in enumerate(x) if i is first)+1]['operands'][2]=='t[8][9].xyzw'
 decl=next(i['dword'] for i in x if i['name']=='dcl_temps')
 count=w[decl+1];uv,inv,coord,total,tap=range(count,count+5)
 rgb=w[o:o+13];scalar=w[o+13:o+26]
 assert rgb[3:5]==scalar[3:5]==src(5,0x24)
 # Both sample destinations use the same four temporary lanes: RGB and intensity.
 rgb[1:3]=dst(tap,7);rgb[3:5]=src(coord,0x24);rgb[5]=0x207e46
 scalar[1:3]=dst(tap,8);scalar[3:5]=src(coord,0x24);scalar[5]=0x207006
 code=mov(dst(uv,7),src(5))+op(86,dst(inv,7),[0x308246,1,1,18])
 code+=op(14,dst(inv,7),imm(.5),src(inv))+mov(dst(total,15),imm(0))
 for z in (-1,1):
  for y in (-1,1):
   for xx in (-1,1):
    code+=op(50,dst(coord,7),src(inv),vec(xx,y,z,0),src(uv))
    code+=rgb+scalar+op(0,dst(total,15),src(total),src(tap))
 code+=op(56,dst(total,15),src(total),imm(.125))
 mask=(w[o+1]>>4)&15
 assert mask in (7,14)
 code+=mov(dst(6,mask),src(total,0xe4 if mask==7 else 0x90))+mov(dst(5,1),src(total,0xff))
 original=w[o:o+26]
 def apply(v):v[o:o+26]=code;v[decl+1]=count+5
 result=transform_shex(data,apply)
 restored=words(result);assert restored[o:o+len(code)]==code
 restored[o:o+len(code)]=original;restored[decl+1]=count;restored[1]=len(restored)
 assert restored==w
 return result
def main():
 folder=root/'build';folder.mkdir(exist_ok=True);research=root/'research/character-surface-r346'
 families=json.loads((research/'families.json').read_text());shaders=[];audit=[]
 for family in families:
  data=base64.b64decode((research/(family['name']+'.dxbc.b64')).read_text());assert hashlib.sha256(data).hexdigest()==family['sha']
  p=folder/('character-'+family['name']+'.dxbc');p.write_bytes(data);old=decode(p);patchset=patches(old);shaders.append(data)
  for index,patch in enumerate(patchset):
   def apply(w):
    for offset,ins in sorted(patch.items(),reverse=True):w[offset:offset]=ins
   result=filtered_volume(data,old) if index==0 else transform_shex(data,apply);assert result[4:20]==checksum(result)
   p=folder/('character-'+family['name']+'-'+NAMES[index]+'.dxbc');p.write_bytes(result);new=decode(p)
   assert old['bindings']==new['bindings'] and old['constant_buffers']==new['constant_buffers']
   if index!=0:
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
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterOriginals[] = {'+','.join(ref(i*7) for i in range(len(families)))+'};']
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterVariants[][6] = {']
 lines+=['{'+','.join(ref(i*7+j) for j in range(1,7))+'},' for i in range(len(families))];lines+=['};']
 lines+=['inline constexpr unsigned kCharacterFamilyCount='+str(len(families))+';']
 lines+=['inline constexpr unsigned kCharacterGroups[] = {'+','.join(str(f['group']) for f in families)+'};']
 (folder/'character_surface.generated.h').write_text('\n'.join(lines)+'\n')
 (folder/'character-surface-audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 print(f'PASS {len(families)} native families, {6*len(families)} rig variants; exact bindings and full original instruction recovery; alpha/depth/coverage preserved')
if __name__=='__main__':main()
