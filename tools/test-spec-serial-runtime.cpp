// Portable routing/ownership test; shader bytecode validation is a separate gate.
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
using HRESULT=std::int32_t;
using REFIID=int;
constexpr HRESULT E_FAIL=static_cast<HRESULT>(0x80004005u);
#define SUCCEEDED(hr) ((hr)>=0)
struct IUnknown { virtual unsigned long Release()=0; };
struct ID3D12Device {};
struct Bytecode { const void* pShaderBytecode; std::size_t BytecodeLength; };
struct Cached { const void* pCachedBlob; std::size_t CachedBlobSizeInBytes; };
struct D3D12_COMPUTE_PIPELINE_STATE_DESC { void* pRootSignature; Bytecode CS; Cached CachedPSO; unsigned NodeMask; unsigned Flags; };
static bool match=true;
static bool PTReflectionSha256(const unsigned char*,std::size_t,char* out){
 std::strcpy(out,match?"4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9":"unsupported");return true;
}
template<class... T> void Log(const char*,T...){}
#include "../src/spec_serial_runtime.h"
struct Object: IUnknown { unsigned releases=0; unsigned long Release() override {return ++releases;} } object;
static unsigned calls=0,behavior=0;
static HRESULT Create(ID3D12Device*,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,REFIID iid,void** out){
 ++calls;
 assert(desc->CS.pShaderBytecode==kSpecSerialLifetimesShader);
 assert(desc->CS.BytecodeLength==sizeof(kSpecSerialLifetimesShader));
 assert(!desc->CachedPSO.pCachedBlob&&!desc->CachedPSO.CachedBlobSizeInBytes);
 assert(desc->pRootSignature==&object&&desc->NodeMask==7&&desc->Flags==9&&iid==42);
 *out=behavior==2?nullptr:&object;
 return behavior==1?E_FAIL:0;
}
int main(){
 ID3D12Device device;
 unsigned char source=0;
 D3D12_COMPUTE_PIPELINE_STATE_DESC d{&object,{&source,42408},{&source,99},7,9};
 const auto original=d;
 void* out=&device; HRESULT hr=E_FAIL;
 assert(spec_serial::TryCreate(&device,&d,42,&out,&Create,false,hr));
 assert(out==&object&&hr==0&&calls==1&&spec_serial::directCreated==1);
 assert(d.CS.pShaderBytecode==original.CS.pShaderBytecode&&d.CachedPSO.pCachedBlob==original.CachedPSO.pCachedBlob);
 assert(spec_serial::TryCreate(&device,&d,42,&out,&Create,true,hr));
 assert(spec_serial::libraryCreated==1&&spec_serial::libraryMatched==1);
 // Both failure forms preserve the caller's existing output. Unexpected failed
 // non-null creations are released, never substituted into a native cache load.
 for(behavior=1;behavior<=2;++behavior){
  out=&device;
  assert(!spec_serial::TryCreate(&device,&d,42,&out,&Create,true,hr));
  assert(out==&device);
 }
 assert(object.releases==1&&spec_serial::failures==2&&spec_serial::libraryCreated==1);
 auto previous=calls; match=false;
 assert(!spec_serial::TryCreate(&device,&d,42,&out,&Create,false,hr));
 match=true; d.CS.BytecodeLength=1;
 assert(!spec_serial::TryCreate(&device,&d,42,&out,&Create,false,hr));
 d=original;
 assert(!spec_serial::TryCreate(&device,&d,42,nullptr,&Create,false,hr));
 assert(!spec_serial::TryCreate(nullptr,&d,42,&out,&Create,false,hr));
 assert(calls==previous);
 std::puts("PASS direct/cache creation, exact-target rejection, unchanged descriptor, failure preservation, COM cleanup and counters");
}
