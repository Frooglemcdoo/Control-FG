"""Audited whole-character lighting boundary diagnostics; no unknown live patching."""
import base64,hashlib,json,re,struct,sys
from pathlib import Path
from dxbc_transform import transform_shex
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
NAMES=['hq-cubic','r351-filter','shadow-y-neutral','shadow-w-neutral','translucency-off','surface-normal','rig-off','buffers-off']
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
def cubic_volume(data,info):
 # Cubic B-spline convolved with [1,2,1]/4 in each axis. Six nonnegative
 # weights factor into27 trilinear reads per texture; continuous cell transitions.
 # Pairwise linear factoring follows Sigg/Hadwiger, GPU Gems2 chapter20.
 w=words(data);x=info['instructions']
 first=next(i for i in x if i['name']=='sample_l' and 't[8][8].' in ' '.join(i['operands']))
 o=first['dword'];assert w[o]>>24==13 and w[o+13]>>24==13
 assert x[next(n for n,i in enumerate(x) if i is first)+1]['operands'][2]=='t[8][9].xyzw'
 decl=next(i['dword'] for i in x if i['name']=='dcl_temps');count=w[decl+1]
 q,dim,base,f,w0,w1,w2,w3,g0,g1,pos2,g2=range(count,count+12)
 rgb=w[o:o+13];scalar=w[o+13:o+26]
 assert rgb[3:5]==scalar[3:5]==src(5,0x24)
 # Once weights and coordinates are built, reuse dead scratch registers.
 lo,mid,hi,coord,total,tap,weight=q,f,pos2,w2,w0,w1,w3
 rgb[1:3]=dst(tap,7);rgb[3:5]=src(coord,0x24);rgb[5]=0x207e46
 scalar[1:3]=dst(tap,8);scalar[3:5]=src(coord,0x24);scalar[5]=0x207006
 code=op(86,dst(dim,7),[0x308246,1,1,18])
 code+=op(50,dst(q,7),src(5),src(dim),imm(-.5))
 code+=op(65,dst(base,7),src(q))+op(50,dst(f,7),src(base),imm(-1),src(q))
 # w0=(1-f)^3/6, w1=(3f^3-6f^2+4)/6,
 # w2=(-3f^3+3f^2+3f+1)/6, w3=f^3/6.
 code+=op(50,dst(w0,7),src(f),imm(-1),imm(1))
 code+=op(56,dst(q,7),src(w0),src(w0))+op(56,dst(w0,7),src(w0),src(q))
 code+=op(56,dst(w0,7),src(w0),imm(1/6))
 code+=op(50,dst(w1,7),src(f),imm(3),imm(-6))
 code+=op(56,dst(w1,7),src(w1),src(f))+op(50,dst(w1,7),src(w1),src(f),imm(4))
 code+=op(56,dst(w1,7),src(w1),imm(1/6))
 code+=op(50,dst(w2,7),src(f),imm(-3),imm(3))
 code+=op(50,dst(w2,7),src(w2),src(f),imm(3))+op(50,dst(w2,7),src(w2),src(f),imm(1))
 code+=op(56,dst(w2,7),src(w2),imm(1/6))
 code+=op(56,dst(w3,7),src(f),src(f))+op(56,dst(w3,7),src(w3),src(f))
 code+=op(56,dst(w3,7),src(w3),imm(1/6))
 # Convolution combines the cubic coefficients into six weights at base-2..+3.
 # Pair sums g0,g1,g2 and upper-member numerators v1,v3,v5 avoid storing all six.
 code+=op(56,dst(g0,7),src(w1),imm(.25))+op(50,dst(g0,7),src(w0),imm(.75),src(g0))
 code+=op(56,dst(g2,7),src(w2),imm(.25))+op(50,dst(g2,7),src(w3),imm(.75),src(g2))
 code+=op(0,dst(g1,7),src(w1),src(w2))+op(56,dst(g1,7),src(g1),imm(.75))
 code+=op(50,dst(g1,7),src(w0),imm(.25),src(g1))+op(50,dst(g1,7),src(w3),imm(.25),src(g1))
 code+=op(56,dst(lo,7),src(w1),imm(.25))+op(50,dst(lo,7),src(w0),imm(.5),src(lo))
 code+=op(56,dst(mid,7),src(w1),imm(.25))+op(50,dst(mid,7),src(w2),imm(.5),src(mid))
 code+=op(50,dst(mid,7),src(w3),imm(.25),src(mid))+op(56,dst(hi,7),src(w3),imm(.25))
 for pos,g,offset in ((lo,g0,-1.5),(mid,g1,.5),(hi,g2,2.5)):
  code+=op(14,dst(pos,7),src(pos),src(g))+op(0,dst(pos,7),src(pos),imm(offset))
  code+=op(0,dst(pos,7),src(pos),src(base))+op(14,dst(pos,7),src(pos),src(dim))
 code+=mov(dst(total,15),imm(0))
 positions=(lo,mid,hi);groups=(g0,g1,g2)
 for z in range(3):
  for y in range(3):
   for xx in range(3):
    for axis,choice in enumerate((xx,y,z)):
     code+=mov(dst(coord,1<<axis),src(positions[choice]))
     code+=mov(dst(weight,1<<axis),src(groups[choice]))
    code+=op(56,dst(weight,1),src(weight,0),src(weight,0x55))
    code+=op(56,dst(weight,1),src(weight,0),src(weight,0xaa))
    code+=rgb+scalar+op(50,dst(total,15),src(tap),src(weight,0),src(total))
 mask=(w[o+1]>>4)&15;assert mask in (7,14)
 code+=mov(dst(6,mask),src(total,0xe4 if mask==7 else 0x90))+mov(dst(5,1),src(total,0xff))
 original=w[o:o+26]
 def apply(v):v[o:o+26]=code;v[decl+1]=count+12
 result=transform_shex(data,apply)
 restored=words(result);assert restored[o:o+len(code)]==code
 restored[o:o+len(code)]=original;restored[decl+1]=count;restored[1]=len(restored);assert restored==w
 return result
