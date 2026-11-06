"""Audited whole-character lighting boundary diagnostics; no unknown live patching."""
import base64,hashlib,json,re,struct,sys
from pathlib import Path
from dxbc_transform import transform_shex
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
NAMES=['hq-cubic','r351-filter','base-diffuse','base-specular','base-diffuse-dim','base-specular-dim']
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
def composition_site(info):
 x=info['instructions']
 hits=[n for n,i in enumerate(x) if i['name']=='ld' and i['operands'][-1]=='t[2][2].ywxz']
 assert len(hits)==1
 a=x[hits[0]:hits[0]+6]
 assert [i['name'] for i in a]==['ld','mul','mul','mad','add','dp3']
 assert a[3]['operands'][0]==a[4]['operands'][0]=='r[0].xyz'
 assert a[3]['operands'][1].endswith('.yzwy')
 assert all(z.endswith('.xyzx') for z in a[3]['operands'][2:])
 assert a[4]['operands'][1]=='r[0].xyzx' and a[4]['operands'][2].endswith('.xyzx')
 return a

def composition_view(data,info,component):
 # Capture one actual composition term before the multiply-add overwrites it,
 # then feed that term through the unchanged native atmosphere/media tail.
 assert component in range(4)
 a=composition_site(info);w=words(data)
 decl=next(i['dword'] for i in info['instructions'] if i['name']=='dcl_temps');count=w[decl+1]
 operands=a[3]['operands']
 if component==0:keep=mov(dst(count,7),src(reg(a[4]['operands'][2])))
 elif component==1:keep=op(56,dst(count,7),src(reg(operands[1]),0x39),src(reg(operands[2])))
 elif component==2:keep=mov(dst(count,7),src(reg(operands[3])))
 else:keep=op(56,dst(count,7),src(reg(operands[2])),imm(.25))
 patches={a[3]['dword']:keep,a[5]['dword']:mov(dst(0,7),src(count))}
 def apply(v):
  for at,code in sorted(patches.items(),reverse=True):v[at:at]=code
  v[decl+1]=count+1
 result=transform_shex(data,apply);v=words(result)
 for at,code in sorted(patches.items()):
  assert v[at:at+len(code)]==code;del v[at:at+len(code)]
 v[decl+1]=count;v[1]=len(v);assert v==w
 return result

def base_composition_site(info):
 x=info['instructions']
 diffuse=[i for i in x if i['name']=='ld' and i['operands'][-1]=='t[3][3].xyzw']
 specular=[i for i in x if i['name']=='ld' and i['operands'][-1]=='t[4][4].xyzw']
 assert len(diffuse)==len(specular)==1
 d,s=diffuse[0],specular[0];dr=reg(d['operands'][0]);sr=reg(s['operands'][0])
 assert d['operands'][0].endswith('.xyz') and s['operands'][0].endswith('.xyz')
 end=next(i['dword'] for i in x if i['dword']>s['dword'] and i['name']=='ld' and i['operands'][-1]=='t[0][0].xyzw')
 hits=[i for i in x if s['dword']<i['dword']<end and i['name']=='mad' and i['operands'][0]==f'r[{sr}].xyz' and i['operands'][1]==f'r[{dr}].xyzx' and i['operands'][3]==f'r[{sr}].xyzx' and i['operands'][2].endswith('.xyzx')]
 assert len(hits)==1
 a=hits[0];assert a['operands'][2].endswith('.xyzx')
 # No diffuse/specular overwrite between their loads and the material MAD.
 for load,rr in ((d,dr),(s,sr)):
  for i in x:
   if not load['dword']<i['dword']<a['dword'] or not i['operands']:continue
   dest=i['operands'][0]
   if dest.startswith(f'r[{rr}].'):assert not set(dest.split('.')[-1])&set('xyz')
 return a

def base_component_view(data,info,component):
 assert component in range(4)
 a=base_composition_site(info);tail=composition_site(info)[5]
 w=words(data);decl=next(i['dword'] for i in info['instructions'] if i['name']=='dcl_temps');count=w[decl+1]
 _,diffuse,material,specular=a['operands']
 keep=(op(56,dst(count,7),src(reg(diffuse)),src(reg(material))) if component%2==0 else mov(dst(count,7),src(reg(specular))))
 if component>=2:keep+=op(56,dst(count,7),src(count),imm(.25))
 patches={a['dword']:keep,tail['dword']:mov(dst(0,7),src(count))}
 def apply(v):
  for at,code in sorted(patches.items(),reverse=True):v[at:at]=code
  v[decl+1]=count+1
 result=transform_shex(data,apply);v=words(result)
 for at,code in sorted(patches.items()):
  assert v[at:at+len(code)]==code;del v[at:at+len(code)]
 v[decl+1]=count;v[1]=len(v);assert v==w
 return result

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
  info=decode(hp)
  results=[hq,filtered_volume(data,old)]+[base_component_view(hq,info,c) for c in range(4)]
  for index,result in enumerate(results):
   assert result[4:20]==checksum(result)
   p=folder/('character-'+family['name']+'-'+NAMES[index]+'.dxbc');p.write_bytes(result);new=decode(p)
   assert old['bindings']==new['bindings'] and old['constant_buffers']==new['constant_buffers']
   shaders.append(result)
  audit.append({'family':family,'variants':NAMES,'reads_per_volume_per_variant':[27,8,27,27,27,27],'composition_views':['base diffuse times material','base specular','0.25 * base diffuse times material','0.25 * base specular'],'native_fog_tail':True})
 lines=['#pragma once','// Generated whole-character diagnostics. Alpha, depth and coverage stay native.']
 for i,b in enumerate(shaders):
  lines+=['inline constexpr unsigned char kCharacterSurface'+str(i)+'[] = {']
  lines+=[','.join(map(str,b[j:j+24]))+',' for j in range(0,len(b),24)];lines+=['};']
 ref=lambda i:'{kCharacterSurface'+str(i)+',sizeof(kCharacterSurface'+str(i)+')}'
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterOriginals[] = {'+','.join(ref(i*7) for i in range(len(families)))+'};']
 lines+=['inline constexpr D3D12_SHADER_BYTECODE kCharacterVariants[][6] = {']
 lines+=['{'+','.join(ref(i*7+j) for j in range(1,7))+'},' for i in range(len(families))];lines+=['};']
 lines+=['inline constexpr unsigned kCharacterVariantCount=6;']
 lines+=['inline constexpr unsigned kCharacterFamilyCount='+str(len(families))+';']
 lines+=['inline constexpr unsigned kCharacterGroups[] = {'+','.join(str(f['group']) for f in families)+'};']
 (folder/'character_surface.generated.h').write_text('\n'.join(lines)+'\n')
 (folder/'character-surface-audit.json').write_text(json.dumps(audit,indent=2)+'\n')
 print(f'PASS {len(families)} native families, {6*len(families)} rig variants; exact bindings and full original instruction recovery; alpha/depth/coverage preserved')
if __name__=='__main__':main()
