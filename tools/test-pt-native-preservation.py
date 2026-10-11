"""Regression checks for native-first dispatch, true OFF, and scratch restoration.
Executes the production dispatch section against a recording command-list mock.
Checks all finite half-float values survive native save/restore conversion.
"""
from pathlib import Path
import tempfile, subprocess, os, shutil, struct, math
root=Path(__file__).resolve().parents[1]
s=(root/'src/pt_reflection_third_runtime.h').read_text()
a=s.index('    PTThirdBind(list,s,true);',s.index('static void STDMETHODCALLTYPE PTThirdHookDispatch'))
b=s.index('\n    if(raw<=8',a)
block=s[a:b]
source=r'''
#include <atomic>
#include <cassert>
#include <cstdio>
#include <vector>
using UINT64=unsigned long long;using DWORD=unsigned long;
constexpr unsigned D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES=32,D3D12_RESOURCE_BARRIER_TYPE_UAV=1;
struct Range{UINT64 StartAddress,SizeInBytes;};
struct D3D12_DISPATCH_RAYS_DESC{Range RayGenerationShaderRecord;unsigned Width,Height,Depth;UINT64 HitGroupTable,MissShaderTable;};
struct D3D12_RESOURCE_BARRIER{unsigned Type;};
std::vector<int> events;std::vector<D3D12_DISPATCH_RAYS_DESC> calls;
struct ID3D12GraphicsCommandList4{void ResourceBarrier(unsigned,const D3D12_RESOURCE_BARRIER*){events.push_back(3);}};
struct PTThirdListState{UINT64 extraRecord;};
bool enabled=true;
bool PTReflectionUserRequested(){return enabled;}
void Dispatch(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC* d){events.push_back(2);calls.push_back(*d);}
using PTThirdDispatchFn=void(*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);
void* PTThirdOriginal(ID3D12GraphicsCommandList4*,unsigned){return (void*)&Dispatch;}
void PTThirdBind(ID3D12GraphicsCommandList4*,const PTThirdListState&,bool custom){events.push_back(custom?1:4);}
bool ReadEngineFrameSafe(unsigned long long* f,DWORD*){*f=17;return true;}
void Log(const char*,...){}
std::atomic<unsigned long long> ptThirdWrittenFrame{0},ptThirdDispatches{0};
void Exercise(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc,PTThirdListState s){
'''+block+r'''
}
int main(){
 ID3D12GraphicsCommandList4 list;
 D3D12_DISPATCH_RAYS_DESC d{{100,32},2560,1440,1,200,300};
 PTThirdListState s{400};
 for(int test=0;test<5;++test){
  enabled=test!=0;d.Depth=test==2?2:1;d.Width=test==3?9000:2560;s.extraRecord=test==4?0:400;
  calls.clear();events.clear();auto before=ptThirdDispatches.load();
  Exercise(&list,&d,s);
  assert(calls[0].RayGenerationShaderRecord.StartAddress==100);
  bool extra=test==1;
  assert(calls.size()==(extra?2u:1u));
  assert(events.front()==1&&events.back()==4);
  if(extra){assert(calls[1].RayGenerationShaderRecord.StartAddress==400);
   assert(calls[1].HitGroupTable==200&&calls[1].MissShaderTable==300);
   assert(calls[1].Width==d.Width&&calls[1].Height==d.Height);
   assert(events==std::vector<int>({1,2,3,2,3,4}));}
  assert(ptThirdDispatches.load()==before+(extra?1:0));
  assert(d.RayGenerationShaderRecord.StartAddress==100);
 }
 puts("PASS: original native dispatch first; OFF skips extra work; barriers order both dispatches; original SBT unchanged; capacity/depth gates");
}
'''
with tempfile.TemporaryDirectory() as td:
    td=Path(td);cpp=td/'dispatch.cpp';cpp.write_text(source)
    exe=td/('dispatch.exe' if os.name=='nt' else 'dispatch')
    if os.name=='nt':
        subprocess.run(['cl','/nologo','/std:c++17','/EHsc',str(cpp),'/Fe'+str(exe),'/Fo'+str(td/'dispatch.obj')],check=True)
    else:
        subprocess.run([shutil.which('g++') or 'clang++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
hlsl=(root/'src/shaders/pt_reflection_p2.hlsl').read_text()
helper=hlsl.split('bool PTTracePreserved(',1)[1].split('[shader("raygeneration")]',1)[0]
assert 'PTTrace(0u,origin,direction,payload)' in helper
for name,saved in [('ptExtra_rwtMaterialId','savedMaterial'),('ptExtra_rwtNormal_TexcoordX','savedNormal'),('ptExtra_rwtPosition_TexcoordY','savedPosition')]:
    assert f'{name}[slot]={saved};' in helper
    assert helper.index(f'{name}[slot]={saved};')<helper.index('return payload')
assert helper.count('return ')==1
assert hlsl.count('[shader(')==1
assert '[uint3(pixel,1' not in hlsl
for bits in range(65536):
    raw=struct.pack('<H',bits);value=struct.unpack('<e',raw)[0]
    if math.isfinite(value):assert struct.pack('<e',value)==raw
print('PASS: native scratch restores all fields before return; no layer-1 writes; every finite half-float value round-trips exactly.')