def neck_patches(info):
 x=info['instructions']
 hits=[n for n,i in enumerate(x) if i['name']=='ld' and i['operands'][-1]=='t[2][2].ywxz'];assert len(hits)==1
 n=hits[0];a=x[n:n+6]
 assert [i['name'] for i in a]==['ld','mul','mul','mad','add','dp3']
 shadow=reg(a[0]['operands'][0]);assert a[0]['operands'][0].endswith('.xy')
 diffuse=reg(a[1]['operands'][2]);specular=reg(a[2]['operands'][2])
 normal={};trans={};buffers={}
 for name,target in [('g_tLightBufferDiffuse',buffers),('g_tLightBufferSpecular',buffers),('g_tTranslucencyKernelSource',trans)]:
  b=next((b for b in info['bindings'] if b['name']==name),None)
  if not b:
   assert name=='g_tTranslucencyKernelSource'
   continue
  t='t[%d][%d].xyzw'%(b['identifier'],b['slot'])
  matches=[j for j,i in enumerate(x) if i['name'] in ('ld','sample_l') and t in i['operands']]
  assert len(matches)==(3 if target is trans else 1),(name,matches)
  for j in matches:
   assert x[j]['operands'][0].endswith('.xyz')
   target[x[j+1]['dword']]=mov(dst(reg(x[j]['operands'][0]),7),imm(0))
 bs=[b for b in info['bindings'] if b['name']=='g_tBentNormals']
 if bs:
  t='t[%d][%d].xyzw'%(bs[0]['identifier'],bs[0]['slot'])
  n=next(n for n,i in enumerate(x) if i['name']=='ld' and t in i['operands'])
  r=reg(x[n]['operands'][0]);assert x[n+1]['name']=='mad'
  assert x[n+1]['operands'][2:]==['l(2,2,2,0)','l(-1,-1,-1,0)']
  decoded=[i for i in x[:n] if i['name']=='div'][-1]
  assert decoded['operands']==['r[2].xyw','r[13].zwzy','r[13].xxxx']
  normal={x[n+2]['dword']:mov(dst(r,7),src(2,0x34))}
 return [{a[1]['dword']:mov(dst(shadow,1),imm(1))},
         {a[1]['dword']:mov(dst(shadow,2),imm(1))},trans,normal,
         {a[0]['dword']:mov(dst(diffuse,7),imm(0))+mov(dst(specular,7),imm(0))},buffers]

def insert_verified(data,patch):
 def apply(w):
  for offset,ins in sorted(patch.items(),reverse=True):w[offset:offset]=ins
 result=transform_shex(data,apply);a=words(result)
 for offset,ins in sorted(patch.items()):
  assert a[offset:offset+len(ins)]==ins;del a[offset:offset+len(ins)]
 a[1]=len(a);assert a==words(data)
 return result

def main():
 folder=root/'build';folder.mkdir(exist_ok=True);research=root/'research/character-surface-r346'
 families=json.loads((research/'families.json').read_text());shaders=[];audit=[]
 for family in families:
  data=base64.b64decode((research/(family['name']+'.dxbc.b64')).read_text());assert hashlib.sha256(data).hexdigest()==family['sha']
  p=folder/('character-'+family['name']+'.dxbc');p.write_bytes(data);old=decode(p);shaders.append(data)
  hq=cubic_volume(data,old);hp=folder/'neck-hq-temporary.dxbc';hp.write_bytes(hq)
  patchset=neck_patches(decode(hp))
  results=[hq,filtered_volume(data,old)]+[insert_verified(hq,p) for p in patchset]
  for index,result in enumerate(results):
   assert result[4:20]==checksum(result)
   p=folder/('character-'+family['name']+'-'+NAMES[index]+'.dxbc');p.write_bytes(result);new=decode(p)
   assert old['bindings']==new['bindings'] and old['constant_buffers']==new['constant_buffers']
   shaders.append(result)
  audit.append({'family':family,'variants':NAMES,'reads_per_volume_per_variant':[27,8,27,27,27,27,27,27],'neck_patch_counts':[len(p) for p in patchset]})
 lines=['#pragma once','// Generated whole-character diagnostics. Alpha, depth and coverage stay native.']
 for i,b in enumerate(shaders):
  lines+=['inline constexpr unsigned char kCharacterSurface'+str(i)+'[] = {']
  lines+=[','.join(map(str,b[j:j+24]))+',' for j in range(0,len(b),24)];lines+=['};']
 ref=lambda i:'{kCharacterSurface'+str(i)+',sizeof(kCharacterSurface'+str(i)+')}'
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterOriginals[] = {'+','.join(ref(i*9) for i in range(len(families)))+'};']
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterVariants[][8] = {']
 lines+=['{'+','.join(ref(i*9+j) for j in range(1,9))+'},' for i in range(len(families))];lines+=['};']
 lines+=['inline constexpr unsigned kCharacterVariantCount=8;']
 lines+=['inline constexpr unsigned kCharacterFamilyCount='+str(len(families))+';']
 lines+=['inline constexpr unsigned kCharacterGroups[] = {'+','.join(str(f['group']) for f in families)+'};']
 (folder/'character_surface.generated.h').write_text('\n'.join(lines)+'\n')
 (folder/'character-surface-audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 print(f'PASS {len(families)} native families, {8*len(families)} rig variants; exact bindings and full original instruction recovery; alpha/depth/coverage preserved')
if __name__=='__main__':main()
