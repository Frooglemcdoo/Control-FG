#include "gpu_test_device.h"
#include <atomic>
#include <type_traits>
#include "../src/gi_reuse/d3d12_frozen_working_set.h"
#include "../src/gi_reuse/d3d12_frozen_invocation.h"
#include "../src/gi_reuse/d3d12_frozen_owner_cache.h"
#include "../src/gi_reuse/d3d12_split_owner_retirement.h"
#include "../src/gi_reuse/d3d12_split_submission_router.h"
#include "../src/gi_frozen_submission_bridge.h"
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
#include "test-gi-native-textures.inl"
HRESULT STDMETHODCALLTYPE RefusedSplitReset(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*){return E_FAIL;}
struct Invocation {gi_reuse::D3D12FrozenWorkingSet scratch;int* deaths;explicit Invocation(int& n):deaths(&n){}~Invocation(){++*deaths;}};
#include "test-gi-list-destruction.inl"
int main()try {
 using namespace gi_reuse;Device d(false);TestNativeGITextures(d);
 auto layout=D3D12FrozenWorkingSet::Describe(18,3,16);Require(bool(layout)&&layout.count==27,"bad compact count");
 Require(!D3D12FrozenWorkingSet::Describe(17,3,16),"odd width admitted");
 Require(!D3D12FrozenWorkingSet::Describe(16384,16384,16),"oversized training admitted");
 auto fourK=D3D12FrozenWorkingSet::Describe(3840,2160,65536);
 Require(fourK.bytes[0]==149299200&&fourK.bytes[1]==16588800,"4K working allocation changed");
 D3D12FrozenOwnerCache cache(4*1024*1024);
 D3D12SplitSubmissionRouter<D3D12FrozenInvocation> uninitializedRouter;
 std::unique_ptr<D3D12FrozenInvocation> rejectedOwner;
 Require(FAILED(cache.Prepare(uninitializedRouter,{},rejectedOwner))&&!rejectedOwner&&!cache.HeldBytes(),"incomplete cache preparation allocated or retained storage");
 D3D12FrozenInvocation incomplete;
 Require(FAILED(incomplete.Prepare({}))&&!incomplete.AllocationBytes(),"incomplete live invocation prepared");
 Require(!incomplete.AcceptProducer(d.list.Get(),nullptr),"unprepared invocation admitted producer");
 Require(incomplete.RecordConsumer(d.list.Get(),{},nullptr)==D3D12FrozenCompletion::Result::RejectedBeforeCommands,
  "unprepared invocation admitted completion");
 // Whole-invocation preflight includes nine real committed buffers, before
 // allocating any private GPU storage. A tiny logical budget must not pass.
 auto nativeTable=d.Buffer(512,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON);
 auto nativeVertices=d.Buffer(256,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON);
 BufferStorageLease vertexLease;vertexLease.resource=nativeVertices;vertexLease.address=nativeVertices->GetGPUVirtualAddress();
 vertexLease.bytes=256;vertexLease.kind=BufferStorageKind::Committed;
 PTNativeBinding::Binding binding{};binding.indices=vertexLease.address;binding.attributes=vertexLease.address+64;
 binding.constants[1]=2;binding.constants[8]=10;binding.constants[9]=64;
 std::vector<PTNativeBinding::Binding> cpuBindings{binding};
 std::vector<D3D12NativeHitSnapshot::Source> sourceLeases{{1,vertexLease,D3D12_RESOURCE_STATE_COMMON}};
 NativeGITable cpuTable;cpuTable.bytes.resize(192);
 D3D12FrozenInvocation::Preparation estimate;estimate.device=d.device.Get();estimate.nativeSbt=nativeTable.Get();
 estimate.nativeBindings=&cpuBindings;estimate.hitSources=&sourceLeases;estimate.nativeHitTable=&cpuTable;
 estimate.budget=4*1024*1024;estimate.ray.width=18;estimate.ray.height=3;estimate.ray.slots=16;
 const auto tableAddress=nativeTable->GetGPUVirtualAddress();
 estimate.dispatch.RayGenerationShaderRecord={tableAddress,32};estimate.dispatch.MissShaderTable={tableAddress+64,32,32};
 estimate.dispatch.HitGroupTable={tableAddress+128,192,96};
 const auto peak=incomplete.RequiredPeakBytes(estimate);
 Require(peak==9*65536&&!incomplete.RetainedAllocationBytes(),"whole owner preflight omitted committed storage or allocated GPU memory");
 estimate.budget=peak-1;Require(!incomplete.RequiredPeakBytes(estimate),"whole owner preflight admitted over budget");
 estimate.budget=peak;Require(incomplete.RequiredPeakBytes(estimate)==peak,"exact committed budget rejected");
 estimate.dispatch.HitGroupTable.SizeInBytes=UINT64_MAX;
 Require(!incomplete.RequiredPeakBytes(estimate)&&!incomplete.RetainedAllocationBytes(),"overflow preflight allocated or admitted storage");
 std::cout<<"PASS whole invocation allocation preflight: nine committed buffers, exact budget, no private allocations and overflow rejection.\n";
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
 Require(!owners.SceneReleaseReady(id),"pending consumer released scene");
 Require(owners.After(d.queue.Get(),second)&&owners.SceneReleaseReady(id),"consumer fence/scene release failed");Check(d.queue->Signal(fence.Get(),2));
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
 // A native Execute exception must not be converted into an empty successful
 // fence signal. It pins the invocation and never admits its consumer.
 {
  D3D12SplitOwnerRetirement<Invocation,2,8*1024*1024> failed;Check(failed.Initialize(d.queue.Get()));
  auto object=std::make_unique<Invocation>(deaths);
  const Recording r{reinterpret_cast<UINT64>(d.list.Get()),generation(d.list.Get())};
  const auto token=failed.Admit(3,r,64,object);Require(token!=0,"failed Execute admission failed");
  auto batch=failed.Before(d.queue.Get(),1,producerLists,generation);
  Require(batch.count==1,"failed Execute ticket missing");
  failed.FailedExecute(d.queue.Get(),batch);
  Require(!batch.count&&!failed.SceneReleaseReady(token)&&!failed.Seal(r)&&!failed.Take(token,retired),
   "failed Execute fabricated submission or scene release");
  failed.DeviceStopped(); // No GPU commands referencing this fixture owner.
 }
 // Drive the real opaque-token bridge into the retirement router. These are
 // empty valid GPU lists: the test checks actual submit/reset ownership only.
 {
  using Router=D3D12SplitSubmissionRouter<Invocation,2,8*1024*1024>;
  static UINT64 releasedScene=0;static unsigned sceneReleases=0;releasedScene=0;sceneReleases=0;
  Router router;Check(router.Initialize(d.queue.Get(),generation,&Router::Pool::NativeSignal,
   +[](UINT64 token)noexcept{releasedScene=token;++sceneReleases;}));
  static Router* active=nullptr;active=&router;
  static const gi_frozen_submission::Callbacks callbacks{
   +[](ID3D12CommandQueue* q,UINT n,ID3D12CommandList*const* l)noexcept -> UINT64{return active->Before(q,n,l);},
   +[](ID3D12CommandQueue* q,UINT64 t,bool submitted)noexcept{active->After(q,t,submitted);}
  };
  gi_frozen_submission::callbacks=&callbacks;
  fixture::gi_compute_hooks::onFrozenRecordingReset=+[](ID3D12CommandList* l,UINT64 g)noexcept{active->Reset(l,g);};
  auto object=std::make_unique<Invocation>(deaths);
  const Recording a{reinterpret_cast<UINT64>(d.list.Get()),generation(d.list.Get())};
  const Recording b{reinterpret_cast<UINT64>(consumer.Get()),generation(consumer.Get())};
  const auto token=router.AdmitScene(9,a,64,object,901);
  Require(token!=0,"router admission failed");Check(d.list->Close());
  {
   gi_frozen_submission::Call call(d.queue.Get(),1,producerLists);
   d.queue->ExecuteCommandLists(1,producerLists);call.Submitted();call.Finish();
  }
  Require(router.Access([&](auto& pool){return pool.Attach(token,9,b)&&pool.ClaimConsumerRecording(token,9,d.queue.Get(),b);}),
   "router did not publish actual producer submission");
  Require(sceneReleases==0,"producer submission released native scene");
  Check(consumer->Close());
  {
   gi_frozen_submission::Call call(d.queue.Get(),1,lists);
   Require(!router.Access([&](auto& pool){return pool.SceneReleaseReady(token);}),"before callback released scene");
   d.queue->ExecuteCommandLists(1,lists);call.Submitted();
  }
  Require(router.Access([&](auto& pool){return pool.SceneReleaseReady(token)&&!pool.Take(token,retired);}),
   "router confused scene release with allocation retirement");
  Require(releasedScene==901&&sceneReleases==1,"actual consumer submission did not release scene once");
  Check(d.queue->Signal(fence.Get(),3));
  HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(done!=nullptr,"router event failed");
  Check(fence->SetEventOnCompletion(3,done));Require(WaitForSingleObject(done,30000)==WAIT_OBJECT_0,"router GPU timeout");CloseHandle(done);
  Check(d.list->Reset(d.allocator.Get(),nullptr));
  Require(!router.Access([&](auto& pool){pool.Poll();return pool.Take(token,retired);}),"single Reset released router owner");
  Check(consumer->Reset(allocator.Get(),nullptr));
  Require(router.Access([&](auto& pool){pool.Poll();return pool.Take(token,retired);}),"router did not retire sealed completed owner");
  retired.reset();
  Require(sceneReleases==1,"Reset released scene twice");
  // No producer Execute: cancellation must wait for its successful Reset.
  object=std::make_unique<Invocation>(deaths);
  const auto cancelled=router.AdmitScene(10,{reinterpret_cast<UINT64>(d.list.Get()),generation(d.list.Get())},64,object,902);
  Require(cancelled&&router.CancelUnattached(cancelled)&&sceneReleases==1,"unsubmitted producer released scene on scope exit");
  Check(d.list->Close());Check(d.list->Reset(d.allocator.Get(),nullptr));
  Require(releasedScene==902&&sceneReleases==2,"sealed cancelled producer did not release scene");
  Require(router.Access([&](auto& pool){return pool.Take(cancelled,retired);}),"cancelled unsubmitted owner did not retire");retired.reset();
  gi_frozen_submission::callbacks=nullptr;fixture::gi_compute_hooks::onFrozenRecordingReset=nullptr;active=nullptr;
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
 TestGIListDestruction(d);
 std::cout<<"PASS split working set with actual Reset/ClearState hooks and private fence: 4K allocation sizing, budget rejection, two-list data preservation, last-consumer lifetime\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
