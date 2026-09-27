#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

static void PT1AfterNativeReflection(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* nativeDesc,unsigned long long frame,std::uint64_t signature) noexcept;

namespace control_pt0 {

static constexpr UINT MaxHeaps=96;
static constexpr UINT MaxCommandTables=32;
static constexpr UINT MaxDescriptorsPerCensus=16;
static constexpr UINT MaxASRecords=512;
static constexpr UINT MaxPassRecords=64;

enum class DescriptorKind : unsigned char { Empty=0,CBV=1,SRV=2,UAV=3 };

struct DescriptorMeta {
 DescriptorKind kind=DescriptorKind::Empty;
 void* resource=nullptr;
 DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
 UINT dimension=0;
 D3D12_GPU_VIRTUAL_ADDRESS gpuva=0;
 UINT64 width=0;
 UINT height=0;
 UINT sizeBytes=0;
};

struct HeapRecord {
 ID3D12DescriptorHeap* heap=nullptr;
 D3D12_DESCRIPTOR_HEAP_TYPE type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
 UINT count=0,increment=0;
 SIZE_T cpuStart=0;
 UINT64 gpuStart=0;
 bool shaderVisible=false;
 DescriptorMeta* meta=nullptr;
};

struct ASRecord {
 D3D12_GPU_VIRTUAL_ADDRESS address=0;
 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
 UINT numDescs=0;
 UINT flags=0;
 unsigned long long lastFrame=0;
};

struct PassRecord {
 std::uint64_t signature=0;
 unsigned long long count=0,firstFrame=0,lastFrame=0;
 UINT width=0,height=0,depth=0;
 char raygen[96]{};
};

struct Semantic {
 bool active=false;
 int rayA=0,rayB=0,pipelineA=0,pipelineB=0;
 unsigned long long frame=0;
 char raygen[96]{"<unset>"};
};

struct CommandState {
 ID3D12GraphicsCommandList4* list=nullptr;
 ID3D12StateObject* stateObject=nullptr;
 ID3D12PipelineState* pipeline=nullptr;
 ID3D12RootSignature* root=nullptr;
 ID3D12DescriptorHeap* heaps[2]{};
 UINT heapCount=0;
 D3D12_GPU_DESCRIPTOR_HANDLE tables[MaxCommandTables]{};
 D3D12_GPU_VIRTUAL_ADDRESS cbv[MaxCommandTables]{};
 D3D12_GPU_VIRTUAL_ADDRESS srv[MaxCommandTables]{};
 D3D12_GPU_VIRTUAL_ADDRESS uav[MaxCommandTables]{};
 UINT constants[MaxCommandTables][64]{};
 std::uint64_t constantsHash[MaxCommandTables]{};
 UINT constantsCount[MaxCommandTables]{};
};

static SRWLOCK lock=SRWLOCK_INIT;
static HeapRecord heaps[MaxHeaps]{};
static ASRecord accel[MaxASRecords]{};
static PassRecord passes[MaxPassRecords]{};
static std::atomic<unsigned long long> dispatchCount{0},buildCount{0},tlasMatches{0},descriptorEvents{0};
static std::atomic<D3D12_GPU_VIRTUAL_ADDRESS> latestTlas{0};
static std::atomic<unsigned long long> verboseUntilPresent{0};
static std::atomic<unsigned int> deviceHookReady{0},commandHookReady{0};
static thread_local CommandState command{};
static thread_local Semantic semantic{};
static bool f7WasDown=false;

using CreateHeapFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_DESCRIPTOR_HEAP_DESC*,REFIID,void**);
using CreateCBVFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_CONSTANT_BUFFER_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateSRVFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,const D3D12_SHADER_RESOURCE_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CreateUAVFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Resource*,ID3D12Resource*,const D3D12_UNORDERED_ACCESS_VIEW_DESC*,D3D12_CPU_DESCRIPTOR_HANDLE);
using CopyDescriptorsFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,UINT,const D3D12_CPU_DESCRIPTOR_HANDLE*,const UINT*,D3D12_DESCRIPTOR_HEAP_TYPE);
using CopyDescriptorsSimpleFn=void(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_DESCRIPTOR_HEAP_TYPE);
using DispatchComputeFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT);
using SetPipelineFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using SetHeapsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,ID3D12DescriptorHeap*const*);
using SetRootFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
using SetTableFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using SetConstantsFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,UINT,const void*,UINT);
using SetGpuVaFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_VIRTUAL_ADDRESS);
using BuildASFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*,UINT,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*);
using SetStateObjectFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,ID3D12StateObject*);
using DispatchRaysFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*);

struct DeviceHooks {
 void** table=nullptr;
 CreateHeapFn createHeap=nullptr;CreateCBVFn createCBV=nullptr;CreateSRVFn createSRV=nullptr;CreateUAVFn createUAV=nullptr;
 CopyDescriptorsFn copyDescriptors=nullptr;CopyDescriptorsSimpleFn copySimple=nullptr;
};
static DeviceHooks deviceHooks{};

