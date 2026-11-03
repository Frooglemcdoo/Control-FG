#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>
static bool hashes=true;
static bool PTReflectionSha256(const unsigned char*,size_t n,char* out){strcpy_s(out,65,hashes?(n==24472?"6297319e1fec315c06ee75993e797b51def658e7d41f1588d7df3feb41a1ea2f":"1ff8a93460d94f58fc0e70afff994a334f1c45be292e3e858a63f51070e5734a"):"unknown");return true;}
static void Log(const char*,...){}
namespace rt_spot_runtime {inline void OnState(ID3D12GraphicsCommandList4*,ID3D12StateObject*){} inline bool Dispatch(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*){return false;}}
namespace rt_shadow_capture {struct Hooks{void (*dispatch)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);};inline Hooks Lookup(ID3D12GraphicsCommandList4*){return {};}}
#include "../src/gi_sequence_runtime.h"
#include "../src/gi_live_toggle.h"
static void Require(bool v){if(!v)throw std::runtime_error("GI live toggle contract failed");}
static const D3D12_STATE_OBJECT_DESC* source=nullptr;
static unsigned augmentedCalls=0,nativeCalls=0;
static HRESULT STDMETHODCALLTYPE Next(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC* d,REFIID,void** out){
 if(d==source){++nativeCalls;*out=nullptr;return S_OK;}
 ++augmentedCalls;Require(d->NumSubobjects==source->NumSubobjects+1);
 Require(d->pSubobjects[0].pDesc==source->pSubobjects[0].pDesc);
 auto& a=*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(d->pSubobjects[2].pDesc);
 Require(a.pSubobjectToAssociate==d->pSubobjects+1&&a.NumExports==2&&!wcscmp(a.pExports[0],L"rayGenMain")&&!wcscmp(a.pExports[1],gi_live_toggle::kExport));
 auto& l=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(d->pSubobjects[3].pDesc);
 auto& old=*static_cast<const D3D12_DXIL_LIBRARY_DESC*>(source->pSubobjects[0].pDesc);
 Require(l.NumExports==1&&!wcscmp(l.pExports[0].ExportToRename,L"rayGenMain")&&!wcscmp(l.pExports[0].Name,gi_live_toggle::kExport));
 Require(l.DXILLibrary.pShaderBytecode==(old.DXILLibrary.BytecodeLength==24472?gi_sequence::kGILiveShader0:gi_sequence::kGILiveShader1));
 *out=nullptr;return E_FAIL; // Exercise clean fallback without a DXR device.
}
int main(){try{
 unsigned char byte=0;D3D12_DXIL_LIBRARY_DESC lib{{&byte,24472},0,nullptr};
 D3D12_RAYTRACING_SHADER_CONFIG config{4,8};LPCWSTR name=L"rayGenMain";
 D3D12_STATE_SUBOBJECT sub[3]={{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib},{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config},{D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,nullptr}};
 D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assoc{sub+1,1,&name};sub[2].pDesc=&assoc;
 D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,3,sub};source=&desc;void* out=nullptr;
 auto run=[&]{Require(SUCCEEDED(gi_live_toggle::Create(&Next,reinterpret_cast<ID3D12Device5*>(1),&desc,__uuidof(ID3D12StateObject),&out)));};
 run();Require(augmentedCalls==1&&nativeCalls==1&&assoc.NumExports==1&&assoc.pSubobjectToAssociate==sub+1);
 lib.DXILLibrary.BytecodeLength=27904;run();Require(augmentedCalls==2&&nativeCalls==2);
 hashes=false;run();Require(augmentedCalls==2&&nativeCalls==3);hashes=true;
 sub[1].Type=D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;run();Require(augmentedCalls==2&&nativeCalls==4);
 sub[1].Type=D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;assoc.pSubobjectToAssociate=nullptr;run();Require(augmentedCalls==2&&nativeCalls==5);
 const bool before=gi_live_toggle::requested.load();gi_live_toggle::Toggle();Require(gi_live_toggle::requested.load()!=before);gi_live_toggle::Toggle();Require(gi_live_toggle::requested.load()==before&&gi_live_toggle::revision==2);
 std::puts("PASS: both GI variants, native library preservation, export alias, associations, local-root rejection, failed creation fallback, toggle round trip");
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
