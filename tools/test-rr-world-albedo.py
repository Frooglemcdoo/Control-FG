"""Verify captured world contracts and an alpha-only transform of native RGB."""
from pathlib import Path
import subprocess,tempfile,shutil,sys,struct,json,hashlib
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
sys.path.insert(0,str(root/'tools'))
from dxbc_transform import transform_shex
from dxbc_checksum import checksum
subprocess.run([sys.executable,str(root/'tools/generate-rr-world-albedo.py')],check=True)
# Independently decode the patched stream: original instructions are unchanged,
# except for a new alpha-only mov immediately before the final ret.
for entry in json.loads((root/'tools/fixtures/rr-world-albedo/manifest.json').read_text()):
    data=bytes.fromhex((root/f"tools/fixtures/rr-world-albedo/variant-{entry['variant']}.hex").read_text())
    def patch(w):w[-1:-1]=[0x05000036,0x00102082,0,0x00004001,0x3f800000]
    modified=transform_shex(data,patch)
    assert modified[4:20]==checksum(modified)
    with tempfile.TemporaryDirectory() as d:
        a=Path(d)/'a.dxbc';b=Path(d)/'b.dxbc';a.write_bytes(data);b.write_bytes(modified)
        old=decode(a);new=decode(b)
    assert old['bindings']==new['bindings'] and old['constant_buffers']==new['constant_buffers']
    assert old['instructions'][:-1]==new['instructions'][:-2]
    mov=new['instructions'][-2]
    assert mov['name']=='mov' and mov['operands']==['o[0].w','l(1)']
    assert new['instructions'][-1]['name']=='ret'
source=r'''
#include <atomic>
#ifdef _WIN32
#include <d3dcompiler.h>
#endif
#include <cassert>
#include "rr_albedo_shader.h"
#include "rr_world_albedo.h"
#include "native_lighting_live_state.h"
namespace native_lighting {inline LiveSelection selection;}
static bool cloneOkay=true;static unsigned clones=0;
static bool PTReflectionCloneShaderRecord(void*,size_t slot,unsigned stage,int id,size_t size,void* record,void*){
 ++clones;assert(slot==0x50&&stage==4&&id>=kRRWorldAlbedoStorageBase&&id<kRRWorldAlbedoStorageBase+4);
 assert(size==kRRWorldVariants[id-kRRWorldAlbedoStorageBase].replaySize);
 if(!cloneOkay)return false;memset(record,0xAB,0xA8);return true;
}
#include "rr_world_albedo_runtime.h"
int main(){
 RRWorldAlbedoHistory history;
 assert(!history.Update(false));assert(history.Update(true));assert(!history.Update(true));
 assert(history.Update(false));assert(!history.Update(false));
 for(unsigned i=0;i<4;++i){
  const auto& v=kRRWorldVariants[i];RRAlbedoShader::Contract c{};const char* why=nullptr;
  assert(RRAlbedoShader::ParsePixelContract(v.native,v.nativeSize,c,&why)&&c.targetCount==1);
  assert(RRAlbedoShader::ParsePixelContract(v.replay,v.replaySize,c,&why)&&c.targetCount==1);
#ifdef _WIN32
  ID3DBlob* disassembly=nullptr;
  assert(SUCCEEDED(D3DDisassemble(v.replay,v.replaySize,0,nullptr,&disassembly))&&disassembly);
  disassembly->Release();
#endif
  RRAlbedoShader::CapturedPixelShader ps;ps.bytes.assign(v.native,v.native+v.nativeSize);
  assert(RRWorldAlbedoVariant(ps)==int(i));ps.bytes.back()^=1;assert(RRWorldAlbedoVariant(ps)==-1);
 }
 assert(!RRWorldAlbedoReplacement(100,0));rrWorldAlbedoStorageReady=true;
 assert(!RRWorldAlbedoReplacement(0,0)&&!RRWorldAlbedoReplacement(100,-1)&&!RRWorldAlbedoReplacement(100,4));
 cloneOkay=false;assert(!RRWorldAlbedoReplacement(100,0)&&rrWorldAlbedoRecordCount==0);cloneOkay=true;
 auto first=RRWorldAlbedoReplacement(100,0);assert(first&&rrWorldAlbedoRecordCount==1);
 auto oldClones=clones;assert(RRWorldAlbedoReplacement(100,0)==first&&clones==oldClones);
 for(unsigned i=1;i<64;++i)assert(RRWorldAlbedoReplacement(100+i,int(i%4)));
 assert(rrWorldAlbedoRecordCount==64&&!RRWorldAlbedoReplacement(1000,0));
 assert(RRWorldAlbedoReplacement(100,0)==first); // Existing records remain usable at capacity.
 native_lighting::selection.Select(native_lighting::Mode::RtSpotWorldAlbedo);assert(RRWorldAlbedoRequested());
 native_lighting::selection.Select(native_lighting::Mode::RtSpotGiFiltered);assert(!RRWorldAlbedoRequested());
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'test.cpp';out=Path(d)/'test.exe';src.write_text(source)
    compiler=shutil.which('cl') or shutil.which('g++') or shutil.which('clang++')
    assert compiler
    if Path(compiler).stem.lower()=='cl':args=[compiler,'/nologo','/std:c++17','/EHsc',str(src),'/I'+str(root/'src'),'/Fe:'+str(out),'/Fo:'+str(Path(d)/'test.obj'),'d3dcompiler.lib']
    else:args=[compiler,'-std=c++17',str(src),'-I'+str(root/'src'),'-o',str(out)]
    subprocess.run(args,check=True);subprocess.run([str(out)],check=True)
print('PASS: exact world shader admission, alpha-only rewrite, native RGB/bindings, parser, bounded clone cache and mode gate')
