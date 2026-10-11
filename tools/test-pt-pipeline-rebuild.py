"""Exercise production native admission and descriptor relocation with mock D3D.
The state retains native bytecode, groups and export names; only extra raygen
and a compatible global root are added. No GPU claims are made here.
"""
from pathlib import Path
import tempfile, subprocess, os, shutil
root=Path(__file__).resolve().parents[1]
s=(root/'src/pt_reflection_third_runtime.h').read_text()
admit=s[s.index('static bool PTThirdIsNative('):s.index('static HRESULT PTThirdCreateState(')]
a=s.index('        const UINT n=desc->NumSubobjects;')
b=s.index('        if(!rootReady||!PTThirdEnsureBuffer',a)
block=s[a:b]
pre=r'''
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdio>
using UINT=unsigned;using LPCWSTR=const wchar_t*;
enum Kind {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
 D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,OTHER};
constexpr int D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE=3;
struct Code{const void* pShaderBytecode;size_t BytecodeLength;};
struct D3D12_DXIL_LIBRARY_DESC{Code DXILLibrary;};
struct D3D12_GLOBAL_ROOT_SIGNATURE{void* pGlobalRootSignature;};
struct D3D12_STATE_SUBOBJECT{Kind Type;const void* pDesc;};
struct D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION{const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate;UINT NumExports;const LPCWSTR* pExports;};
struct D3D12_STATE_OBJECT_DESC{int Type;UINT NumSubobjects;const D3D12_STATE_SUBOBJECT* pSubobjects;};
void* PTThirdGetRoot(void*,void*){return (void*)1234;}
bool PTReflectionSha256(const unsigned char* p,size_t,char* sha){strcpy(sha,p[0]==1?
 "c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e":"wrong");return true;}
'''
exercise=r'''
void Exercise(const D3D12_STATE_OBJECT_DESC* desc){
 void* device=nullptr;
'''+block+r'''
 assert(rootReady);
 assert(subs[0].pDesc==desc->pSubobjects[0].pDesc); // original DXIL descriptor untouched
 auto* a=(const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*)subs[2].pDesc;
 assert(a->pSubobjectToAssociate==&subs[3]);
 assert(a->NumExports==2&&!wcscmp(a->pExports[1],L"PTExtraRayGeneration"));
 assert(((const D3D12_GLOBAL_ROOT_SIGNATURE*)subs[1].pDesc)->pGlobalRootSignature==(void*)1234);
}
int main(){
 std::vector<unsigned char> bytes(39008,1);
 D3D12_DXIL_LIBRARY_DESC lib{{bytes.data(),bytes.size()}};
 D3D12_GLOBAL_ROOT_SIGNATURE root{(void*)99};
 D3D12_STATE_SUBOBJECT subs[4]{};
 LPCWSTR name=L"reflectionRayGeneration";
 D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assoc{&subs[3],1,&name};
 int config=4;
 subs[0]={D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib};
 subs[1]={D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&root};
 subs[2]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&assoc};
 subs[3]={D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config};
 D3D12_STATE_OBJECT_DESC desc{3,4,subs};
 assert(PTThirdIsNative(&desc));Exercise(&desc);
 assert(root.pGlobalRootSignature==(void*)99&&assoc.NumExports==1&&assoc.pSubobjectToAssociate==&subs[3]);
 assert(lib.DXILLibrary.pShaderBytecode==bytes.data()&&lib.DXILLibrary.BytecodeLength==39008);
 bytes[0]=0;assert(!PTThirdIsNative(&desc));
 bytes[0]=1;lib.DXILLibrary.BytecodeLength=39007;assert(!PTThirdIsNative(&desc));
 lib.DXILLibrary.BytecodeLength=39008;desc.Type=0;assert(!PTThirdIsNative(&desc));
 assert(!PTThirdIsNative(nullptr));
 puts("PASS: exact native admission; native DXIL descriptor preserved; relocated association; extra shader config; original input immutable");
}
'''
source=pre+'\n#include <cwchar>\n'+admit+exercise
with tempfile.TemporaryDirectory() as td:
    td=Path(td);cpp=td/'pipeline.cpp';cpp.write_text(source)
    exe=td/('pipeline.exe' if os.name=='nt' else 'pipeline')
    if os.name=='nt':
        subprocess.run(['cl','/nologo','/std:c++17','/EHsc',str(cpp),'/Fe'+str(exe),'/Fo'+str(td/'pipeline.obj')],check=True)
    else:
        subprocess.run([shutil.which('g++') or 'clang++','-std=c++17','-Wall','-Wextra',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
