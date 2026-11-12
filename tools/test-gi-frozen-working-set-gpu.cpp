#include "gpu_test_device.h"
#include <atomic>
#include <type_traits>
#include "../src/gi_reuse/d3d12_frozen_working_set.h"
#include "../src/gi_reuse/d3d12_split_owner_retirement.h"
#include "../src/gi_reuse/d3d12_frozen_completion.h"
#include "../src/gi_reuse/d3d12_frozen_producer.h"
#include "../src/gi_reuse/d3d12_frozen_pipeline_bundle.h"
#include "../src/gi_reuse/graphics_table_state.h"
static_assert(!std::is_copy_constructible_v<gi_reuse::D3D12FrozenWorkingSet>);
static_assert(!std::is_copy_constructible_v<gi_reuse::D3D12GIRayConstantSnapshot>);
static_assert(!std::is_copy_constructible_v<gi_reuse::D3D12FrozenRaygenRecords>);
static_assert(!std::is_copy_assignable_v<gi_reuse::D3D12GIRayConstantSnapshot>);
static_assert(!std::is_copy_assignable_v<gi_reuse::D3D12FrozenRaygenRecords>);
namespace gi_reuse {namespace runtime {inline bool Enabled()noexcept{return false;}}}
namespace fixture {
inline void Trace(const char*,...){}
inline bool PatchSlot(void** slot,void* expected,void* replacement)noexcept {
 DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;
 const bool ok=InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
 DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);return ok;
}
#include "../src/gi_compute_hooks.inl"
}
HRESULT STDMETHODCALLTYPE RefusedSplitReset(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*){return E_FAIL;}
struct Invocation {gi_reuse::D3D12FrozenWorkingSet scratch;int* deaths;explicit Invocation(int& n):deaths(&n){}~Invocation(){++*deaths;}};
int main()try {
 using namespace gi_reuse;Device d(false);
 auto layout=D3D12FrozenWorkingSet::Describe(18,3,16);Require(bool(layout)&&layout.count==27,"bad compact count");
 Require(!D3D12FrozenWorkingSet::Describe(17,3,16),"odd width admitted");
 Require(!D3D12FrozenWorkingSet::Describe(16384,16384,16),"oversized training admitted");
 auto fourK=D3D12FrozenWorkingSet::Describe(3840,2160,65536);
 Require(fourK.bytes[0]==149299200&&fourK.bytes[1]==16588800,"4K working allocation changed");
 int deaths=0;auto invocation=std::make_unique<Invocation>(deaths);auto& scratch=invocation->scratch;
 D3D12SplitOwnerRetirement<Invocation,2,8*1024*1024> owners;Check(owners.Initialize(d.queue.Get()));
 fixture::gi_compute_hooks::EnableForDenoiser();fixture::gi_compute_hooks::InstallList(d.list.Get());
 fixture::gi_compute_hooks::Begin(d.list.Get(),nullptr);
 static decltype(owners)* liveOwners=nullptr;liveOwners=&owners;
 fixture::gi_compute_hooks::onFrozenRecordingReset=+[](ID3D12CommandList* list,UINT64 previous)noexcept {
  liveOwners->Seal({reinterpret_cast<UINT64>(list),previous});
 };
 static unsigned legacyResets=0;
 fixture::gi_compute_hooks::onRecordingReset=+[](ID3D12CommandList*,UINT64)noexcept{++legacyResets;};
 const auto generation=&fixture::gi_compute_hooks::RecordingGeneration;
 Require(FAILED(scratch.Initialize(d.device.Get(),layout,1))&&!scratch.Get(D3D12FrozenWorkingSet::Training),"budget failure published partial set");
 Check(scratch.Initialize(d.device.Get(),layout,4*1024*1024));
 Require(scratch.Matches(18,3,16)&&!scratch.Matches(18,4,16),"wrong extent reuse admitted");
 Require(scratch.AllocationBytes()>=4*65536,"committed allocation not accounted");
 Require(FAILED(scratch.Initialize(d.device.Get(),layout,4*1024*1024)),"live allocation overwritten");
 // WARP cannot execute the real DXR producer. Exercise fail-closed admission
 // and compile its complete command path without pretending this is a trace.
 D3D12GIRayConstantSnapshot unpreparedConstants;D3D12GISbtSnapshot unpreparedTables;
 D3D12FrozenRaygenRecords unpreparedRecords;D3D12FrozenProducer::Inputs producerInput;
 ComputeBindingSnapshot saved;saved.arguments.Root(1);
 saved.arguments.Descriptor(2,RootArgumentKind::CBV,65536);
 saved.arguments.Descriptor(3,RootArgumentKind::CBV,65536+FrozenRayConstants::GIConstantOffset);
 const auto noTrace=+[](ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*){Require(false,"rejected producer dispatched rays");};
 Require(D3D12FrozenProducer::Record(d.list.Get(),producerInput,scratch,unpreparedConstants,
  unpreparedTables,unpreparedRecords,noTrace,saved)==D3D12FrozenProducer::Result::RejectedBeforeCommands,
  "producer without native lease admitted");
 producerInput.nativeSingleRayAbi=true;producerInput.sceneAndDescriptorLease=true;
 Require(D3D12FrozenProducer::Record(d.list.Get(),producerInput,scratch,unpreparedConstants,
  unpreparedTables,unpreparedRecords,noTrace,saved)==D3D12FrozenProducer::Result::RejectedBeforeCommands,
  "unprepared producer admitted");
 Require(FrozenRayConstants::Matches(saved.arguments,65536),"rejected producer published partial bindings");
 // Recovery is not an escape hatch for a missing producer or scene lease.
 // No fallback trace may be emitted from incomplete admission state.
 D3D12FrozenCompletion::Inputs completionInput;
 D3D12FrozenEvaluator unpreparedEvaluator;D3D12FrozenGuideBuild unpreparedBuilder;
 for(unsigned mode=0;mode<4;++mode){
  completionInput.separateProducerSubmitted=(mode&1)!=0;
  completionInput.sceneAndDescriptorLease=(mode&2)!=0;
  Require(D3D12FrozenCompletion::RecordOrRecover(d.list.Get(),completionInput,scratch,
   unpreparedEvaluator,unpreparedBuilder,unpreparedRecords,noTrace)==D3D12FrozenCompletion::Result::RejectedBeforeCommands,
   "unprepared completion escaped into recovery");
 }
 // Distinct command lists/submissions: producer writes sentinel data; consumer
 // reads it only after producer completes. CPU ownership waits for BOTH resets.
 const UINT count=layout.count;std::vector<float> values(count);for(UINT i=0;i<count;++i)values[i]=float(i)+.25f;
 auto upload=d.Upload(values),read=d.Buffer(count*sizeof(float),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 auto* weights=scratch.Get(D3D12FrozenWorkingSet::Weights);
 using Recording=decltype(owners)::Recording;
 const Recording producer{reinterpret_cast<UINT64>(d.list.Get()),generation(d.list.Get())};
 const auto id=owners.Admit(1,producer,scratch.AllocationBytes(),invocation);Require(id&&!invocation,"owner admission failed");
 // Global Execute callbacks also see unrelated queues/lists. Unknown generation
 // on an unrelated list must not quarantine this producer before it submits.
 ComPtr<ID3D12CommandQueue> foreign;D3D12_COMMAND_QUEUE_DESC qd{};qd.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
 Check(d.device->CreateCommandQueue(&qd,IID_PPV_ARGS(&foreign)));
 ComPtr<ID3D12CommandAllocator> unrelatedAllocator;ComPtr<ID3D12GraphicsCommandList> unrelated;
 Check(d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&unrelatedAllocator)));
 Check(d.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,unrelatedAllocator.Get(),nullptr,IID_PPV_ARGS(&unrelated)));
 ID3D12CommandList* unrelatedLists[]={unrelated.Get()};
 const auto unknown=+[](ID3D12CommandList*)noexcept -> UINT64{return 0;};
 Require(!owners.Before(foreign.Get(),1,unrelatedLists,unknown).count,"unrelated foreign queue tagged as GI");
 Require(!owners.Before(d.queue.Get(),1,unrelatedLists,unknown).count,"unrelated untracked list tagged as GI");
 Require(!owners.Before(d.queue.Get(),0,nullptr,generation).count,"empty Execute tagged as GI");
 std::array<ID3D12CommandList*,257> unrelatedLarge{};unrelatedLarge.fill(unrelated.Get());
 Require(!owners.Before(foreign.Get(),static_cast<UINT>(unrelatedLarge.size()),unrelatedLarge.data(),unknown).count,"large unrelated batch tagged as GI");
 Require(!owners.Before(foreign.Get(),1,unrelatedLists,nullptr).count,"unrelated absent observer tagged as GI");
 std::unique_ptr<Invocation> retired;
 d.Transition(weights,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->CopyBufferRegion(weights,0,upload.Get(),0,count*sizeof(float));
 const auto bindingBefore=fixture::gi_compute_hooks::Read(d.list.Get())->generation;
 d.list->ClearState(nullptr);
 Require(generation(d.list.Get())==producer.generation&&fixture::gi_compute_hooks::Read(d.list.Get())->generation>bindingBefore,"ClearState changed recording identity");
 ID3D12CommandList* producerLists[]={d.list.Get()};
 auto first=owners.Before(d.queue.Get(),1,producerLists,generation);Require(first.count==1,"producer submit refused");d.Submit();
 // Device::Submit waits for its actual queue fence; the owner adapter adds
 // its own private completion cut after the intercepted submission.
 Check(d.list->Reset(d.allocator.Get(),nullptr));
 Require(owners.After(d.queue.Get(),first),"producer fence refused");owners.Poll();
 Require(generation(d.list.Get())>producer.generation&&!owners.Take(id,retired)&&deaths==0,"producer completion retired consumer scratch");
 ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList4> consumer;
 Check(d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 Check(d.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&consumer)));
 fixture::gi_compute_hooks::InstallList(consumer.Get());fixture::gi_compute_hooks::Begin(consumer.Get(),nullptr);
 const Recording shading{reinterpret_cast<UINT64>(consumer.Get()),generation(consumer.Get())};
 Require(owners.Attach(id,1,shading),"consumer attach failed");
 Require(owners.CanRecordConsumer(id,1,d.queue.Get(),shading),"real producer submission did not admit consumer recording");
 Require(!owners.CanRecordConsumer(id,2,d.queue.Get(),shading),"wrong frame admitted consumer recording");
 Require(owners.ClaimConsumerRecording(id,1,d.queue.Get(),shading),"consumer recording claim failed");
 Require(!owners.ClaimConsumerRecording(id,1,d.queue.Get(),shading),"duplicate consumer commands admitted");
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
 barrier.Transition={weights,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE};
 consumer->ResourceBarrier(1,&barrier);consumer->CopyBufferRegion(read.Get(),0,weights,0,count*sizeof(float));Check(consumer->Close());
 ID3D12CommandList* lists[]={consumer.Get()};auto second=owners.Before(d.queue.Get(),1,lists,generation);Require(second.count==1,"consumer submit refused");d.queue->ExecuteCommandLists(1,lists);
 auto fence=d.fence;
 Require(owners.After(d.queue.Get(),second),"consumer fence failed");Check(d.queue->Signal(fence.Get(),2));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event failed");
 Check(fence->SetEventOnCompletion(2,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"consumer timed out");CloseHandle(event);
 owners.Poll();Require(!owners.Take(id,retired)&&deaths==0,"closed list retired before reset");
 Require(!owners.CanRecordConsumer(id,1,d.queue.Get(),shading),"executed consumer admitted a second recording");
 // A refused Reset must not seal the consumer or change its recording identity.
 fixture::gi_compute_hooks::Hooks* intercepted=nullptr;
 for(auto& h:fixture::gi_compute_hooks::hooks)if(h.table==*reinterpret_cast<void***>(consumer.Get()))intercepted=&h;
 Require(intercepted!=nullptr,"missing Reset interposer");const auto savedReset=intercepted->reset;
 const auto beforeRefused=legacyResets;
 intercepted->reset=&RefusedSplitReset;const auto refused=consumer->Reset(allocator.Get(),nullptr);intercepted->reset=savedReset;
 Require(FAILED(refused)&&legacyResets==beforeRefused&&generation(consumer.Get())==shading.generation&&!owners.Take(id,retired),"failed Reset retired consumer");
 Check(consumer->Reset(allocator.Get(),nullptr));Require(owners.Take(id,retired)&&owners.RetainedBytes()==0,"completed/reset consumer not retired");
 retired.reset();Require(deaths==1,"invocation resources not released exactly once");
 float* mapped=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&mapped)));
 for(UINT i=0;i<count;++i)Require(mapped[i]==values[i],"split scratch data changed");D3D12_RANGE empty{};read->Unmap(0,&empty);
 Require(legacyResets>=2,"independent observer displaced the existing Reset observer");
 fixture::gi_compute_hooks::onRecordingReset=nullptr;fixture::gi_compute_hooks::onFrozenRecordingReset=nullptr;liveOwners=nullptr;
 for(unsigned mode=0;mode<2;++mode){
  D3D12SplitOwnerRetirement<Invocation,2,8*1024*1024> bad;Check(bad.Initialize(d.queue.Get()));
  auto object=std::make_unique<Invocation>(deaths);const Recording r{reinterpret_cast<UINT64>(d.list.Get()),generation(d.list.Get())};
  const auto token=bad.Admit(2,r,64,object);Require(token!=0,"bad-path fixture admission failed");
  auto batch=bad.Before(mode?d.queue.Get():foreign.Get(),1,producerLists,mode?unknown:generation);
  Require(!batch.count&&!bad.Seal(r)&&!bad.Take(token,retired),"owned wrong-queue/unknown-generation execution escaped quarantine");
  bad.DeviceStopped(); // Fixture recorded no GPU commands using this owner.
 }
 // Model a recycled COM address by dropping the private State after all owners
 // retire, then attaching fresh State at the identical command-list address.
 const auto oldIdentity=generation(consumer.Get());
 Check(consumer->SetPrivateDataInterface(fixture::gi_compute_hooks::stateKey,nullptr));
 fixture::gi_compute_hooks::InstallList(consumer.Get());fixture::gi_compute_hooks::Begin(consumer.Get(),nullptr);
 Require(generation(consumer.Get())>oldIdentity,"recycled list address reused a recording identity");
 const auto savedSerial=fixture::gi_compute_hooks::recordingSerial.exchange(UINT64_MAX);
 Require(!fixture::gi_compute_hooks::NextRecording()&&fixture::gi_compute_hooks::recordingSerial==UINT64_MAX,"recording serial wrapped");
 fixture::gi_compute_hooks::recordingSerial=savedSerial;
 std::cout<<"PASS split working set with actual Reset/ClearState hooks and private fence: 4K allocation sizing, budget rejection, two-list data preservation, last-consumer lifetime\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
