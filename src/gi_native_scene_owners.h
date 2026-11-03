#pragma once
#include "gi_reuse/d3d12_live_scene_owners.h"
#include "gi_native_lifetime_runtime.h"
// Recovered R103 creation boundaries. These observe native ownership only;
// installing them does not enable the replacement lighting dispatch.
namespace GINativeSceneOwners {
static gi_reuse::D3D12LiveSceneOwners owners;
static std::atomic<bool> ready{false};
static std::atomic<UINT64> created{0},rejected{0};
static bool Enabled() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_LIVE",b,4)==1&&b[0]=='1';}();return value;}
struct Wrapper {IUnknown* resource=nullptr;UINT64 bytes=0,address=0;};
static bool Read(void* pointer,Wrapper& output) noexcept {
 output={};__try{if(!pointer)return false;auto* p=static_cast<unsigned char*>(pointer);
  output.resource=*reinterpret_cast<IUnknown**>(p);output.bytes=*reinterpret_cast<UINT64*>(p+0x30);output.address=*reinterpret_cast<UINT64*>(p+0x48);
  return output.resource&&output.bytes&&output.address;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void Created(void* wrapper) noexcept {
 if(!ready.load())return;const DWORD error=GetLastError();Wrapper input;
 if(Read(wrapper,input)&&owners.Created(reinterpret_cast<UINT_PTR>(wrapper),input.resource,input.address,input.bytes))++created;
 else {owners.Retire(reinterpret_cast<UINT_PTR>(wrapper));++rejected;}
 SetLastError(error);
}
static void Retire(void* wrapper) noexcept {if(ready.load())owners.Retire(reinterpret_cast<UINT_PTR>(wrapper));}
using General=void(*)(UINT64,UINT64,unsigned,unsigned,UINT64,UINT64,void*);
using Ray=void(*)(int,int,void*,int);
static General general=nullptr;static Ray ray=nullptr;
static RRAlbedoCallPatch creates[11]{};
static void GeneralHook(UINT64 bytes,UINT64 stride,unsigned flags,unsigned format,UINT64 a,UINT64 b,void* wrapper){general(bytes,stride,flags,format,a,b,wrapper);Created(wrapper);}
static void RayHook(int bytes,int type,void* wrapper,int flags){ray(bytes,type,wrapper,flags);Created(wrapper);}
// CPU source bytes are copied before the native memcpy, never read back from
// upload heaps. Build observers must retain the returned immutable snapshot.
struct InstanceUpload {UINT64 address=0,token=0;std::vector<unsigned char> bytes;};
static std::mutex uploadsLock;
static std::map<UINT64,std::shared_ptr<const InstanceUpload>> uploads;
using LockInstances=void(*)(void*,UINT64,UINT64,unsigned,void**);
using CopyInstances=void*(*)(void*,const void*,std::size_t);
static LockInstances lockInstances=nullptr;static CopyInstances copyInstances=nullptr;
static RRIndirectCallPatch lockPatch;static RRAlbedoCallPatch copyPatch{};
struct Locked {void* pointer=nullptr;UINT64 address=0,bytes=0;};static thread_local Locked locked;
static bool ReadPointer(void** source,void*& value) noexcept {__try{if(!source)return false;value=*source;return value!=nullptr;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
static bool CopyBytes(void* to,const void* from,std::size_t bytes) noexcept {__try{if(!from)return false;std::memcpy(to,from,bytes);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
static void LockHook(void* wrapper,UINT64 offset,UINT64 bytes,unsigned mode,void** pointer){
 lockInstances(wrapper,offset,bytes,mode,pointer);const DWORD error=GetLastError();locked={};Wrapper input;void* mapped=nullptr;
 if(ready.load()&&!offset&&bytes&&bytes<=2ull*1024*1024&&bytes%64==0&&Read(wrapper,input)&&bytes<=input.bytes&&ReadPointer(pointer,mapped))locked={mapped,input.address,bytes};
 SetLastError(error);
}
static void* CopyHook(void* to,const void* from,std::size_t bytes){
 const DWORD error=GetLastError();const auto current=locked;locked={};
 if(ready.load()&&to==current.pointer&&bytes==current.bytes&&current.address){
  try {std::lock_guard<std::mutex> guard(uploadsLock);uploads.erase(current.address);
   gi_reuse::D3D12LiveSceneOwners::Identity allocation;
   if(uploads.size()<64&&owners.Identify(current.address,bytes,allocation)){
    auto snapshot=std::make_shared<InstanceUpload>();snapshot->address=current.address;snapshot->token=allocation.token;snapshot->bytes.resize(bytes);
    if(CopyBytes(snapshot->bytes.data(),from,bytes))uploads.emplace(current.address,std::move(snapshot));
   }
  }catch(...){ready=false;}
 }
 SetLastError(error);return copyInstances(to,from,bytes);
}
[[maybe_unused]] static std::shared_ptr<const InstanceUpload> Instances(UINT64 address,UINT count) noexcept {
 try {std::lock_guard<std::mutex> guard(uploadsLock);gi_reuse::D3D12LiveSceneOwners::Identity allocation;
  const auto found=uploads.find(address);if(!ready.load()||!count||found==uploads.end()||found->second->bytes.size()!=UINT64(count)*64||
      !owners.Identify(address,UINT64(count)*64,allocation)||allocation.token!=found->second->token)return {};
  return found->second;
 }catch(...){return {};}
}
struct Writer {
 GINativeJobRoute::Ticket ticket;
 gi_reuse::NativeGIWriterInputs inputs;
 Microsoft::WRL::ComPtr<ID3D12StateObject> pipeline;
 PTShaderTableAdapter::Identifier opaque{},alpha{};
};
struct Event {
 gi_reuse::OrderedASEvent build;
 std::shared_ptr<const InstanceUpload> instances;
 std::vector<gi_reuse::D3D12LiveSceneOwners::Identity> bottoms;
 std::vector<PTOwnedScene::Triangle> geometry;
 std::shared_ptr<const Writer> writer;
};
static const GUID recordingKey={0x310e0001,0x1739,0x4401,{0x91,0x33,0x41,0x51,0x61,0x71,0x81,0x91}};
static std::atomic<UINT64> generations{1};
class Recording final:public IUnknown {
 std::atomic<ULONG> refs{1};
public:
 const UINT64 generation=generations.fetch_add(1);
 bool complete=true;
 std::vector<Event> events;
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override {if(!out)return E_POINTER;*out=nullptr;if(iid!=__uuidof(IUnknown))return E_NOINTERFACE;*out=static_cast<IUnknown*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{const auto n=--refs;if(!n)delete this;return n;}
};
static Microsoft::WRL::ComPtr<Recording> ReadRecording(ID3D12Object* object,bool create=true) noexcept {
 Microsoft::WRL::ComPtr<Recording> result;if(!object||!ready.load())return result;
 UINT bytes=sizeof(IUnknown*);IUnknown* value=nullptr;
 if(SUCCEEDED(object->GetPrivateData(recordingKey,&bytes,&value))&&value){if(bytes==sizeof(value))result.Attach(static_cast<Recording*>(value));else value->Release();return result;}
 if(!create)return result;result.Attach(new(std::nothrow) Recording);
 if(result&&FAILED(object->SetPrivateDataInterface(recordingKey,result.Get())))result.Reset();return result;
}
static void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!ready.load()||!list)return;
 // A successful native Reset cancels unsubmitted metadata, while submitted
 // owners must be retained separately by the queue retirement path.
 list->SetPrivateDataInterface(recordingKey,nullptr);
}
static void Build(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc) noexcept {
 auto recording=ReadRecording(list);if(!recording||!recording->complete)return;
 try {
  if(!desc||recording->events.size()>=65536){recording->complete=false;return;}
  gi_reuse::D3D12LiveSceneOwners::Identity destination,source;
  if(!owners.Identify(desc->DestAccelerationStructureData,256,destination)){recording->complete=false;return;}
  Event e;auto& event=e.build.event;auto& r=event.record;
  r.address=desc->DestAccelerationStructureData;r.count=desc->Inputs.NumDescs;
  r.allowUpdate=(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)!=0;
  e.build.destinationToken=destination.token;event.operation=gi_reuse::ASOperation::Build;
  if(desc->Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE){
   event.source=desc->SourceAccelerationStructureData;event.operation=gi_reuse::ASOperation::Update;
   if(!owners.Identify(event.source,256,source)){recording->complete=false;return;}e.build.sourceToken=source.token;
  }
  bool valid=r.count!=0;
  if(desc->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
   r.topLevel=true;r.instances=desc->Inputs.InstanceDescs;e.instances=Instances(r.instances,r.count);
   valid=valid&&desc->Inputs.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY&&bool(e.instances);
   if(valid){e.bottoms.resize(r.count);
    for(UINT i=0;i<r.count;++i){gi_reuse::InstanceRecord row;
     if(!gi_reuse::DecodeInstance(e.instances->bytes.data()+SIZE_T(i)*64,row)){valid=false;break;}
     if(row.blas&&!owners.Identify(row.blas,256,e.bottoms[i])){valid=false;break;}
    }
   }
  }else{
   r.geometry=gi_reuse::CopyBLASGeometry(desc->Inputs);valid=valid&&bool(r.geometry);
   if(valid){e.geometry.resize(r.count);for(UINT i=0;i<r.count;++i)if(!owners.Geometry(*r.geometry->At(i),e.geometry[i])){valid=false;break;}}
  }
  // Unsupported writes hide the prior version; they never preserve an old
  // valid geometry mapping at the same address.
  if(!valid){event.operation=gi_reuse::ASOperation::Invalidate;e.geometry.clear();e.bottoms.clear();e.instances.reset();}
  recording->events.push_back(std::move(e));
 }catch(...){recording->complete=false;}
}
static void Copy(ID3D12GraphicsCommandList4* list,UINT64 destination,UINT64 source,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode) noexcept {
 auto recording=ReadRecording(list);if(!recording||!recording->complete)return;
 try {gi_reuse::D3D12LiveSceneOwners::Identity dst,src;
  if(recording->events.size()>=65536||!owners.Identify(destination,256,dst)){recording->complete=false;return;}
  Event e;e.build.destinationToken=dst.token;e.build.event.record.address=destination;e.build.event.operation=gi_reuse::ASOperation::Invalidate;
  if((mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE||mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT)&&owners.Identify(source,256,src)){
   e.build.sourceToken=src.token;e.build.event.source=source;e.build.event.operation=gi_reuse::ASOperation::Copy;
  }
  recording->events.push_back(std::move(e));
 }catch(...){recording->complete=false;}
}
static void WriterReady(const GINativeJobRoute::Ticket& ticket) noexcept {
 if(!ready.load())return;auto recording=ReadRecording(reinterpret_cast<ID3D12GraphicsCommandList4*>(ticket.list));
 if(!recording||!recording->complete)return;
 try {if(recording->events.size()>=65536){recording->complete=false;return;}
  auto writer=std::make_shared<Writer>();writer->ticket=ticket;
  gi_reuse::native_gi_detail::Header header;
  if(!gi_reuse::SnapshotNativeGIWriter(ticket,writer->inputs)||!gi_reuse::native_gi_detail::ReadHeader(ticket.writer,header)||header.pipeline!=ticket.pipeline){recording->complete=false;return;}
  writer->opaque=header.opaque;writer->alpha=header.alpha;
  writer->pipeline=reinterpret_cast<ID3D12StateObject*>(ticket.pipeline);
  Event e;e.writer=std::move(writer);recording->events.push_back(std::move(e));
 }catch(...){recording->complete=false;}
}
static bool Install(HMODULE d3d) noexcept {
 if(!Enabled()||!gi_reuse::runtime::Enabled()||!HashMatches(d3d,kD3dHash)||!HashMatches(verifiedRenderer,kRendererHash))return false;
 if(ready.load())return true;auto* b=reinterpret_cast<unsigned char*>(d3d);auto* r=reinterpret_cast<unsigned char*>(verifiedRenderer);
 general=reinterpret_cast<General>(b+0x26030);ray=reinterpret_cast<Ray>(b+0x26360);
 lockInstances=reinterpret_cast<LockInstances>(*reinterpret_cast<void**>(r+0x5dfe70));copyInstances=reinterpret_cast<CopyInstances>(r+0x588e0e);
 unsigned n=0;bool ok=true;
 for(unsigned offset:{0x25217u,0x2527fu,0x252f1u,0x25367u,0x253d1u,0x2544bu,0x254bbu,0x2551fu}){
  if(!RRAlbedoPrepareCallPatch(b+offset,reinterpret_cast<void*>(general),reinterpret_cast<void*>(&GeneralHook),&creates[n++])){ok=false;break;}}
 if(ok)for(unsigned offset:{0x2824fu,0x2ae34u,0x35763u}){
  if(!RRAlbedoPrepareCallPatch(b+offset,reinterpret_cast<void*>(ray),reinterpret_cast<void*>(&RayHook),&creates[n++])){ok=false;break;}}
 ok=ok&&n==11&&lockPatch.Prepare(r+0xa8972,reinterpret_cast<void**>(r+0x5dfe70),reinterpret_cast<void*>(lockInstances),reinterpret_cast<void*>(&LockHook))&&
  RRAlbedoPrepareCallPatch(r+0xa8987,reinterpret_cast<void*>(copyInstances),reinterpret_cast<void*>(&CopyHook),&copyPatch);
 if(ok)for(auto& p:creates)if(!RRAlbedoExchangeCall(&p,true)){ok=false;break;}
 if(ok)ok=lockPatch.Exchange(true)&&RRAlbedoExchangeCall(&copyPatch,true);
 if(!ok){if(copyPatch.site)RRAlbedoExchangeCall(&copyPatch,false);if(lockPatch.changed)lockPatch.Exchange(false);for(auto& p:creates)if(p.site)RRAlbedoExchangeCall(&p,false);}
 if(ok){GINativeLifetimeRuntime::liveRetire.store(&Retire);gi_reuse::runtime::liveBuild.store(&Build);gi_reuse::runtime::liveCopy.store(&Copy);gi_reuse::runtime::liveReset.store(&Reset);ready=true;}
 Log("GI_LIVE_OWNERS ready=%u native_creates=11 instance_source_copy=1 replacement_dispatch=0",unsigned(ok));return ok;
}
}
