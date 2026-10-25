"""Zero only six baked-brick samples in each captured RT diffuse-GI shader."""
import base64, hashlib, importlib.util, json, struct
from pathlib import Path
from dxbc_transform import transform_shex
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('decoder',ROOT/'research/native-integration-audit/tools/decode_dxbc.py')
d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
def main():
 out=ROOT/'build';out.mkdir(exist_ok=True)
 header=['#pragma once','#include "native_lighting_isolation.h"','namespace native_lighting {']
 for index,item in enumerate(json.loads((ROOT/'assets/baked-gi-rt/manifest.json').read_text())):
  name=item['name'];raw=base64.b64decode((ROOT/'assets/baked-gi-rt'/f'{name}.dxbc.b64').read_text())
  assert hashlib.sha256(raw).hexdigest()==item['sha256'] and len(raw)==item['bytes']
  original=out/f'{name}.dxbc';original.write_bytes(raw);decoded=d.decode(original)
  brick=next(b for b in decoded['bindings'] if b['name']=='g_tGIBrickTextures');assert brick['count']==6 and brick['space']==0
  start=next(o+8 for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I',raw,28)[0],raw,32) if raw[o:o+4]==b'SHEX')
  edits=[];slots=[]
  for ins in decoded['instructions']:
   if ins['name']!='sample_l' or not ins['operands'][2].startswith(f"t[{brick['identifier']}]["):continue
   off=ins['dword'];token=struct.unpack_from('<I',raw,start+off*4)[0];length=token>>24&127
   words=list(struct.unpack_from('<'+'I'*length,raw,start+off*4))
   # Exact captured instruction layout: opcode, temp destination, coordinate,
   # resource identifier/index, sampler, scalar immediate LOD.
   assert length==13 and words[1]>>20&3==1 and words[1]>>12&255==0
   replacement=[0x05000036,words[1],words[2],0x4001,0]+[0x0100003a]*(length-5)
   edits.append((off,words,replacement));slots.append(ins['operands'][2])
  assert len(edits)==6 and len(set(slots))==6
  def patch(w):
   for off,expected,replacement in edits:
    assert w[off:off+len(expected)]==expected;w[off:off+len(expected)]=replacement
  result=transform_shex(raw,patch);patched=out/f'{name}_no_bricks.dxbc';patched.write_bytes(result)
  assert len(raw)==len(result)
  allowed=set(range(4,20))
  for off,words,_ in edits:allowed.update(range(start+off*4,start+(off+len(words))*4))
  assert all(a==b for n,(a,b) in enumerate(zip(raw,result)) if n not in allowed)
  assert d.decode(patched)['bindings']==decoded['bindings']
  symbol=f'kRtBakedGi{index}'
  def array(n,w):header.append(f'inline constexpr uint32_t {n}[]={{'+','.join(hex(v)+'u' for v in w)+'};')
  for n,(off,expected,replacement) in enumerate(edits):array(symbol+f'Expected{n}',expected);array(symbol+f'Replacement{n}',replacement)
  header.append(f'inline constexpr InstructionEdit {symbol}Edits[]={{'+','.join('{'+f'{start+off*4},{symbol}Expected{n},{symbol}Replacement{n},{len(expected)}'+'}' for n,(off,expected,_) in enumerate(edits))+'};')
  header.append(f'inline constexpr unsigned char {symbol}Checksum[]={{'+','.join(str(v) for v in result[4:20])+'};')
  header.append(f'inline constexpr ShaderPatch {symbol}={{"RT baked GI {index}",{len(raw)},"{item["sha256"]}","{hashlib.sha256(result).hexdigest()}",{start+edits[0][0]*4},{symbol}Expected0,{len(edits[0][1])},nullptr,0,{symbol}Checksum,{symbol}Edits,6}};')
  print(f'PASS {name}: six baked texture samples zeroed; all other bytes except checksum preserved')
 header+=['inline constexpr const ShaderPatch* kRtBakedGiPatches[]={&kRtBakedGi0,&kRtBakedGi1};','}']
 destination=ROOT/'src/rt_baked_gi_patches.generated.h'
 text='\n'.join(header)+'\n'
 import sys
 if '--check' in sys.argv:assert destination.read_text()==text
 else:destination.write_text(text)
if __name__=='__main__':main()