struct CommandHooks {
 void** table=nullptr;
 DispatchComputeFn dispatchCompute=nullptr;SetPipelineFn setPipeline=nullptr;
 SetHeapsFn setHeaps=nullptr;SetRootFn setRoot=nullptr;SetTableFn setTable=nullptr;SetConstantsFn setConstants=nullptr;
 SetGpuVaFn setCBV=nullptr,setSRV=nullptr,setUAV=nullptr;
 BuildASFn buildAS=nullptr;SetStateObjectFn setState=nullptr;DispatchRaysFn dispatch=nullptr;
};
static CommandHooks commandHooks{};

static bool PatchSlot(void** slot,void* replacement) noexcept {
 DWORD old=0;if(!slot||!replacement||!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return false;
 InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(slot),replacement);
 DWORD ignored=0;const bool protect=VirtualProtect(slot,sizeof(void*),old,&ignored)!=FALSE;
 const bool flush=FlushInstructionCache(GetCurrentProcess(),slot,sizeof(void*))!=FALSE;
 return protect&&flush;
}
static std::uint64_t HashBytes(const void* data,size_t bytes,std::uint64_t h=1469598103934665603ull) noexcept {
 const auto* p=static_cast<const unsigned char*>(data);for(size_t i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;
}
static std::uint64_t HashString(const char* s,std::uint64_t h=1469598103934665603ull) noexcept {
 if(!s)return h;for(size_t i=0;s[i]&&i<95;++i){h^=static_cast<unsigned char>(s[i]);h*=1099511628211ull;}return h;
}
static bool Verbose() noexcept {return presentCount.load()<=verboseUntilPresent.load(std::memory_order_acquire);}
static const char* Classify(const char* name) noexcept {
 if(!name)return "unknown";
 if(strstr(name,"contact")||strstr(name,"Contact"))return "contact_shadow";
 if(strstr(name,"shadow")||strstr(name,"Shadow"))return "shadow";
 if(strstr(name,"diffuse")||strstr(name,"Diffuse")||strstr(name,"dgi")||strstr(name,"DGI"))return "diffuse_gi";
 if(strstr(name,"reflection")||strstr(name,"Reflection")||strstr(name,"reflect")||strstr(name,"Reflect"))return "reflection";
 return "unknown";
}
static unsigned long long EngineFrame() noexcept {unsigned long long f=0;DWORD e=0;return ReadEngineFrameSafe(&f,&e)?f:0;}

static HeapRecord* ResolveCpu(SIZE_T ptr,UINT* index=nullptr) noexcept {
 for(auto& h:heaps){
  if(!h.heap||!h.increment||!h.count||ptr<h.cpuStart)continue;
  const SIZE_T delta=ptr-h.cpuStart;const UINT i=static_cast<UINT>(delta/h.increment);
  if(i<h.count&&SIZE_T(i)*h.increment==delta){if(index)*index=i;return &h;}
 }
 return nullptr;
}
static HeapRecord* ResolveGpu(UINT64 ptr,UINT* index=nullptr) noexcept {
 for(auto& h:heaps){
  if(!h.heap||!h.shaderVisible||!h.increment||!h.count||ptr<h.gpuStart)continue;
  const UINT64 delta=ptr-h.gpuStart;const UINT i=static_cast<UINT>(delta/h.increment);
  if(i<h.count&&UINT64(i)*h.increment==delta){if(index)*index=i;return &h;}
 }
 return nullptr;
}
static DescriptorMeta* MetaCpu(D3D12_CPU_DESCRIPTOR_HANDLE handle) noexcept {
 UINT index=0;auto* h=ResolveCpu(handle.ptr,&index);return h&&h->meta?&h->meta[index]:nullptr;
}
static void FillResource(DescriptorMeta& m,ID3D12Resource* r) noexcept {
 m.resource=r;if(!r)return;
 __try {const auto d=r->GetDesc();m.format=d.Format;m.dimension=static_cast<UINT>(d.Dimension);m.width=d.Width;m.height=d.Height;if(d.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER)m.gpuva=r->GetGPUVirtualAddress();}
 __except(EXCEPTION_EXECUTE_HANDLER){}
}
static void RememberHeap(ID3D12Device* device,ID3D12DescriptorHeap* heap) noexcept {
 if(!device||!heap)return;const auto d=heap->GetDesc();
 HeapRecord rec{};rec.heap=heap;rec.type=d.Type;rec.count=d.NumDescriptors;rec.increment=device->GetDescriptorHandleIncrementSize(d.Type);rec.cpuStart=heap->GetCPUDescriptorHandleForHeapStart().ptr;
 rec.shaderVisible=(d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)!=0;rec.gpuStart=rec.shaderVisible?heap->GetGPUDescriptorHandleForHeapStart().ptr:0;
 if(rec.count&&rec.count<=262144)rec.meta=static_cast<DescriptorMeta*>(HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DescriptorMeta)*rec.count));
 AcquireSRWLockExclusive(&lock);for(auto& h:heaps)if(!h.heap){h=rec;rec.heap=nullptr;break;}ReleaseSRWLockExclusive(&lock);
 if(rec.heap&&rec.meta)HeapFree(GetProcessHeap(),0,rec.meta);
 Log("PT0_DESCRIPTOR_HEAP heap=%p type=%u count=%u shader_visible=%u cpu_start=0x%llX gpu_start=0x%llX increment=%u",
  heap,unsigned(d.Type),d.NumDescriptors,unsigned((d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)!=0),
  static_cast<unsigned long long>(heap->GetCPUDescriptorHandleForHeapStart().ptr),
  static_cast<unsigned long long>((d.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)?heap->GetGPUDescriptorHandleForHeapStart().ptr:0),device->GetDescriptorHandleIncrementSize(d.Type));
}
static void RememberBoundHeap(ID3D12DescriptorHeap* heap) noexcept {
 if(!heap)return;
 AcquireSRWLockShared(&lock);bool known=false;for(const auto& h:heaps)if(h.heap==heap){known=true;break;}ReleaseSRWLockShared(&lock);
 if(known)return;ID3D12Device* d=nullptr;if(SUCCEEDED(heap->GetDevice(IID_PPV_ARGS(&d)))&&d){RememberHeap(d,heap);d->Release();}
}
static HRESULT STDMETHODCALLTYPE CreateHeapHook(ID3D12Device* d,const D3D12_DESCRIPTOR_HEAP_DESC* desc,REFIID iid,void** out) {
 const HRESULT hr=deviceHooks.createHeap(d,desc,iid,out);if(SUCCEEDED(hr)&&out&&*out)RememberHeap(d,static_cast<ID3D12DescriptorHeap*>(*out));return hr;
}
static void STDMETHODCALLTYPE CreateCBVHook(ID3D12Device* d,const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst) {
 deviceHooks.createCBV(d,desc,dst);AcquireSRWLockExclusive(&lock);if(auto* m=MetaCpu(dst)){*m={};m->kind=DescriptorKind::CBV;if(desc){m->gpuva=desc->BufferLocation;m->sizeBytes=desc->SizeInBytes;}}ReleaseSRWLockExclusive(&lock);++descriptorEvents;
}
static void STDMETHODCALLTYPE CreateSRVHook(ID3D12Device* d,ID3D12Resource* r,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst) {
 deviceHooks.createSRV(d,r,desc,dst);AcquireSRWLockExclusive(&lock);if(auto* m=MetaCpu(dst)){*m={};m->kind=DescriptorKind::SRV;FillResource(*m,r);if(desc){m->format=desc->Format;m->dimension=static_cast<UINT>(desc->ViewDimension);if(desc->ViewDimension==D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)m->gpuva=desc->RaytracingAccelerationStructure.Location;}}ReleaseSRWLockExclusive(&lock);++descriptorEvents;
}
static void STDMETHODCALLTYPE CreateUAVHook(ID3D12Device* d,ID3D12Resource* r,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE dst) {
 deviceHooks.createUAV(d,r,counter,desc,dst);AcquireSRWLockExclusive(&lock);if(auto* m=MetaCpu(dst)){*m={};m->kind=DescriptorKind::UAV;FillResource(*m,r);if(desc){m->format=desc->Format;m->dimension=static_cast<UINT>(desc->ViewDimension);}}ReleaseSRWLockExclusive(&lock);++descriptorEvents;
}
static void CopyMeta(D3D12_CPU_DESCRIPTOR_HANDLE dst,D3D12_CPU_DESCRIPTOR_HANDLE src,D3D12_DESCRIPTOR_HEAP_TYPE type) noexcept {
 UINT di=0,si=0;auto* dh=ResolveCpu(dst.ptr,&di);auto* sh=ResolveCpu(src.ptr,&si);if(!dh||!sh||dh->type!=type||sh->type!=type||!dh->meta||!sh->meta)return;dh->meta[di]=sh->meta[si];
}
static void STDMETHODCALLTYPE CopySimpleHook(ID3D12Device* d,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE dst,D3D12_CPU_DESCRIPTOR_HANDLE src,D3D12_DESCRIPTOR_HEAP_TYPE type) {
 deviceHooks.copySimple(d,count,dst,src,type);AcquireSRWLockExclusive(&lock);UINT di=0,si=0;auto* dh=ResolveCpu(dst.ptr,&di);auto* sh=ResolveCpu(src.ptr,&si);
 if(dh&&sh&&dh->type==type&&sh->type==type&&dh->meta&&sh->meta){const UINT n=(count<(dh->count-di)?count:(dh->count-di));const UINT n2=(n<(sh->count-si)?n:(sh->count-si));for(UINT i=0;i<n2;++i)dh->meta[di+i]=sh->meta[si+i];}
 ReleaseSRWLockExclusive(&lock);
}
static void STDMETHODCALLTYPE CopyDescriptorsHook(ID3D12Device* d,UINT nd,const D3D12_CPU_DESCRIPTOR_HANDLE* ds,const UINT* dsz,UINT ns,const D3D12_CPU_DESCRIPTOR_HANDLE* ss,const UINT* ssz,D3D12_DESCRIPTOR_HEAP_TYPE type) {
 deviceHooks.copyDescriptors(d,nd,ds,dsz,ns,ss,ssz,type);AcquireSRWLockExclusive(&lock);
 UINT dr=0,sr=0,doff=0,soff=0,copied=0;while(dr<nd&&sr<ns&&copied<4096){const UINT dn=dsz?dsz[dr]:1,sn=ssz?ssz[sr]:1;if(doff>=dn){++dr;doff=0;continue;}if(soff>=sn){++sr;soff=0;continue;}D3D12_CPU_DESCRIPTOR_HANDLE dh=ds[dr],sh=ss[sr];HeapRecord* dhr=ResolveCpu(dh.ptr);HeapRecord* shr=ResolveCpu(sh.ptr);if(dhr)dh.ptr+=SIZE_T(doff)*dhr->increment;if(shr)sh.ptr+=SIZE_T(soff)*shr->increment;CopyMeta(dh,sh,type);++doff;++soff;++copied;}ReleaseSRWLockExclusive(&lock);
}

