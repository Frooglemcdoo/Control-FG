"""R274: instrument pinned GI spotlight arithmetic; retain every native instruction."""
import base64, hashlib, importlib.util, json, struct
from pathlib import Path
from dxbc_transform import transform_shex
from gi_spot_pcf import decoder, dst, src, ins
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('sites',ROOT/'tools/audit-gi-spot-trace-sites.py')
sites=importlib.util.module_from_spec(spec);spec.loader.exec_module(sites)
CAPACITY=65536
STRIDE=256

def uint(v):return [0x4001,v]
def uav(mask=15):return [0x21e002|(mask<<4),7,0] # SM5.1 identifier 7, u0 in space 2

def generate(raw,path,identity):
 path.write_bytes(raw);decoded=decoder.decode(path);contract=sites.audit(decoded,identity)
 count=struct.unpack_from('<I',raw,28)[0]
 start=next(o+8 for o in struct.unpack_from('<'+'I'*count,raw,32) if raw[o:o+4]==b'SHEX')
 size=struct.unpack_from('<I',raw,start-4)[0];words=list(struct.unpack_from('<'+'I'*(size//4),raw,start))
 assert all(b['space']==1 for b in decoded['bindings'] if b['kind']==3), 'Unreviewed space-0 sampler usage'
 inst=decoded['instructions'];by={i['dword']:i for i in inst}
 td=next(i['dword'] for i in inst if i['name']=='dcl_temps');temp=words[td+1]
 # a.x=record byte address, a.y=valid stages; q scratch, v scratch.
 a,q,v=temp,temp+1,temp+2
 inserts={};patches={td+1:temp+3}
 def add(at,code):inserts.setdefault(at,[]).extend(code)
 def after(i):return i['dword']+((words[i['dword']]>>24)&127)
 def result(i):
  token,reg=words[i['dword']+1:i['dword']+3]
  assert token>>20==1 and (token&0xfff00000)==0x100000
  mask=(token>>4)&15
  if mask in (1,2,4,8):return src(reg,(mask.bit_length()-1)*0x55)
  return src(reg)
 def store(offset,value,mask=15):
  return ins('iadd',dst(q,1),src(a,0),uint(offset))+ins('store_raw',uav(mask),src(q,0),value)
 def stage(i,offset,bit,value=None,before=False,mask=15):
  code=ins('ult',dst(q,2),src(a,0),uint(16+CAPACITY*STRIDE))+ins('if',src(q,0x55))
  # IF must test nonzero; op helper defaults zero control.
  code[7]|=0x40000
  code+=store(offset,result(i) if value is None else value,mask)
  code+=ins('or',dst(a,2),src(a,0x55),uint(bit))+store(192,src(a,0x55),1)+ins('endif')
  add(i['dword'] if before else after(i),code)
 c=contract['sites'];coef=by[c['light_coefficients']['instruction_shex_dword']]
 # Resolve native pixel/ray counter from final ray increment, and light ID from load.
 ray_inc=next(i for i in reversed(inst) if i['name']=='iadd' and i['operands'][-1]=='l(1)')
 assert ray_inc['operands'][0]=='r[0].z'
 light_token=words[coef['dword']+3:coef['dword']+5]
 assert len(light_token)==2
 assert ((light_token[0]>>2)&3)==2
 light_token=src(light_token[1],((light_token[0]>>4)&3)*0x55)
 # Only one pixel in each 32x32 cell; full scene coverage, no material exclusions.
 code=ins('mov',dst(a,1),uint(0xffffffff))+ins('mov',dst(a,2),uint(0))
 code+=ins('and',dst(q,3),src(0),uint(31))
 code+=ins('ieq',dst(q,3),src(q),uint(16))+ins('and',dst(q,1),src(q,0),src(q,0x55))
 branch=ins('if',src(q,0));branch[0]|=0x40000;code+=branch
 code+=ins('imm_atomic_iadd',dst(q,1),uav(1),uint(0),uint(1))
 code+=ins('ult',dst(q,2),src(q,0),uint(CAPACITY))
 branch=ins('if',src(q,0x55));branch[0]|=0x40000;code+=branch
 code+=ins('ishl',dst(a,1),src(q,0),uint(8))+ins('iadd',dst(a,1),src(a,0),uint(16))
 code+=ins('mov',dst(v,7),src(0))+ins('mov',dst(v,8),light_token)+store(0,src(v))
 code+=ins('endif')+ins('endif')
 add(coef['dword'],code)
 stage(coef,16,1)
 fields=[('shadow_visibility_before_early_out',64,2),('light_color_range',32,4),
         ('distance_squared',48,8),('denominator_before_floor',52,16),('attenuation',56,32),
         ('range_times_attenuation',60,64),('projection_color',80,128)]
 for name,off,bit in fields:stage(by[c[name]['instruction_shex_dword']],off,bit,mask=1 if name in ('distance_squared','denominator_before_floor','attenuation','range_times_attenuation') else 15)
 mad=by[c['spot_diffuse_accumulation']['instruction_shex_dword']];p=mad['dword']
 # Native scalar factor and gated RGB, before accumulation into previous lights.
 stage(mad,96,256,words[p+3:p+5],True)
 stage(mad,112,512,words[p+5:p+7],True)
 # Resolve secondary albedo and primary throughput from post-spot operations.
 end=inst.index(mad)
 albedo=next(i for i in inst[end:] if i['name']=='mul' and i['operands'][0]=='r[15].xyz')
 throughput=next(i for i in inst[end:] if i['name']=='mad' and i['operands'][0]=='r[10].xyz')
 assert albedo['operands'][1]=='r[17].xyzx' and throughput['operands'][1]=='r[3].xyzx'
 stage(mad,128,1024,src(17),True)
 stage(mad,144,2048,src(3),True)
 stage(mad,160,4096,src(12),True)
 stage(mad,176,8192,src(13),True)
 # New declaration uses an identifier outside all native UAV declarations.
 decl=ins('dcl_unordered_access_view_raw',[0x31ee46,7,0,0,2])
 first=next(i['dword'] for i in inst if not i['name'].startswith('dcl_'))
 add(first,decl)
 def patch(w):
  out=w[:2]
  for n in range(2,len(w)):
   out+=inserts.get(n,[]);out.append(patches.get(n,w[n]))
  w[:]=out
 changed=transform_shex(raw,patch)
 # Invert insertions and the single temp-count change, recovering exact native bytes.
 def undo(w):
  out=w[:2];cursor=2
  for n in range(2,len(words)):
   cursor+=len(inserts.get(n,[]));out.append(words[n] if n in patches else w[cursor]);cursor+=1
  assert cursor==len(w);w[:]=out
 assert transform_shex(changed,undo)==raw
 path.write_bytes(changed)
 # GPU oracle executes the exact inserted store/atomic/branch blocks with
 # deterministic native-register values, including an early-out and cookie skip.
 fixture=words[:2]+words[2:3]+decl+ins('dcl_temps',[temp+3])+ins('dcl_thread_group',[1,1,1])
 def floats(*values):return [0x4002]+[struct.unpack('<I',struct.pack('<f',x))[0] for x in values]
 for sample in range(4):
  fixture+=ins('mov',dst(0,15),[0x4002,16,16,sample//2,0])
  fixture+=ins('mov',dst(10,8),uint(100+sample))
  fixture+=inserts[coef['dword']]
  for index,(name,offset,bit) in enumerate([('light_coefficients',16,1)]+fields):
   item=by[c[name]['instruction_shex_dword']]
   if name=='projection_color' and sample==1:continue
   if sample==0 and index>1:continue
   destination=words[item['dword']+1:item['dword']+3]
   fixture+=ins('mov',destination,floats(index+.125,index+.25,index+.5,0 if name=='shadow_visibility_before_early_out' and sample==0 else index+.75))
   fixture+=inserts[after(item)]
  if sample:
   fixture+=ins('mov',dst(14,15),floats(.1,.2,.3,.4))+ins('mov',dst(20,15),floats(2,3,4,5))
   fixture+=ins('mov',dst(17,15),floats(.5,.6,.7,.8))+ins('mov',dst(3,15),floats(.9,1,1.1,1.2))
   fixture+=ins('mov',dst(12,15),floats(10,11,12,13))+ins('mov',dst(13,15),floats(0,1,0,14))
   fixture+=inserts[mad['dword']]
 fixture+=ins('ret')
 def fixture_patch(w):w[:]=fixture
 (ROOT/'build/gi-spot-trace-oracle.dxbc').write_bytes(transform_shex(raw,fixture_patch))
 return changed,dict(native_sha256=identity['sha256'],sha256=hashlib.sha256(changed).hexdigest(),temp=temp,capacity=CAPACITY,stride=STRIDE,sampling_step=32,sites=contract['sites'])

def main():
 out=ROOT/'build';out.mkdir(exist_ok=True)
 header=['#pragma once','namespace gi_spot_trace_shader {','struct Shader {size_t nativeSize;const char* nativeSha;const unsigned char* bytes;size_t size;};']
 identities=json.loads((ROOT/'research/gi-source-r267/manifest.json').read_text());audit=[]
 for v in (6,):
  identity=identities[v]
  raw=base64.b64decode((ROOT/f'research/gi-source-r267/gi-{v}.dxbc.b64').read_bytes())
  assert hashlib.sha256(raw).hexdigest()==identity['sha256']
  data,record=generate(raw,out/f'gi-spot-trace-{v}.dxbc',identity);audit.append(record)
  header.append(f'inline constexpr unsigned char shader{v}[]={{'+','.join(map(str,data))+'};')
 header.append('inline constexpr Shader shaders[]={'+','.join('{'+f'{i["bytes"]},"{i["sha256"]}",shader{v},sizeof(shader{v})'+'}' for v,i in [(6,identities[6])])+'};')
 from gi_ray_trace import generate as generate_ray
 ray,ray_report=generate_ray(raw,out/'gi-ray-trace-6.dxbc',identities[6])
 header.append('inline constexpr unsigned char rayShader6[]={'+','.join(map(str,ray))+'};')
 header.append('inline constexpr Shader rayShader={'+f'{identities[6]["bytes"]},"{identities[6]["sha256"]}",rayShader6,sizeof(rayShader6)'+'};')
 (out/'gi-ray-trace-validation.json').write_text(json.dumps(ray_report,indent=2)+'\n')
 header.append('}')
 (out/'gi_spot_trace.generated.h').write_text('\n'.join(header)+'\n')
 (out/'gi-spot-trace-validation.json').write_text(json.dumps(audit,indent=2)+'\n')
 print('PASS R274 active variant 6: exact native-instruction recovery, bounded stage records, dedicated space2 UAV')
if __name__=='__main__':main()
