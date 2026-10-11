"""Compile production native-library replacement with mock D3D descriptors.
Checks association relocation and exact-hash admission without a GPU.
"""
from pathlib import Path
import tempfile, subprocess, os, shutil
root=Path(__file__).resolve().parents[1]
s=(root/'src/pt_reflection_third_runtime.h').read_text()
a=s.index('    if(!target&&desc&&desc->pSubobjects')
b=s.index('    if(!target){',a)
block=s[a:b]
source=r'''
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdio>
using UINT=unsigned;using HRESULT=int;using REFIID=int;
constexpr int E_INVALIDARG=-1,E_OUTOFMEMORY=-2;
struct ID3D12Device5{};
enum Kind { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
 D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, OTHER };
struct Code { const void* pShaderBytecode;size_t BytecodeLength; };
struct D3D12_DXIL_LIBRARY_DESC {Code DXILLibrary;};
struct D3D12_STATE_SUBOBJECT { Kind Type;const void* pDesc;};
struct D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION {const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate;};
struct D3D12_STATE_OBJECT_DESC {UINT NumSubobjects;const D3D12_STATE_SUBOBJECT* pSubobjects;};
std::vector<unsigned char> ptReflectionP2Bytes(30256,2);
bool PTReflectionSha256(const unsigned char* p,size_t,char* sha){
 strcpy(sha,p[0]==1?"c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e":"wrong-hash");return true;
}
void Log(const char*,...){}
int captures=0;
HRESULT PTThirdCreateState(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC* desc,REFIID,void**){
 ++captures;
 const auto* lib=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[0].pDesc);
 assert(lib->DXILLibrary.pShaderBytecode==ptReflectionP2Bytes.data());
 assert(lib->DXILLibrary.BytecodeLength==30256);
 const auto* assoc=static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[1].pDesc);
 assert(assoc->pSubobjectToAssociate==&desc->pSubobjects[2]);
 return 7;
}
HRESULT Exercise(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out){
 bool target=false;
'''+block+r'''
 return 0;
}
int main(){
 std::vector<unsigned char> bytes(39008,1);
 D3D12_DXIL_LIBRARY_DESC lib{{bytes.data(),bytes.size()}};
 D3D12_STATE_SUBOBJECT subs[3]{};
 D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assoc{&subs[2]};
 int root=99;
 subs[0]={D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib};
 subs[1]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&assoc};
 subs[2]={OTHER,&root};
 D3D12_STATE_OBJECT_DESC desc{3,subs};
 assert(Exercise(nullptr,&desc,0,nullptr)==7&&captures==1);
 assert(lib.DXILLibrary.pShaderBytecode==bytes.data());
 assert(assoc.pSubobjectToAssociate==&subs[2]);
 bytes[0]=0;assert(Exercise(nullptr,&desc,0,nullptr)==0&&captures==1);
 bytes[0]=1;lib.DXILLibrary.BytecodeLength=39007;
 assert(Exercise(nullptr,&desc,0,nullptr)==0&&captures==1);
 puts("PASS: exact library admission, same-size wrong-hash rejection, size rejection, association relocation, input preservation");
}
'''
with tempfile.TemporaryDirectory() as td:
 td=Path(td);p=td/'rebuild.cpp';p.write_text(source);exe=td/('test.exe' if os.name=='nt' else 'test')
 cmd=['cl','/nologo','/std:c++17','/EHsc',str(p),'/Fe'+str(exe),'/Fo'+str(td/'test.obj')] if os.name=='nt' else [shutil.which('g++') or 'clang++','-std=c++17',str(p),'-o',str(exe)]
 subprocess.run(cmd,check=True);subprocess.run([str(exe)],check=True)
