"""R268: compare four native PCF texels before bilinear visibility filtering."""
import base64, hashlib, importlib.util, json, struct
from pathlib import Path
from dxbc_transform import transform_shex
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('decoder',ROOT/'research/native-integration-audit/tools/decode_dxbc.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def dst(r,mask=3):return [0x100002|(mask<<4),r]
def src(r,sw=0xe4):return [0x100006|(sw<<4),r]
def neg(r,sw=0xe4):return [0x80100006|(sw<<4),0x41,r]
def imm(v):return [0x4001,struct.unpack('<I',struct.pack('<f',v))[0]]
def vec(*v):return [0x4002]+[struct.unpack('<I',struct.pack('<f',x))[0] for x in v]
def ins(name,*args):
 body=sum(args,[]);return [((len(body)+1)<<24)|decoder.opnames.index(name)]+body

def block(sample,compare,atlas,temp):
 # Registers used only by this block. Native registers are read-only except
 # the original scalar visibility destination, written once at the end.
 dim,uv,frac,lo,hi,q,taps=range(temp,temp+7)
 resource=list(sample[5:8]);resource[0]=(resource[0]&~0xff0)|(0xe4<<4)
 code=ins('resinfo',dst(dim),imm(0),resource)
 code+=ins('rcp',dst(dim,12),src(dim,0x44)) # zw = 1/width,1/height
 code+=ins('mad',dst(uv),sample[3:5],src(dim),imm(-.5))
 code+=ins('frc',dst(frac),src(uv))
 code+=ins('round_ni',dst(uv),src(uv))
 code+=ins('add',dst(uv),src(uv),imm(.5))
 code+=ins('mul',dst(uv),src(uv),src(dim,0xee))
 code+=ins('mad',dst(lo),src(dim,0xee),imm(.5),src(atlas))
 code+=ins('add',dst(hi),src(atlas),src(atlas,0xee))
 code+=ins('mad',dst(hi),src(dim,0xee),imm(-.5),src(hi))
 # Defend tiny atlas rectangles; do not invert a clamp interval.
 code+=ins('max',dst(hi),src(hi),src(lo))
 output_component=(sample[1]>>4)&15
 assert output_component in (1,2,4,8)
 channel=(sample[5]>>(4+2*(output_component.bit_length()-1)))&3
 assert channel==0, 'Captured PCF depth channel must remain R'
 for component,(x,y) in enumerate(((0,0),(1,0),(0,1),(1,1))):
  code+=ins('mad',dst(q),src(dim,0xee),vec(x,y,0,0),src(uv))
  code+=ins('max',dst(q),src(q),src(lo))
  code+=ins('min',dst(q),src(q),src(hi))
  tap=list(sample);tap[1:3]=dst(taps,1<<component);tap[3:5]=src(q)
  tap[5]=(tap[5]&~0xff0) # xxxx, same native R depth
  code+=tap
  code+=ins('ge',dst(taps,1<<component),src(taps,component*0x55),compare[5:7])
  code+=ins('and',dst(taps,1<<component),src(taps,component*0x55),imm(1))
 code+=ins('add',dst(q,1),src(taps,0x55),neg(taps,0))
 code+=ins('add',dst(q,2),src(taps,0xff),neg(taps,0xaa))
 code+=ins('mad',dst(q),src(q),src(frac,0),src(taps,0x88))
 code+=ins('add',dst(q,2),src(q,0x55),neg(q,0))
 code+=ins('mad',sample[1:3],src(q,0x55),src(frac,0x55),src(q,0))
 return code

def generate(raw,path):
 identity=hashlib.sha256(raw).hexdigest()
 manifests=json.loads((ROOT/'research/gi-source-r267/manifest.json').read_text())[:8]
 assert any(x['sha256']==identity for x in manifests), 'Unknown native GI shader'
 path.write_bytes(raw);d=decoder.decode(path)
 binding=next(b for b in d['bindings'] if b['name']=='g_tSpotLightPCFShadowAtlas')
 instructions=d['instructions']
 sites=[i for i,x in enumerate(instructions) if x['name']=='sample_l' and x['operands'][2].startswith(f"t[{binding['identifier']}][")]
 assert len(sites)==1
 i=sites[0];site=instructions[i]['dword'];end=instructions[i+3]['dword']
 assert [x['name'] for x in instructions[i:i+3]]==['sample_l','ge','and']
 assert instructions[i-1]['name']=='mad'
 count=struct.unpack_from('<I',raw,28)[0]
 start=next(o+8 for o in struct.unpack_from('<'+'I'*count,raw,32) if raw[o:o+4]==b'SHEX')
 size=struct.unpack_from('<I',raw,start-4)[0]
 words=list(struct.unpack_from('<'+'I'*(size//4),raw,start))
 temp_decl=next(x['dword'] for x in instructions if x['name']=='dcl_temps')
 temp=words[temp_decl+1]
 assert end-site==27
 sample=words[site:site+13];compare=words[site+13:site+20]
 atlas=words[site-1] # last operand register of atlas UV MAD
 code=block(sample,compare,atlas,temp)
 def patch(w):
  w[temp_decl+1]=temp+7
  w[site:end]=code
 result=transform_shex(raw,patch)
 # Invert all intended changes and recover the original container exactly.
 def undo(w):
  w[temp_decl+1]=temp
  w[site:site+len(code)]=words[site:end]
 assert transform_shex(result,undo)==raw
 path.write_bytes(result);after=decoder.decode(path)
 assert after['bindings']==d['bindings'] and after['constant_buffers']==d['constant_buffers']
 return result,dict(native_sha256=identity,sha256=hashlib.sha256(result).hexdigest(),site=site,native_words=27,new_words=len(code),temp=temp,atlas=atlas,sample=sample,compare=compare,code=code)

def main():
 out=ROOT/'build';out.mkdir(exist_ok=True)
 header=['#pragma once','namespace gi_spot_pcf {','struct Shader {size_t nativeSize;const char* nativeSha;const unsigned char* bytes;size_t size;};']
 audit=[]
 for variant in range(8):
  raw=base64.b64decode((ROOT/f'research/gi-source-r267/gi-{variant}.dxbc.b64').read_bytes())
  path=out/f'gi-spot-pcf-{variant}.dxbc';result,record=generate(raw,path);audit.append(record)
  header.append(f'inline constexpr unsigned char shader{variant}[]={{'+','.join(map(str,result))+'};')
 header.append('inline constexpr Shader shaders[]={'+','.join('{'+f'{x["bytes"]},"{x["sha256"]}",shader{i},sizeof(shader{i})'+'}' for i,x in enumerate(json.loads((ROOT/'research/gi-source-r267/manifest.json').read_text())[:8]))+'};')
 header.append('}')
 (out/'gi_spot_pcf.generated.h').write_text('\n'.join(header)+'\n')
 (out/'gi-spot-pcf-validation.json').write_text(json.dumps(audit,indent=2)+'\n')
 print('PASS R268: all eight shaders generated; inverse edits recover native containers; bindings unchanged')
if __name__=='__main__':main()
