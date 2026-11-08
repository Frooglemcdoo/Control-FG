#pragma once
#include "gi_dual_history_bridge.h"
#include "gi_dual_history_policy.h"
#include <array>
#include <mutex>
#include <vector>
// Small, independent sidecar owner. Does not enable AS/shadow inventory hooks.
namespace GIDualRuntime {
using Microsoft::WRL::ComPtr;
using namespace gi_dual_history;
static std::recursive_mutex mutex;
static Policy policy;
struct Storage {ComPtr<ID3D12Resource> fast,moments;ComPtr<ID3D12DescriptorHeap> views;UINT width=0,height=0;};
static std::array<Storage,Slots> storage;
struct Recording {
 ComPtr<ID3D12GraphicsCommandList> list;
 Lease lease;UINT64 generation=0,fence=0;bool reset=false,submitted=false;
};
static std::vector<Recording> recordings;
static ComPtr<ID3D12CommandQueue> queue;
static ComPtr<ID3D12Fence> fence;
static ComPtr<ID3D12DescriptorHeap> nullViews;
static UINT64 nextFence=0,nextSerial=0,epoch=1,lastView=0;
static UINT width=0,height=0,lastProducer=0;
static std::atomic<bool> active{false};
struct Pass {
 UINT64 frame=0,view=0,serial=0;bool failed=false;
 PTGlassRuntime::Context context{};
 std::unique_ptr<PTGlass::LightingTableShadow> shadow;
};
static thread_local Pass pass;
struct Unobserve {bool old=gi_dual_bridge::observing;Unobserve(){gi_dual_bridge::observing=false;}~Unobserve(){gi_dual_bridge::observing=old;}};
static void Fail(const char* reason) noexcept {
 if(!policy.Failed())Log("GI_DUAL disabled=1 reason=%s allocations_retained=1",reason);
 policy.Poison();
}
static ID3D12Resource* Target() noexcept {
 using Get=void*(*)(void*);
 auto getter=reinterpret_cast<Get>(GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol));
 if(!getter)return nullptr;
 __try {void* t=getter(reinterpret_cast<unsigned char*>(verifiedRenderer)+0x128f3a0);
  return t?*reinterpret_cast<ID3D12Resource**>(static_cast<unsigned char*>(t)+0x88):nullptr;
 }__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
static void Begin(UINT64 frame,UINT64 view) noexcept {
 gi_dual_bridge::observing=false;pass.frame=frame;pass.view=view;pass.failed=false;pass.context={};pass.serial=0;
 // The build-locked glass descriptor hooks already cover ConstantTable::use.
 // Never enable the legacy GI scene descriptor index as a prerequisite.
 if(!PTGlassRuntime::device||!PTGlassRuntime::copySimple||!PTGlassRuntime::createCbv)return;
 gi_dual_bridge::observing=true;
}
static void End() noexcept {gi_dual_bridge::observing=false;}
static PTGlass::LightingTableShadow* Shadow() noexcept {
 if(!gi_dual_bridge::observing||pass.failed)return nullptr;
 try {
  if(!pass.serial){
   if(!PTGlassRuntime::ReadContext(&pass.context)){pass.failed=true;return nullptr;}
   Unobserve mute;
   if(!pass.shadow){pass.shadow=std::make_unique<PTGlass::LightingTableShadow>();
    if(FAILED(pass.shadow->Initialize(PTGlassRuntime::device.Get()))){pass.shadow.reset();pass.failed=true;return nullptr;}}
   {std::lock_guard<std::recursive_mutex> guard(mutex);pass.serial=++nextSerial;}
   if(!pass.shadow->Begin(pass.serial,pass.context.table)){pass.failed=true;return nullptr;}
  }
  return pass.shadow.get();
 }catch(...){pass.failed=true;return nullptr;}
}
static void Copy(D3D12_CPU_DESCRIPTOR_HANDLE dst,UINT count,D3D12_CPU_DESCRIPTOR_HANDLE src) noexcept {
 if(auto* s=Shadow()){Unobserve mute;s->ObserveNativeCopy(pass.serial,dst,count,src);}
}
static void Constant(D3D12_CPU_DESCRIPTOR_HANDLE dst,const D3D12_CONSTANT_BUFFER_VIEW_DESC& cb) noexcept {
 if(auto* s=Shadow()){Unobserve mute;s->ObserveNativeConstant(pass.serial,dst,cb);}
}
static void Poll() {
 const auto completed=fence?fence->GetCompletedValue():0;
 if(completed==UINT64_MAX){Fail("device_removed");return;}
 for(auto it=recordings.begin();it!=recordings.end();){
  if(it->reset&&(!it->submitted||it->fence<=completed)){
   if(!it->submitted&&!policy.Cancel(it->lease,it->generation))Fail("cancel_identity");
   it=recordings.erase(it);
  }else ++it;
 }
}
static void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!active.load(std::memory_order_relaxed))return;
 std::lock_guard<std::recursive_mutex> guard(mutex);
 for(auto& r:recordings)if(r.list.Get()==list)r.reset=true;
 Poll();
}
static UINT64 Before(ID3D12CommandQueue* q,UINT n,ID3D12CommandList*const* lists) noexcept {
 if(!active.load(std::memory_order_relaxed))return 0;
 mutex.lock();UINT64 mask=0;
 for(unsigned r=0;r<recordings.size();++r){auto& item=recordings[r];if(item.reset)continue;
  for(UINT i=0;i<n;++i)if(lists&&lists[i]==item.list.Get()){
   if(q!=queue.Get()||item.submitted||(mask&(1ull<<r))){Fail("queue_or_replay");continue;}
   mask|=1ull<<r;
  }
 }
 if(!mask){mutex.unlock();return 0;}
 return mask;
}
static void After(ID3D12CommandQueue* q,UINT64 mask) noexcept {
 if(!mask)return;
 // Called after the actual Execute, while the same owner's mutex is held.
 for(unsigned i=0;i<recordings.size();++i)if(mask&(1ull<<i)){
  auto& r=recordings[i];const auto value=++nextFence;
  r.submitted=true;r.fence=value;
  if(FAILED(q->Signal(fence.Get(),value))||!policy.Submitted(r.lease,reinterpret_cast<UINT64>(q),r.generation,value))Fail("submission_fence");
 }
 mutex.unlock();
}
static D3D12_CPU_DESCRIPTOR_HANDLE Handle(ID3D12DescriptorHeap* heap,UINT i) {
 auto h=heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*PTGlassRuntime::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);return h;
}
static bool Allocate(Storage& s,UINT w,UINT h) {
 if(s.width==w&&s.height==h)return true;
 Storage next;auto* d=PTGlassRuntime::device.Get();
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;rd.Width=w;rd.Height=h;rd.DepthOrArraySize=rd.MipLevels=1;
 rd.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;rd.SampleDesc.Count=1;rd.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&next.fast)))||
    FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&next.moments))))return false;
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=4;
 if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&next.views))))return false;
 D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=rd.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=rd.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
 for(UINT i=0;i<2;++i){auto* resource=i?next.moments.Get():next.fast.Get();d->CreateShaderResourceView(resource,&srv,Handle(next.views.Get(),i));d->CreateUnorderedAccessView(resource,nullptr,&uav,Handle(next.views.Get(),i+2));}
 next.width=w;next.height=h;s=std::move(next);return true;
}
static bool Initialize(ID3D12CommandQueue* q) {
 if(queue)return queue.Get()==q;
 auto* d=PTGlassRuntime::device.Get();ComPtr<ID3D12Fence> f;ComPtr<ID3D12DescriptorHeap> nulls;
 if(FAILED(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&f))))return false;
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;
 if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&nulls))))return false;
 D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 for(UINT i=0;i<2;++i)d->CreateShaderResourceView(nullptr,&srv,Handle(nulls.Get(),i));
 if(!native_lighting::rt_spot_table_audit::InstallGameQueue(q))return false;
 fence=std::move(f);nullViews=std::move(nulls);queue=q;recordings.reserve(32);active=true;return true;
}
static void Transition(ID3D12GraphicsCommandList* list,Storage& s,bool write) {
 D3D12_RESOURCE_BARRIER b[2]{};
 for(UINT i=0;i<2;++i){b[i].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b[i].Transition={i?s.moments.Get():s.fast.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
  write?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
  write?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};}
 list->ResourceBarrier(2,b);
}
static bool Dispatch(ID3D12GraphicsCommandList* list,ID3D12PipelineState* native,ID3D12PipelineState* custom,UINT x,UINT y,UINT z,gi_dual_bridge::Dispatch dispatch) noexcept {
 if(!pass.serial||pass.failed||!pass.shadow||z!=1)return false;
 const auto serial=pass.serial;pass.serial=0; // exactly one consumption per bind
 std::lock_guard<std::recursive_mutex> guard(mutex);Unobserve mute;
 Lease lease;bool commands=false;
 try {
  if(policy.Failed())return false;
  const auto resetReady=gi_dual_bridge::resetReady.load();if(!resetReady||!resetReady(list))return false;
  PTGlassRuntime::Context c;if(!PTGlassRuntime::ReadContext(&c)||c.list!=list||c.list!=pass.context.list||c.queue!=pass.context.queue||c.gpu.ptr!=pass.context.gpu.ptr||c.table.heap!=pass.context.table.heap)return false;
  // Native b0/b1 remain root CBVs. Mirror all five native temporal SRVs and u0.
  constexpr UINT64 nativeMask=(31ull<<5)|(1ull<<41);
  if(!pass.shadow->Has(serial,nativeMask)||!Initialize(c.queue))return false;
  auto* target=Target();if(!target)return false;const auto desc=target->GetDesc();
  if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.DepthOrArraySize!=1||desc.SampleDesc.Count!=1||!desc.Width||!desc.Height||desc.Width>8192||desc.Height>8192||x!=(desc.Width+7)/8||y!=(desc.Height+7)/8)return false;
  // Hard bound includes all four sidecar pairs. No unbounded resize generations.
  if(desc.Width*desc.Height*32*Slots>1024ull*1024*1024)return false;
  Poll();if(policy.Failed()||recordings.size()>=32)return false;
  const auto producer=unsigned(gi_reuse::ownedRequested.load());
  if(width!=desc.Width||height!=desc.Height||lastView!=pass.view||lastProducer!=producer){++epoch;width=UINT(desc.Width);height=desc.Height;lastView=pass.view;lastProducer=producer;}
  unsigned reusable=(1u<<Slots)-1;
  // A closed command list may be replayed until Reset. Keep all resources it
  // references pinned, even after its first GPU execution has completed.
  for(const auto& r:recordings)if(!r.reset){reusable&=~(1u<<r.lease.output);if(r.lease.previous<Slots)reusable&=~(1u<<r.lease.previous);}
  lease=policy.Begin(pass.frame,epoch,reinterpret_cast<UINT64>(c.queue),serial,fence->GetCompletedValue(),reusable);
  if(!lease)return false;
  if(!Allocate(storage[lease.output],width,height)){policy.Cancel(lease,serial);return false;}
  auto& out=storage[lease.output];auto* previous=lease.previous<Slots?storage[lease.previous].views.Get():nullViews.Get();
  bool good=pass.shadow->Copy(serial,10,{previous,0,2})&&pass.shadow->Copy(serial,42,{out.views.Get(),2,2});
  PTGlass::DescriptorRange fresh;D3D12_GPU_DESCRIPTOR_HANDLE table{};
  good=good&&PTGlassRuntime::Fresh(c,&fresh)&&pass.shadow->PublishChecked(serial,fresh,nativeMask|(3ull<<10)|(3ull<<42),&table);
  if(!good){policy.Cancel(lease,serial);return false;}
  Recording r;r.list=list;r.lease=lease;r.generation=serial;recordings.push_back(std::move(r));
  commands=true;Transition(list,out,true);
  {PTGlass::ScopedLightingBinding binding(list,native,c.gpu,custom,table);dispatch(list,x,y,z);}
  Transition(list,out,false);
  static UINT reports=0;if(reports++<4)Log("GI_DUAL applied=1 frame=%llu previous=%u extent=%ux%u independent_lifetime=1",pass.frame,unsigned(lease.previous<Slots),width,height);
  return true;
 }catch(...){if(commands)Fail("recording_exception");else if(lease)policy.Cancel(lease,serial);return commands;}
}
static const gi_dual_bridge::Callbacks callbacks{&Begin,&End,&Copy,&Constant,&Dispatch,&Reset,&Before,&After};
[[maybe_unused]] static const bool registered=[] {gi_dual_bridge::callbacks.store(&callbacks);return true;}();
}