static void OnDevice(IUnknown* unknown) noexcept {
 if(!unknown||deviceHookReady.load())return;ID3D12Device* d=nullptr;if(FAILED(unknown->QueryInterface(IID_PPV_ARGS(&d)))||!d)return;auto** t=*reinterpret_cast<void***>(d);
 DeviceHooks h{};h.table=t;h.createHeap=reinterpret_cast<CreateHeapFn>(t[14]);h.createCBV=reinterpret_cast<CreateCBVFn>(t[17]);h.createSRV=reinterpret_cast<CreateSRVFn>(t[18]);h.createUAV=reinterpret_cast<CreateUAVFn>(t[19]);h.copyDescriptors=reinterpret_cast<CopyDescriptorsFn>(t[23]);h.copySimple=reinterpret_cast<CopyDescriptorsSimpleFn>(t[24]);
 bool ok=h.createHeap&&h.createCBV&&h.createSRV&&h.createUAV&&h.copyDescriptors&&h.copySimple;
 if(ok){deviceHooks=h;ok=PatchSlot(&t[14],reinterpret_cast<void*>(&CreateHeapHook))&&PatchSlot(&t[17],reinterpret_cast<void*>(&CreateCBVHook))&&PatchSlot(&t[18],reinterpret_cast<void*>(&CreateSRVHook))&&PatchSlot(&t[19],reinterpret_cast<void*>(&CreateUAVHook))&&PatchSlot(&t[23],reinterpret_cast<void*>(&CopyDescriptorsHook))&&PatchSlot(&t[24],reinterpret_cast<void*>(&CopySimpleHook));}
 deviceHookReady.store(ok?1u:2u);Log("PT0_DEVICE_HOOKS ready=%u device=%p table=%p descriptor_capture=%u",unsigned(ok),d,t,unsigned(ok));d->Release();
}

