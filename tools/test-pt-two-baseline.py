"""Lock the user-confirmed first two bounces and exercise optional-hit fallback."""
from pathlib import Path
import hashlib, tempfile, subprocess, shutil, os
root=Path(__file__).resolve().parents[1]
ray=(root/'src/shaders/pt_reflection_p2.hlsl').read_text()
prefix=ray.split('    // Native reflection targets',1)[1].split('        PTTrace(secondLayer,firstHitWorld+firstHitNormal*0.01f,secondDirection,secondPayload);',1)[0]
assert hashlib.sha256(prefix.encode()).hexdigest()=='c9e390ee8fc27af931050826848c1f6e43808ff8a354d880ac617e0c19944bb0'
s=(root/'src/shaders/pt_reflection_p2_native_composite.hlsl').read_text()
a=s.index('#include "pt_private_hits.hlsli"');b=s.index('float3 PTNativeViewToWorld',a)
baseline=s[:a]+s[b:]
a=baseline.index('                            // The original two-bounce output');b=baseline.index('                            if(!all(isfinite(replacement))',a)
baseline=baseline[:a]+baseline[b:]
baseline=baseline.replace('float3 secondRetained;','float3 unusedDirect;').replace('true,1.0f,secondRetained);','true,1.0f,unusedDirect);')
assert hashlib.sha256(baseline.encode()).hexdigest()=='336dfb99c933a822bb54f6c4f3f3f90c580c87a84aa213e1f2049bd8513694d5'
optional=s[s.index('// The original two-bounce output'):s.index('if(!all(isfinite(replacement))')]
assert 'rejection=' not in optional and 'return;' not in optional
loader=(root/'src/pt_reflection_loader.h').read_text()
fresh=loader[loader.index('const bool thirdFresh='):loader.index('    DWORD fault=0,restoreFault=0;',loader.index('const bool thirdFresh='))]
assert 'return;' not in fresh
assert 'SetComputeRoot32BitConstant(9,thirdFresh?1u:0u,0)' in loader
helper=(root/'src/shaders/pt_optional_third.hlsli').read_text()
source='''#include <cmath>
#include <cassert>
#include <cstring>
#include <limits>
#include <cstdio>
struct float3 {float x,y,z;};
'''+helper+'''
int main(){
  const float nan=std::numeric_limits<float>::quiet_NaN();
  const float inf=std::numeric_limits<float>::infinity();
  for(int i=0;i<10000;++i){
    float3 base{float(i)*.001f, -0.0f, float(i)*.003f};
    float3 valid{.25f,.5f,.75f};
    float3 cases[]={{nan,1,2},{1,nan,2},{1,2,nan},{inf,1,2},{1,-inf,2},{1,2,inf}};
    auto absent=PTOptionalThirdOutput(base,valid,false);
    assert(!memcmp(&base,&absent,sizeof(base)));
    for(auto candidate:cases){
      auto result=PTOptionalThirdOutput(base,candidate,true);
      assert(!memcmp(&base,&result,sizeof(base)));
    }
    auto accepted=PTOptionalThirdOutput(base,valid,true);
    assert(!memcmp(&valid,&accepted,sizeof(valid)));
  }
  puts("PASS: confirmed two-bounce source preserved; stale third cannot skip composite; missing/nonfinite third preserves output bit-for-bit; valid third selected");
}
'''
with tempfile.TemporaryDirectory() as td:
    td=Path(td);cpp=td/'optional.cpp';cpp.write_text(source)
    exe=td/('optional.exe' if os.name=='nt' else 'optional')
    args=['cl','/nologo','/std:c++17','/EHsc',str(cpp),'/Fe'+str(exe),'/Fo'+str(td/'optional.obj')] if os.name=='nt' else [shutil.which('g++') or 'clang++','-std=c++17',str(cpp),'-o',str(exe)]
    subprocess.run(args,check=True);subprocess.run([str(exe)],check=True)
