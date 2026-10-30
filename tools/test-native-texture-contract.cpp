#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dxgi1_4.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
static void Log(const char*,...){}
static bool HashMatches(HMODULE,const char*){return false;}
static void* AllocateExecutableRelayNear(const void*){return nullptr;}
static bool Rel32Fits(const void*,const void*){return false;}
#include "../src/native_cpu_call_patch.h"
#include "../src/native_loader_trace_runtime.h"
#include "../src/native_texture_contract_runtime.h"
namespace tc=native_texture_contract;
using U64=std::uint64_t;
static void* owner=reinterpret_cast<void*>(0x1234);
static void* texture=reinterpret_cast<void*>(0x5678);
static void* container=reinterpret_cast<void*>(0x7890);
static char name[]="texture",buffer[32]{};
static unsigned char record[0x188]{},publication[16]{};
static U64 nextId=7;
static bool nested=false;
static void Put(void* p,unsigned offset,U64 value){memcpy(static_cast<unsigned char*>(p)+offset,&value,8);}
static void Inspect(void* t,unsigned level,tc::Snapshot& s){
 assert(t==texture&&level==2);s.container=reinterpret_cast<U64>(container);SetLastError(900);
}
static std::int64_t Request(void* o,const char* n,void* listener,U64 offset,U64 bytes,char* b,int thread,int priority,bool flag,void* user){
 assert(o==owner&&n==name&&listener==texture&&offset==123&&bytes==456&&b==buffer&&thread==14&&priority==-7&&flag&&user==reinterpret_cast<void*>(2));
 assert(GetLastError()==101);
 if(nested){
  nested=false;auto* before=tc::pending;assert(before);tc::requests=8;
  assert(tc::RequestHook(o,n,listener,offset,bytes,b,thread,priority,flag,user)==static_cast<std::int64_t>(nextId));
  assert(tc::pending==before); // unsampled nested call must not consume outer metadata
 }
 Put(record,0x138,reinterpret_cast<U64>(listener));Put(record,0x140,2);Put(record,0x178,nextId);
 tc::Construct(reinterpret_cast<U64>(o),record);
 SetLastError(202);return static_cast<std::int64_t>(nextId);
}
static void Submit(){SetLastError(101);assert(tc::RequestHook(owner,name,texture,123,456,buffer,14,-7,true,reinterpret_cast<void*>(2))==static_cast<std::int64_t>(nextId));assert(GetLastError()==202&&tc::pending==nullptr);}
static void Upload(void* f,unsigned mode,void* c){assert(f==buffer&&mode==0&&c==container&&GetLastError()==301);SetLastError(302);}
static void Publish(void* o,void* r){assert(o==owner&&r==publication&&GetLastError()==401);SetLastError(402);}
static void Finish(){SetLastError(401);tc::PublishHook(owner,publication);assert(GetLastError()==402);}
static BOOL WINAPI FailedRead(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED){SetLastError(501);return FALSE;}
static bool observedFailure=false;
static void ObserveFailure(HANDLE,const loader_candidate::Candidate&,DWORD received,bool success){assert(!success&&received==0);observedFailure=true;SetLastError(999);}
int main(){
 tc::inspector=&Inspect;tc::request=&Request;tc::upload=&Upload;tc::publish=&Publish;
 Put(publication,0,reinterpret_cast<U64>(texture));Put(publication,8,2);publication[12]=1;
 Submit();assert(tc::constructed==1&&tc::entries[7].occupied);
 loader_candidate::Candidate actual{7,reinterpret_cast<U64>(owner),99,0,{123,456}};
 auto wrong=actual;wrong.owner++;tc::Read(INVALID_HANDLE_VALUE,wrong,456,true);assert(tc::reads==0);
 tc::Read(INVALID_HANDLE_VALUE,actual,456,true);tc::Read(INVALID_HANDLE_VALUE,actual,456,true);assert(tc::reads==1);
 SetLastError(301);tc::UploadHook(buffer,0,container);assert(GetLastError()==302&&tc::uploads==1);
 Finish();assert(tc::queued==1&&!tc::entries[7].occupied);Finish();assert(tc::queued==1);
 nextId=8;nested=true;Submit();assert(tc::constructed==2&&tc::entries[8].occupied);Finish();
 // Same request/publication slot cannot overwrite an outstanding observation.
 tc::requests=0;nextId=9;Submit();nextId=265;Submit();assert(tc::entries[9].data.id==9&&tc::dropped==1);Finish();
 // Two native requests sharing a container must not be attributed arbitrarily.
 tc::entries[10].occupied=tc::entries[11].occupied=true;
 tc::entries[10].data.container=tc::entries[11].data.container=reinterpret_cast<U64>(container);
 SetLastError(301);tc::UploadHook(buffer,0,container);assert(GetLastError()==302&&!tc::entries[10].upload&&!tc::entries[11].upload&&tc::dropped==2);
 tc::entries[10]={};tc::entries[11]={};
 // Bounded metadata sampling, with completed entries retired between requests.
 tc::requests=0;tc::sampleAttempts=0;tc::captured=0;
 for(unsigned i=0;i<7000;++i){nextId=1000+i;Submit();Finish();}
 assert(tc::captured==96);
 // A failed native read must not expose an untouched output byte count.
 native_loader_stages::read=&FailedRead;native_loader_stages::observeRead=&ObserveFailure;
 {native_loader_stages::WorkerScope scope(owner,record);scope.context.reading=true;DWORD received=0xdeadbeef;
  assert(!native_loader_stages::ReadHook(INVALID_HANDLE_VALUE,buffer,32,&received,nullptr)&&GetLastError()==501&&observedFailure);}
 using Microsoft::WRL::ComPtr;ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
 assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
 assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=129;desc.Height=65;desc.DepthOrArraySize=1;desc.MipLevels=8;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;
 native_texture_layout::Layout layout{};assert(native_texture_layout::Build(device.Get(),desc,layout));
 assert(layout.count==8&&layout.mips[0].rowBytes==516&&layout.mips[0].rowPitch==768&&layout.mips[0].compactBytes==33540&&layout.directMips<8);
 for(unsigned i=1;i<8;++i)assert(layout.mips[i].compactOffset==layout.mips[i-1].compactOffset+layout.mips[i-1].compactBytes);
 assert(layout.mips[7].compactBytes==4&&layout.mips[7].footprintBytes==4);
 desc.Width=128;desc.Height=64;desc.Format=DXGI_FORMAT_BC7_UNORM;assert(native_texture_layout::Build(device.Get(),desc,layout));
 assert(layout.mips[0].rowBytes==512&&layout.mips[0].rows==16&&layout.mips[0].compactBytes==8192&&layout.mips[7].compactBytes==16);
 desc.DepthOrArraySize=6;assert(!native_texture_layout::Build(device.Get(),desc,layout)&&layout.count==0);
 desc.DepthOrArraySize=1;desc.SampleDesc.Count=4;assert(!native_texture_layout::Build(device.Get(),desc,layout));
 desc.SampleDesc.Count=1;desc.MipLevels=17;assert(!native_texture_layout::Build(device.Get(),desc,layout));
 assert(!native_texture_layout::Build(nullptr,desc,layout));
 puts("PASS R240: native ABI forwarding, LastError, nested requests, owner identity, collisions, ambiguous uploads, bounded sampling, failed reads, RGBA/BC7 D3D12 footprints; native_gpu_redirect=0");
}