static CommandHooks HooksFor(ID3D12GraphicsCommandList4* list) noexcept {CommandHooks h{};auto** t=*reinterpret_cast<void***>(list);AcquireSRWLockShared(&lock);if(commandHooks.table==t)h=commandHooks;ReleaseSRWLockShared(&lock);return h;}
static void ResetCommand(ID3D12GraphicsCommandList4* list) noexcept {if(command.list!=list){command={};command.list=list;}}

static void STDMETHODCALLTYPE SetPipelineHook(ID3D12GraphicsCommandList* l,ID3D12PipelineState* p){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setPipeline)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));command.pipeline=p;h.setPipeline(l,p);
}
static void STDMETHODCALLTYPE SetHeapsHook(ID3D12GraphicsCommandList* l,UINT n,ID3D12DescriptorHeap*const* hs){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setHeaps)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 command.heapCount=n>2?2:n;std::memset(command.tables,0,sizeof(command.tables));
 for(UINT i=0;i<2;++i){command.heaps[i]=i<command.heapCount?hs[i]:nullptr;if(i<command.heapCount)RememberBoundHeap(hs[i]);}
 h.setHeaps(l,n,hs);
}
static void STDMETHODCALLTYPE SetRootHook(ID3D12GraphicsCommandList* l,ID3D12RootSignature* r){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setRoot)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 if(command.root!=r){
  std::memset(command.tables,0,sizeof(command.tables));std::memset(command.cbv,0,sizeof(command.cbv));std::memset(command.srv,0,sizeof(command.srv));std::memset(command.uav,0,sizeof(command.uav));
  std::memset(command.constants,0,sizeof(command.constants));std::memset(command.constantsCount,0,sizeof(command.constantsCount));std::memset(command.constantsHash,0,sizeof(command.constantsHash));
 }
 command.root=r;h.setRoot(l,r);
}
static void STDMETHODCALLTYPE SetTableHook(ID3D12GraphicsCommandList* l,UINT i,D3D12_GPU_DESCRIPTOR_HANDLE v){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setTable)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 if(i<MaxCommandTables){command.tables[i]=v;command.cbv[i]=command.srv[i]=command.uav[i]=0;command.constantsCount[i]=0;}h.setTable(l,i,v);
}
static void STDMETHODCALLTYPE SetConstantsHook(ID3D12GraphicsCommandList* l,UINT i,UINT n,const void* data,UINT offset){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setConstants)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 if(i<MaxCommandTables&&data&&offset<64){
  command.tables[i]={};command.cbv[i]=command.srv[i]=command.uav[i]=0;
  const UINT copy=(n<64-offset)?n:64-offset;std::memcpy(&command.constants[i][offset],data,size_t(copy)*4);
  const UINT end=offset+copy;if(end>command.constantsCount[i])command.constantsCount[i]=end;
  command.constantsHash[i]=HashBytes(command.constants[i],size_t(command.constantsCount[i])*4);
 }
 h.setConstants(l,i,n,data,offset);
}
static void STDMETHODCALLTYPE SetCBVHook(ID3D12GraphicsCommandList* l,UINT i,D3D12_GPU_VIRTUAL_ADDRESS v){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setCBV)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 if(i<MaxCommandTables){command.cbv[i]=v;command.tables[i]={};command.srv[i]=command.uav[i]=0;command.constantsCount[i]=0;}h.setCBV(l,i,v);
}
static void STDMETHODCALLTYPE SetSRVHook(ID3D12GraphicsCommandList* l,UINT i,D3D12_GPU_VIRTUAL_ADDRESS v){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setSRV)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 if(i<MaxCommandTables){command.srv[i]=v;command.tables[i]={};command.cbv[i]=command.uav[i]=0;command.constantsCount[i]=0;}h.setSRV(l,i,v);
}
static void STDMETHODCALLTYPE SetUAVHook(ID3D12GraphicsCommandList* l,UINT i,D3D12_GPU_VIRTUAL_ADDRESS v){
 auto h=HooksFor(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));if(!h.setUAV)return;ResetCommand(reinterpret_cast<ID3D12GraphicsCommandList4*>(l));
 if(i<MaxCommandTables){command.uav[i]=v;command.tables[i]={};command.cbv[i]=command.srv[i]=0;command.constantsCount[i]=0;}h.setUAV(l,i,v);
}

static bool IsKnownTLAS(D3D12_GPU_VIRTUAL_ADDRESS va) noexcept {for(const auto& a:accel)if(a.address==va&&a.type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)return true;return false;}
static void STDMETHODCALLTYPE BuildASHook(ID3D12GraphicsCommandList4* l,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d,UINT n,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* p){
 auto h=HooksFor(l);if(!h.buildAS)return;const auto frame=EngineFrame();if(d){AcquireSRWLockExclusive(&lock);ASRecord* slot=nullptr;for(auto& a:accel)if(a.address==d->DestAccelerationStructureData){slot=&a;break;}if(!slot)for(auto& a:accel)if(!a.address){slot=&a;break;}if(slot){slot->address=d->DestAccelerationStructureData;slot->type=d->Inputs.Type;slot->numDescs=d->Inputs.NumDescs;slot->flags=unsigned(d->Inputs.Flags);slot->lastFrame=frame;}if(d->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)latestTlas.store(d->DestAccelerationStructureData,std::memory_order_release);ReleaseSRWLockExclusive(&lock);
 const auto c=++buildCount;if(d->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL||c<=32||Verbose())Log("PT0_BUILD_AS count=%llu frame=%llu type=%s dest=0x%llX source=0x%llX scratch=0x%llX num_descs=%u flags=0x%X postbuild=%u",c,frame,d->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL?"TLAS":"BLAS",d->DestAccelerationStructureData,d->SourceAccelerationStructureData,d->ScratchAccelerationStructureData,d->Inputs.NumDescs,unsigned(d->Inputs.Flags),n);}
 h.buildAS(l,d,n,p);
}
static void STDMETHODCALLTYPE SetStateHook(ID3D12GraphicsCommandList4* l,ID3D12StateObject* s){auto h=HooksFor(l);if(!h.setState)return;ResetCommand(l);command.stateObject=s;if(semantic.active||Verbose())Log("PT0_STATE_OBJECT frame=%llu list=%p state=%p raygen=%s",semantic.frame,l,s,semantic.raygen);h.setState(l,s);}

static void LogKnownTexture(const char* label,size_t rva,std::uint64_t sig) noexcept {
 if(!verifiedRenderer||!rrGuideGetNativeTexture)return;void* native=nullptr;NativeTextureStateSnapshot snap{};DWORD fault=0;
 __try {native=rrGuideGetNativeTexture(reinterpret_cast<unsigned char*>(verifiedRenderer)+rva);if(!native||!ReadNativeTextureState(native,&snap))return;}
 __except(EXCEPTION_EXECUTE_HANDLER){fault=GetExceptionCode();return;}
 Log("PT0_RESOURCE_CENSUS sig=%016llX label=%s shader_rva=0x%zX native=%p resource=%p dim=%u width=%llu height=%u format=%u flags=0x%X state_known=%u state=0x%X fault=0x%08lX",
  sig,label,rva,native,snap.resource,unsigned(snap.desc.Dimension),snap.desc.Width,snap.desc.Height,unsigned(snap.desc.Format),unsigned(snap.desc.Flags),snap.stateKnown,snap.trackedState,fault);
}
static void LogTable(std::uint64_t sig,UINT root,D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept {
 if(!handle.ptr)return;AcquireSRWLockShared(&lock);UINT base=0;auto* h=ResolveGpu(handle.ptr,&base);if(!h||!h->meta){ReleaseSRWLockShared(&lock);Log("PT0_ROOT_TABLE sig=%016llX root=%u handle=0x%llX heap=unresolved",sig,root,handle.ptr);return;}
 Log("PT0_ROOT_TABLE sig=%016llX root=%u handle=0x%llX heap=%p heap_type=%u base_index=%u",sig,root,handle.ptr,h->heap,unsigned(h->type),base);
 const UINT end=(base+MaxDescriptorsPerCensus<h->count)?base+MaxDescriptorsPerCensus:h->count;
 for(UINT i=base;i<end;++i){const auto& m=h->meta[i];if(m.kind==DescriptorKind::Empty)continue;const bool tlas=m.kind==DescriptorKind::SRV&&m.dimension==D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;Log("PT0_DESCRIPTOR sig=%016llX root=%u index=%u kind=%u resource=%p format=%u view_dim=%u gpuva=0x%llX width=%llu height=%u size=%u tlas=%u",sig,root,i,unsigned(m.kind),m.resource,unsigned(m.format),m.dimension,m.gpuva,m.width,m.height,m.sizeBytes,unsigned(tlas));if(tlas){++tlasMatches;Log("PT0_TLAS_OK sig=%016llX source=descriptor_table root=%u index=%u gpuva=0x%llX raygen=%s",sig,root,i,m.gpuva,semantic.raygen);}}
 ReleaseSRWLockShared(&lock);
}
static void STDMETHODCALLTYPE DispatchHook(ID3D12GraphicsCommandList4* l,const D3D12_DISPATCH_RAYS_DESC* d){
 auto h=HooksFor(l);if(!h.dispatch)return;ResetCommand(l);const auto call=++dispatchCount;if(!d){h.dispatch(l,d);return;}
 std::uint64_t sig=HashString(semantic.raygen);sig=HashBytes(&semantic.pipelineA,sizeof(semantic.pipelineA),sig);sig=HashBytes(&semantic.pipelineB,sizeof(semantic.pipelineB),sig);sig=HashBytes(&d->Width,sizeof(d->Width),sig);sig=HashBytes(&d->Height,sizeof(d->Height),sig);sig=HashBytes(&d->Depth,sizeof(d->Depth),sig);
 const UINT64 shape[]={d->RayGenerationShaderRecord.SizeInBytes,d->MissShaderTable.SizeInBytes,d->MissShaderTable.StrideInBytes,d->HitGroupTable.StrideInBytes,d->CallableShaderTable.StrideInBytes};sig=HashBytes(shape,sizeof(shape),sig);
 bool first=false;unsigned long long occurrence=0;AcquireSRWLockExclusive(&lock);PassRecord* rec=nullptr;for(auto& p:passes)if(p.signature==sig){rec=&p;break;}if(!rec)for(auto& p:passes)if(!p.signature){rec=&p;first=true;p.signature=sig;p.firstFrame=semantic.frame;p.width=d->Width;p.height=d->Height;p.depth=d->Depth;strncpy_s(p.raygen,sizeof(p.raygen),semantic.raygen,_TRUNCATE);break;}if(rec){occurrence=++rec->count;rec->lastFrame=semantic.frame;}ReleaseSRWLockExclusive(&lock);
 const bool detail=first||occurrence<=4||Verbose();
 if(detail)Log("PT0_DXR_PASS sig=%016llX occurrence=%llu frame=%llu class=%s raygen=%s ray_args=%d,%d pipeline_args=%d,%d dimensions=%ux%ux%u list=%p state=%p root=%p heaps=%u",sig,occurrence,semantic.frame,Classify(semantic.raygen),semantic.raygen,semantic.rayA,semantic.rayB,semantic.pipelineA,semantic.pipelineB,d->Width,d->Height,d->Depth,l,command.stateObject,command.root,command.heapCount);
 if(detail)Log("PT0_DISPATCH_CAPTURE sig=%016llX raygen_va=0x%llX raygen_size=%llu miss_va=0x%llX miss_size=%llu miss_stride=%llu hit_va=0x%llX hit_size=%llu hit_stride=%llu callable_va=0x%llX callable_size=%llu callable_stride=%llu",sig,d->RayGenerationShaderRecord.StartAddress,d->RayGenerationShaderRecord.SizeInBytes,d->MissShaderTable.StartAddress,d->MissShaderTable.SizeInBytes,d->MissShaderTable.StrideInBytes,d->HitGroupTable.StartAddress,d->HitGroupTable.SizeInBytes,d->HitGroupTable.StrideInBytes,d->CallableShaderTable.StartAddress,d->CallableShaderTable.SizeInBytes,d->CallableShaderTable.StrideInBytes);
 if(detail){for(UINT i=0;i<MaxCommandTables;++i){if(command.srv[i]){const bool t=IsKnownTLAS(command.srv[i]);Log("PT0_ROOT_BIND sig=%016llX root=%u type=SRV gpuva=0x%llX tlas=%u",sig,i,command.srv[i],unsigned(t));if(t){++tlasMatches;Log("PT0_TLAS_OK sig=%016llX source=root_srv root=%u gpuva=0x%llX raygen=%s",sig,i,command.srv[i],semantic.raygen);}}if(command.cbv[i])Log("PT0_ROOT_BIND sig=%016llX root=%u type=CBV gpuva=0x%llX",sig,i,command.cbv[i]);if(command.uav[i])Log("PT0_ROOT_BIND sig=%016llX root=%u type=UAV gpuva=0x%llX",sig,i,command.uav[i]);if(command.constantsCount[i])Log("PT0_ROOT_CONSTANTS sig=%016llX root=%u count=%u hash=%016llX",sig,i,command.constantsCount[i],command.constantsHash[i]);if(command.tables[i].ptr)LogTable(sig,i,command.tables[i]);}
  LogKnownTexture("reflection_target",kRRShaderReflectionTargetRva,sig);LogKnownTexture("diffuse_gi_color",kRRShaderDiffuseGIColorRva,sig);LogKnownTexture("diffuse_gi_weight_uav",kRRShaderDiffuseGIWeightUavRva,sig);LogKnownTexture("diffuse_gi_weight_srv",kRRShaderDiffuseGIWeightSrvRva,sig);LogKnownTexture("gbuffer0",kRRShaderGBufferCandidate0Rva,sig);LogKnownTexture("gbuffer1",kRRShaderGBufferCandidate1Rva,sig);LogKnownTexture("gbuffer2",kRRShaderGBufferCandidate2Rva,sig);LogKnownTexture("gbuffer3",kRRShaderGBufferCandidate3Rva,sig);LogKnownTexture("gbuffer4",kRRShaderGBufferCandidate4Rva,sig);LogKnownTexture("light_diffuse",kRRShaderLightBufferDiffuseRva,sig);LogKnownTexture("light_specular",kRRShaderLightBufferSpecularRva,sig);
 }
 h.dispatch(l,d);
 if(semantic.active&&std::strcmp(semantic.raygen,"reflectionRayGeneration")==0)::PT1AfterNativeReflection(l,d,semantic.frame,sig);
}

static bool EnsureCommandHooks(ID3D12GraphicsCommandList* base) noexcept {
 if(!base)return false;ID3D12GraphicsCommandList4* l4=nullptr;if(FAILED(base->QueryInterface(IID_PPV_ARGS(&l4)))||!l4)return false;auto** t=*reinterpret_cast<void***>(l4);
 if(commandHookReady.load()==1&&commandHooks.table==t){l4->Release();return true;}
 CommandHooks h{};h.table=t;h.dispatchCompute=reinterpret_cast<DispatchComputeFn>(t[14]);h.setPipeline=reinterpret_cast<SetPipelineFn>(t[25]);h.setHeaps=reinterpret_cast<SetHeapsFn>(t[28]);h.setRoot=reinterpret_cast<SetRootFn>(t[29]);h.setTable=reinterpret_cast<SetTableFn>(t[31]);h.setConstants=reinterpret_cast<SetConstantsFn>(t[35]);h.setCBV=reinterpret_cast<SetGpuVaFn>(t[37]);h.setSRV=reinterpret_cast<SetGpuVaFn>(t[39]);h.setUAV=reinterpret_cast<SetGpuVaFn>(t[41]);h.buildAS=reinterpret_cast<BuildASFn>(t[72]);h.setState=reinterpret_cast<SetStateObjectFn>(t[75]);h.dispatch=reinterpret_cast<DispatchRaysFn>(t[76]);
 bool ok=h.dispatchCompute&&h.setPipeline&&h.setHeaps&&h.setRoot&&h.setTable&&h.setConstants&&h.setCBV&&h.setSRV&&h.setUAV&&h.buildAS&&h.setState&&h.dispatch;
 if(ok){commandHooks=h;ok=PatchSlot(&t[25],reinterpret_cast<void*>(&SetPipelineHook))&&PatchSlot(&t[28],reinterpret_cast<void*>(&SetHeapsHook))&&PatchSlot(&t[29],reinterpret_cast<void*>(&SetRootHook))&&PatchSlot(&t[31],reinterpret_cast<void*>(&SetTableHook))&&PatchSlot(&t[35],reinterpret_cast<void*>(&SetConstantsHook))&&PatchSlot(&t[37],reinterpret_cast<void*>(&SetCBVHook))&&PatchSlot(&t[39],reinterpret_cast<void*>(&SetSRVHook))&&PatchSlot(&t[41],reinterpret_cast<void*>(&SetUAVHook))&&PatchSlot(&t[72],reinterpret_cast<void*>(&BuildASHook))&&PatchSlot(&t[75],reinterpret_cast<void*>(&SetStateHook))&&PatchSlot(&t[76],reinterpret_cast<void*>(&DispatchHook));}
 commandHookReady.store(ok?1u:2u);Log("PT0_COMMAND_HOOKS ready=%u base_list=%p list4=%p same_interface=%u table=%p compute_dispatch_slot=14 pipeline_slot=25 dispatch_rays_slot=76 build_as_slot=72 state_slot=75",unsigned(ok),base,l4,unsigned(reinterpret_cast<void*>(base)==reinterpret_cast<void*>(l4)),t);l4->Release();return ok;
}
static void SetSemantic(const char* raygen,int pa,int pb,int a,int b,unsigned long long frame) noexcept {semantic={};semantic.active=true;semantic.pipelineA=pa;semantic.pipelineB=pb;semantic.rayA=a;semantic.rayB=b;semantic.frame=frame;if(raygen)strncpy_s(semantic.raygen,sizeof(semantic.raygen),raygen,_TRUNCATE);}
static void ClearSemantic() noexcept {semantic.active=false;}
static void Poll(unsigned long long present) noexcept {const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;if(down&&!f7WasDown){verboseUntilPresent.store(present+600,std::memory_order_release);Log("PT0_CAPTURE_ARM present=%llu until=%llu hotkey=F8 detail_dispatches=1 resource_census=1 descriptor_census=1",present,present+600);}f7WasDown=down;}
static void Summary(unsigned long long present) noexcept {if(present!=1&&present%600!=0)return;unsigned used=0;AcquireSRWLockShared(&lock);for(const auto& p:passes)if(p.signature){++used;Log("PT0_PASS_SUMMARY sig=%016llX count=%llu first_frame=%llu last_frame=%llu dimensions=%ux%ux%u raygen=%s",p.signature,p.count,p.firstFrame,p.lastFrame,p.width,p.height,p.depth,p.raygen);}ReleaseSRWLockShared(&lock);Log("PT0_SUMMARY present=%llu passes=%u dispatches=%llu as_builds=%llu tlas_matches=%llu descriptor_events=%llu device_hooks=%u command_hooks=%u",present,used,dispatchCount.load(),buildCount.load(),tlasMatches.load(),descriptorEvents.load(),deviceHookReady.load(),commandHookReady.load());}

} // namespace control_pt0

static void PT0OnDeviceCreated(IUnknown* device) noexcept {control_pt0::OnDevice(device);}
static bool PT0EnsureCommandHooks(ID3D12GraphicsCommandList* list) noexcept {return control_pt0::EnsureCommandHooks(list);}
static void PT0SetSemantic(const char* raygen,int pa,int pb,int a,int b,unsigned long long frame) noexcept {control_pt0::SetSemantic(raygen,pa,pb,a,b,frame);}
static void PT0ClearSemantic() noexcept {control_pt0::ClearSemantic();}
static void PT0Poll(unsigned long long present) noexcept {control_pt0::Poll(present);}
static void PT0Summary(unsigned long long present) noexcept {control_pt0::Summary(present);}
static D3D12_GPU_VIRTUAL_ADDRESS PT0LatestTlas() noexcept {return control_pt0::latestTlas.load(std::memory_order_acquire);}
