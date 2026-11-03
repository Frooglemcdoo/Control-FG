#include <windows.h>
#include "../src/gi_reuse/native_gi_writer_snapshot.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <cstdlib>
#include "../src/gi_reuse/d3d12_submission_metadata.h"
#include "../src/gi_reuse/d3d12_scene_traversal_lease.h"
namespace fixture {
inline bool PatchSlot(void** slot,void* expected,void* replacement) noexcept {
 DWORD old=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;
 const bool ok=InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
 DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);return ok;
}
#include "../src/gi_resource_hooks.inl"
}
using Microsoft::WRL::ComPtr;
namespace rt=gi_reuse::runtime;
void CheckAt(HRESULT hr,int line){if(FAILED(hr)){std::fprintf(stderr,"HRESULT failure line=%d hr=0x%08lX\n",line,static_cast<unsigned long>(hr));std::exit(1);}}
#define Check(expression) CheckAt((expression),__LINE__)
static void TestNativeGIWriter(){
 using namespace gi_reuse;using namespace GINativeJobRoute;
 unsigned char buffer[80]{},state[64]{};
 PTShaderTableAdapter::Identifier opaque{},alpha{};opaque[0]=1;alpha[0]=2;
 const void* opaquePtr=opaque.data();const void* alphaPtr=alpha.data();
 const std::uint64_t address=4096,bytes=1024,pipeline=42;
 std::memcpy(buffer+0x48,&address,8);std::memcpy(buffer+0x30,&bytes,8);
 std::memcpy(state,&pipeline,8);std::memcpy(state+0x20,&opaquePtr,8);std::memcpy(state+0x30,&alphaPtr,8);
 // Reflection-only ID slots 1 and 3 are deliberately null.
 PTNativeBinding::Binding bindings[2]{};bindings[1].type=1;bindings[1].constants[3]=77;
 Ticket t;t.serial=1;t.list=2;t.pipeline=pipeline;t.counter=3;t.engineFrame=4;
 t.layout={address,bytes,32,64,32,64,96,4};t.hitTable=address+128;t.hitBytes=384;
 t.writer={reinterpret_cast<std::uintptr_t>(buffer),reinterpret_cast<std::uintptr_t>(state),reinterpret_cast<std::uintptr_t>(bindings),512,1,2};
 NativeGIWriterInputs result;
 assert(SnapshotNativeGIWriter(t,result)&&result.table.Active(2)&&result.table.bytes[192]==2&&result.bindings[1].constants[3]==77);
 const auto saved=t;t.pipeline=43;assert(!SnapshotNativeGIWriter(t,result)&&result.bindings.empty()&&!result.table.pipeline);t=saved;
 t.writer.rays=2;assert(!SnapshotNativeGIWriter(t,result));t=saved;
 t.writer.bytes=511;assert(!SnapshotNativeGIWriter(t,result));t=saved;
 t.hitTable+=96;assert(!SnapshotNativeGIWriter(t,result));t=saved;
 t.writer.bindings=1;assert(!SnapshotNativeGIWriter(t,result)&&result.bindings.empty());t=saved;
 t.writer.state=1;assert(!SnapshotNativeGIWriter(t,result));t=saved;
 bindings[1].type=2;assert(!SnapshotNativeGIWriter(t,result)&&result.table.bytes.empty());bindings[1].type=1;
 alpha.fill(0);assert(!SnapshotNativeGIWriter(t,result));
}
int main(){
 TestNativeGIWriter();
 SetEnvironmentVariableA("CONTROLTOH_GI_METADATA","1");
 SetEnvironmentVariableA("CONTROLTOH_GI_BINDINGS","1");
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList4> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
 // Observe and retain a real resource without mapping or copying its bytes.
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;
 D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=64;buffer.Height=1;
 buffer.DepthOrArraySize=1;buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 ComPtr<ID3D12Resource> owner;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&owner)));
 const auto address=owner->GetGPUVirtualAddress();rt::ObserveBuffer(owner.Get());rt::ObserveBuffer(owner.Get());owner.Reset();
 UINT64 offset=0;assert(rt::owners.Resolve(address+16,48,owner,offset)&&offset==16);
 assert(!rt::owners.Resolve(address+16,49,owner,offset));assert(!rt::owners.Resolve(UINT64_MAX,2,owner,offset));
 unsigned owned=0;UINT64 retained=0;bool limited=false;rt::owners.Stats(owned,retained,limited);assert(owned==1&&retained==64&&!limited);
 // Startup churn exceeds cache capacity; referenced geometry remains protected.
 gi_reuse::BasicBufferOwners<4,256> cache;
 ComPtr<ID3D12Resource> resources[8];
 for(auto& resource:resources)Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&resource)));
 cache.Observe(resources[0].Get());assert(cache.Resolve(resources[0]->GetGPUVirtualAddress(),64,owner,offset));
 for(unsigned i=1;i<8;++i)cache.Observe(resources[i].Get());
 cache.Stats(owned,retained,limited);assert(owned==4&&retained==256&&!limited);
 assert(cache.Resolve(resources[0]->GetGPUVirtualAddress(),64,owner,offset));
 assert(!cache.Resolve(resources[1]->GetGPUVirtualAddress(),64,owner,offset));
 for(unsigned i=5;i<8;++i)assert(cache.Resolve(resources[i]->GetGPUVirtualAddress(),64,owner,offset));
 cache.Observe(resources[1].Get());cache.Stats(owned,retained,limited);assert(limited&&owned==4);
 unsigned protectedCount=0;UINT64 evictions=0;cache.RetentionStats(protectedCount,evictions);assert(protectedCount==4&&evictions==4);
 assert(!cache.Resolve(resources[1]->GetGPUVirtualAddress(),64,owner,offset));
 // Upload churn must not displace default-heap scene buffers before first build.
 gi_reuse::BasicBufferOwners<4,256> priority;
 D3D12_HEAP_PROPERTIES gpuHeap{};gpuHeap.Type=D3D12_HEAP_TYPE_DEFAULT;
 ComPtr<ID3D12Resource> scene;Check(device->CreateCommittedResource(&gpuHeap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&scene)));
 priority.Observe(scene.Get());for(auto& r:resources)priority.Observe(r.Get());
 assert(priority.Resolve(scene->GetGPUVirtualAddress(),64,owner,offset));
 // Shader-readable default buffers outrank unrelated default allocations.
 gi_reuse::BasicBufferOwners<4,256> candidates;
 candidates.Observe(scene.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 ComPtr<ID3D12Resource> defaults[6];
 for(auto& r:defaults){Check(device->CreateCommittedResource(&gpuHeap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&r)));candidates.Observe(r.Get());}
 assert(candidates.Resolve(scene->GetGPUVirtualAddress(),64,owner,offset));
 // Index-capable default buffers survive generic shader-read churn, including
 // promotion through the known-pointer fast path before their first AS lookup.
 gi_reuse::BasicBufferOwners<2,128> indexPriority;
 indexPriority.Observe(scene.Get());indexPriority.Observe(scene.Get(),D3D12_RESOURCE_STATE_INDEX_BUFFER);
 for(auto& r:defaults)indexPriority.Observe(r.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 assert(indexPriority.Resolve(scene->GetGPUVirtualAddress(),64,owner,offset));
 assert(indexPriority.HistoricalEvidence(defaults[0]->GetGPUVirtualAddress(),64).event==gi_reuse::BufferEvent::Evicted);
 // A large lower-priority arrival cannot evict the index candidate, even before
 // its first Resolve protects it. Refusal is atomic under byte-budget pressure.
 gi_reuse::BasicBufferOwners<2,128> indexBudget;
 indexBudget.Observe(scene.Get(),D3D12_RESOURCE_STATE_INDEX_BUFFER);
 auto shaderDesc=buffer;shaderDesc.Width=128;ComPtr<ID3D12Resource> shaderBuffer;
 Check(device->CreateCommittedResource(&gpuHeap,D3D12_HEAP_FLAG_NONE,&shaderDesc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&shaderBuffer)));
 indexBudget.Observe(shaderBuffer.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 assert(indexBudget.HistoricalEvidence(shaderBuffer->GetGPUVirtualAddress(),128).event==gi_reuse::BufferEvent::Rejected);
 assert(indexBudget.Resolve(scene->GetGPUVirtualAddress(),64,owner,offset));
 indexBudget.Stats(owned,retained,limited);assert(owned==1&&retained==64&&limited);
 std::puts("PASS R291 index priority, fast-path promotion, byte-budget refusal and eviction/rejection evidence");
 // Rejected admission is atomic: no useful entries evicted for a buffer that
 // cannot fit even after all permissible eviction.
 gi_reuse::BasicBufferOwners<4,256> admission;admission.Observe(resources[0].Get());
 assert(admission.Resolve(resources[0]->GetGPUVirtualAddress(),64,owner,offset));admission.Observe(resources[1].Get());
 auto largeDesc=buffer;largeDesc.Width=224;ComPtr<ID3D12Resource> large;
 Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&largeDesc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&large)));
 admission.Observe(large.Get());admission.Stats(owned,retained,limited);assert(limited&&owned==2&&retained==128);
 admission.RetentionStats(protectedCount,evictions);assert(evictions==0);
 assert(admission.Resolve(resources[1]->GetGPUVirtualAddress(),64,owner,offset));
 // Excluded AS-storage classification persists across later untyped observations.
 // Synthetic state hint only; WARP does not execute an AS allocation/build here.
 gi_reuse::BasicBufferOwners<4,256> excluded;excluded.Observe(defaults[0].Get());
 excluded.Observe(defaults[0].Get(),D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
 excluded.Observe(defaults[0].Get());assert(!excluded.Resolve(defaults[0]->GetGPUVirtualAddress(),64,owner,offset));
 D3D12_HEAP_PROPERTIES readbackHeap{};readbackHeap.Type=D3D12_HEAP_TYPE_READBACK;ComPtr<ID3D12Resource> readback;
 Check(device->CreateCommittedResource(&readbackHeap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));
 excluded.Observe(readback.Get());excluded.Stats(owned,retained,limited);assert(owned==0&&retained==0);
 // Historical inspection must not pin a candidate that a real build never used.
 gi_reuse::BasicBufferOwners<1,64> inspectOnly;
 inspectOnly.Observe(resources[0].Get());assert(inspectOnly.Resolve(resources[0]->GetGPUVirtualAddress(),64,owner,offset,nullptr,false));
 inspectOnly.RetentionStats(protectedCount,evictions);assert(protectedCount==0);
 inspectOnly.Observe(resources[1].Get());assert(!inspectOnly.Resolve(resources[0]->GetGPUVirtualAddress(),64,owner,offset,nullptr,false));
 assert(inspectOnly.Resolve(resources[1]->GetGPUVirtualAddress(),64,owner,offset));
 inspectOnly.RetentionStats(protectedCount,evictions);assert(protectedCount==1&&evictions==1);
 std::puts("PASS R293 historical inspection leaves candidates evictable; build lookup still protects geometry");
 // Pointer and range indexes must stay consistent through eviction/exclusion.
 gi_reuse::RangeMatch missingReason{};
 assert(!admission.Resolve(1,1,owner,offset,&missingReason)&&missingReason==gi_reuse::RangeMatch::Missing);
 assert(!admission.Resolve(UINT64_MAX,2,owner,offset,&missingReason)&&missingReason==gi_reuse::RangeMatch::Invalid);
 static_assert(gi_reuse::BufferOwners::MaxOwners==8192&&gi_reuse::BufferOwners::Budget==1024ull*1024*1024,"R293 diagnostic extent budget is 1 GiB");
 rt::CheckOwner(1,1,rt::OwnerKind::Vertex);rt::CheckOwner(1,1,rt::OwnerKind::Vertex);rt::CheckOwner(UINT64_MAX,2,rt::OwnerKind::Index);
 std::array<rt::MissingRange,128> misses{};rt::ReadMissing(misses);assert(misses[0].hits==2&&misses[0].reason==gi_reuse::RangeMatch::Missing&&misses[1].reason==gi_reuse::RangeMatch::Invalid);
 // Inventory generations cannot inherit evidence after eviction/re-admission.
 gi_reuse::BasicBufferOwners<1,64> identities;identities.Observe(resources[0].Get());
 const auto firstIdentity=identities.Identity(resources[0].Get());assert(firstIdentity);
 identities.Observe(resources[1].Get());assert(!identities.Identity(resources[0].Get()));
 identities.Observe(resources[0].Get());assert(identities.Identity(resources[0].Get())!=firstIdentity);
 // Actual creation interception discovers a CPU-written buffer without any copy.
 fixture::gi_resource_hooks::queueObserver=+[](ID3D12CommandQueue* q) noexcept {UINT marker=1;q->SetPrivateData(gi_reuse::storageKindKey,sizeof(marker),&marker);};
 fixture::gi_resource_hooks::Install(device.Get());const auto created=rt::creationObservations.load();
 ComPtr<ID3D12Resource> createdBuffer;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&createdBuffer)));
 assert(rt::creationObservations==created+1);assert(rt::owners.Resolve(createdBuffer->GetGPUVirtualAddress(),64,owner,offset));
 D3D12_COMMAND_QUEUE_DESC observedQueueDesc{};ComPtr<ID3D12CommandQueue> observedQueue;
 Check(device->CreateCommandQueue(&observedQueueDesc,IID_PPV_ARGS(&observedQueue)));
 UINT queueMarker=0,queueMarkerBytes=sizeof(queueMarker);Check(observedQueue->GetPrivateData(gi_reuse::storageKindKey,&queueMarkerBytes,&queueMarker));
 assert(queueMarker==1&&fixture::gi_resource_hooks::queueInstalled.load());
 std::puts("PASS future queue creation interception and observer forwarding");
 // Placed-resource path has a distinct native signature and vtable slot.
 D3D12_HEAP_DESC placedHeapDesc{};placedHeapDesc.SizeInBytes=65536;placedHeapDesc.Properties=gpuHeap;placedHeapDesc.Flags=D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
 ComPtr<ID3D12Heap> placedHeap;Check(device->CreateHeap(&placedHeapDesc,IID_PPV_ARGS(&placedHeap)));
 ComPtr<ID3D12Resource> placed;Check(device->CreatePlacedResource(placedHeap.Get(),0,&buffer,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&placed)));
 assert(rt::owners.Resolve(placed->GetGPUVirtualAddress(),64,owner,offset));
 gi_reuse::BufferStorageLease storageLease;
 assert(!gi_reuse::AcquireStorageLease(resources[0].Get(),storageLease)); // Created before hooks.
 assert(gi_reuse::AcquireStorageLease(placed.Get(),storageLease));
 assert(storageLease.kind==gi_reuse::BufferStorageKind::Placed && storageLease.heap.Get()==placedHeap.Get());
 // Independent lease survives the caller releasing its resource/heap references.
 const auto placedAddress=placed->GetGPUVirtualAddress();
 placed.Reset(); placedHeap.Reset();
 assert(storageLease.resource->GetGPUVirtualAddress()==placedAddress && storageLease.heap->GetDesc().SizeInBytes==65536);
 placed=storageLease.resource; placedHeap=storageLease.heap;
 gi_reuse::BufferStorageLease committedLease;
 assert(gi_reuse::AcquireStorageLease(owner.Get(),committedLease)); // Resolved placed owner.
 assert(committedLease.kind==gi_reuse::BufferStorageKind::Placed);
 ComPtr<ID3D12Resource> storageCommitted;
 Check(device->CreateCommittedResource(&gpuHeap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&storageCommitted)));
 assert(gi_reuse::AcquireStorageLease(storageCommitted.Get(),committedLease));
 assert(committedLease.kind==gi_reuse::BufferStorageKind::Committed && !committedLease.heap);
 assert(!gi_reuse::AcquireStorageLease(nullptr,committedLease) && !committedLease.resource);
 // Preflight must distinguish storage proof from mere owner resolution.
 for(auto& c:rt::inputAudit[2])c=0;
 rt::AuditInput(resources[0].Get(),0,4,rt::OwnerKind::Transform);
 rt::AuditInput(placed.Get(),2,6,rt::OwnerKind::Transform);
 rt::AuditInput(storageCommitted.Get(),0,4,rt::OwnerKind::Transform);
 rt::AuditInput(storageCommitted.Get(),buffer.Width-1,2,rt::OwnerKind::Transform);
 assert(rt::inputAudit[2][0]==2 && rt::inputAudit[2][1]==1 && rt::inputAudit[2][2]==1);
 assert(rt::inputAudit[2][4]==1 && rt::inputAudit[2][5]==2);
 std::puts("PASS input preflight: committed/placed/unknown storage, alignment and overrun rejection");
 std::puts("PASS geometry storage lease: observed committed/placed creation, retained backing heap, unknown creation refusal and independent lease lifetime");

 // Synthetic AS classification only: WARP resources are ordinary buffers.
 // Verify lifetime tokens/heap retention without issuing unsupported DXR work.
 gi_reuse::ASResourceOwners<2,128> asRegistry;
 assert(asRegistry.Created(scene.Get()));assert(asRegistry.Created(scene.Get()));
 gi_reuse::ASResourceOwners<2,128>::Lease asLease;
 assert(asRegistry.Resolve(scene->GetGPUVirtualAddress()+16,asLease)&&asLease.token&&asLease.offset==16);
 const auto asToken=asLease.token;
 assert(asRegistry.Created(placed.Get(),placedHeap.Get()));
 assert(asRegistry.Resolve(placed->GetGPUVirtualAddress(),asLease)&&asLease.heap.Get()==placedHeap.Get()&&asLease.token!=asToken);
 assert(!asRegistry.Created(defaults[1].Get()));
 assert(asRegistry.Resolve(scene->GetGPUVirtualAddress(),asLease)&&asLease.token==asToken);
 UINT64 asCount=0,asBytes=0,asRejected=0;asRegistry.Stats(asCount,asBytes,asRejected);
 assert(asCount==2&&asBytes==128&&asRejected==1);
 assert(!asRegistry.Resolve(UINT64_MAX,asLease)&&!asLease.token);
 // Atomic allocation-set acquisition: no partial leases on late failure.
 using ASRegistry=gi_reuse::ASResourceOwners<2,128>;
 assert(asRegistry.Resolve(placed->GetGPUVirtualAddress(),asLease));
 std::vector<ASRegistry::Reference> sceneRefs={{scene->GetGPUVirtualAddress(),asToken},{placed->GetGPUVirtualAddress(),asLease.token}};
 std::vector<ASRegistry::Lease> sceneLeases;
 assert(asRegistry.ResolveBatch(sceneRefs,sceneLeases)&&sceneLeases.size()==2&&sceneLeases[1].heap.Get()==placedHeap.Get());
 gi_reuse::SceneTraversalClosure leaseClosure;
 leaseClosure.cut=1;leaseClosure.epoch=1;leaseClosure.top={sceneRefs[0].address,sceneRefs[0].token,1};
 leaseClosure.bottoms.push_back({sceneRefs[1].address,sceneRefs[1].token,2});
 assert(gi_reuse::RetainTraversalClosure(asRegistry,leaseClosure,sceneLeases)&&sceneLeases.size()==2);
 auto retainedScene=sceneLeases;
 auto wrongRefs=sceneRefs;++wrongRefs[1].token;
 assert(!asRegistry.ResolveBatch(wrongRefs,sceneLeases)&&sceneLeases.empty());
 wrongRefs=sceneRefs;wrongRefs[1].address=UINT64_MAX;
 assert(!asRegistry.ResolveBatch(wrongRefs,sceneLeases)&&sceneLeases.empty());
 assert(!asRegistry.ResolveBatch({},sceneLeases));
 // Retirement is an offline API until every submitted use holds its own lease.
 gi_reuse::ASResourceOwners<2,128>::Lease pendingAS,retiredAS;
 assert(asRegistry.Resolve(scene->GetGPUVirtualAddress(),pendingAS));
 assert(!asRegistry.Retire(scene.Get(),asToken+100,retiredAS)&&!retiredAS.resource);
 assert(asRegistry.Retire(scene.Get(),asToken,retiredAS));
 assert(!asRegistry.Resolve(scene->GetGPUVirtualAddress(),asLease));
 assert(retainedScene[0].resource.Get()==scene.Get());
 assert(!asRegistry.ResolveBatch(sceneRefs,sceneLeases)&&sceneLeases.empty());
 assert(pendingAS.resource->GetGPUVirtualAddress()==scene->GetGPUVirtualAddress());
 assert(retiredAS.token==asToken&&retiredAS.resource.Get()==scene.Get());
 assert(!asRegistry.Retire(scene.Get(),asToken,asLease));
 asRegistry.Stats(asCount,asBytes,asRejected);assert(asCount==1&&asBytes==64);
 // Reuse a freed slot without changing the surviving placed-resource index.
 assert(asRegistry.Created(scene.Get()));
 assert(asRegistry.Resolve(scene->GetGPUVirtualAddress(),asLease)&&asLease.token!=asToken);
 assert(!asRegistry.Retire(scene.Get(),asToken,retiredAS));
 assert(asRegistry.Resolve(placed->GetGPUVirtualAddress(),asLease)&&asLease.heap.Get()==placedHeap.Get());
 const auto placedToken=asLease.token;
 assert(asRegistry.Retire(placed.Get(),placedToken,retiredAS));
 assert(retiredAS.heap.Get()==placedHeap.Get());
 assert(!asRegistry.Resolve(placed->GetGPUVirtualAddress(),asLease));
 for(unsigned cycle=0;cycle<1000;++cycle){
  assert(asRegistry.Created(placed.Get(),placedHeap.Get()));
  assert(asRegistry.Resolve(placed->GetGPUVirtualAddress(),asLease)&&asLease.token>placedToken);
  assert(asRegistry.Retire(placed.Get(),asLease.token,retiredAS));
 }
 asRegistry.Stats(asCount,asBytes,asRejected);assert(asCount==1&&asBytes==64);
 std::puts("PASS atomic AS set acquisition: exact tokens, partial failure rollback, placed heaps and independent retirement lifetime");
 std::puts("PASS AS retirement: exact generation, stale/duplicate refusal, independent leases, placed heap retention and 1000 slot reuse cycles");
 rt::ObserveCreated(defaults[4].Get(),D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,true);
 const auto knownIdentity=rt::ASIdentities(defaults[4]->GetGPUVirtualAddress(),defaults[4]->GetGPUVirtualAddress());
 assert(knownIdentity.first&&knownIdentity.first==knownIdentity.second);
 rt::ObserveCreated(defaults[1].Get(),D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,false);
 assert(!rt::ASIdentities(defaults[1]->GetGPUVirtualAddress(),0).first);
 std::puts("PASS AS resource identity: retained committed/placed owners, distinct tokens, bounded refusal and unsupported-storage rejection");
 // Buffers created before hook installation are discovered at native barriers.
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barrier.UAV.pResource=scene.Get();
 rt::ObserveBarriers(1,&barrier);assert(rt::owners.Resolve(scene->GetGPUVirtualAddress(),64,owner,offset));
 rt::Initialize(list.Get());auto initial=rt::Read(list.Get());assert(initial&&initial->data.size==0);
 D3D12_RESOURCE_BARRIER stateBarrier{};stateBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
 stateBarrier.Transition.pResource=createdBuffer.Get();stateBarrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
 stateBarrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;stateBarrier.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 rt::RecordBarriers(list.Get(),1,&stateBarrier);
 rt::CheckOwner(createdBuffer->GetGPUVirtualAddress(),32,rt::OwnerKind::Vertex,&initial->states);
 assert(rt::localStateCounts[0][1]==1);
 stateBarrier.Flags=D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;rt::RecordBarriers(list.Get(),1,&stateBarrier);
 rt::CheckOwner(createdBuffer->GetGPUVirtualAddress(),32,rt::OwnerKind::Vertex,&initial->states);assert(rt::localStateCounts[0][2]==1);
 D3D12_RESOURCE_BARRIER alias{};alias.Type=D3D12_RESOURCE_BARRIER_TYPE_ALIASING;rt::RecordBarriers(list.Get(),1,&alias);
 rt::CheckOwner(createdBuffer->GetGPUVirtualAddress(),32,rt::OwnerKind::Vertex,&initial->states);assert(rt::localStateCounts[0][0]==1);
 std::puts("PASS R294 runtime recording evidence, pending split, alias invalidation and inventory-generation isolation");
 const auto generation=initial->data.generation;rt::Initialize(list.Get());assert(rt::Read(list.Get())->data.generation==generation);
 D3D12_RAYTRACING_GEOMETRY_DESC geometry{};geometry.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
 geometry.Triangles.VertexBuffer={4096,12};geometry.Triangles.VertexCount=3;geometry.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;
 geometry.Triangles.IndexBuffer=8192;geometry.Triangles.IndexCount=3;geometry.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.DestAccelerationStructureData=256;
 build.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;build.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;
 build.Inputs.NumDescs=1;build.Inputs.pGeometryDescs=&geometry;
 gi_reuse::GeometryDiagnostic detail;
 geometry.Triangles.VertexBuffer.StrideInBytes=16;
 auto padded=gi_reuse::CopyBLASGeometry(build.Inputs,&detail);assert(padded&&gi_reuse::GeometryMetadata::VertexBytes(*padded->At(0))==44);
 geometry.Triangles.IndexFormat=DXGI_FORMAT_R32_UINT;
 assert(gi_reuse::CopyBLASGeometry(build.Inputs,&detail));
 geometry.Triangles.IndexFormat=DXGI_FORMAT_R32_FLOAT;
 assert(!gi_reuse::CopyBLASGeometry(build.Inputs,&detail)&&detail.reason==gi_reuse::GeometryReject::IndexFormat&&detail.index==0&&detail.triangle.stride==16);
 geometry.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;geometry.Triangles.VertexBuffer.StrideInBytes=12;
 const D3D12_RAYTRACING_GEOMETRY_DESC* pointers[]={&geometry};build.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS;build.Inputs.ppGeometryDescs=pointers;
 assert(gi_reuse::CopyBLASGeometry(build.Inputs,&detail));
 pointers[0]=nullptr;assert(!gi_reuse::CopyBLASGeometry(build.Inputs,&detail)&&detail.reason==gi_reuse::GeometryReject::NullGeometry);
 build.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;build.Inputs.pGeometryDescs=&geometry;
 rt::RememberGeometryReject(build,detail);rt::RejectedGeometry saved;assert(rt::ReadGeometryReject(saved)&&saved.diagnostic.reason==gi_reuse::GeometryReject::NullGeometry&&saved.count==1);
 std::puts("PASS R298 padded stride, exact vertex span, pointer-array parsing and retained rejection fields");
 // Synthetic metadata only. WARP executes an empty list, not a fake AS build.
 rt::RecordBuild(list.Get(),&build);rt::RecordCopy(list.Get(),512,256,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
 geometry.Triangles.VertexBuffer.StartAddress=16384;
 assert(rt::Read(list.Get())->data.events[0].record.geometry->At(0)->vertices==4096);
 Check(list->Close());ID3D12CommandList* lists[]={list.Get()};
 const auto token=rt::BeforeExecute(queue.Get(),1,lists);assert(token&&!rt::fault);
 // Freeze recording metadata before native Execute; resetting private metadata
 // cannot modify that submission's owned snapshot.
 rt::Reset(list.Get());assert(rt::Read(list.Get())->data.generation!=generation&&initial->data.size==2);
 queue->ExecuteCommandLists(1,lists);rt::AfterExecute(queue.Get(),token);assert(!rt::fault);
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);Check(rt::fence->SetEventOnCompletion(token,event));assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
 UINT64 issued=0,completed=0;bool ready=false;rt::CompletionStats(issued,completed,ready);assert(ready&&issued==token&&completed>=token);
 auto snapshot=std::make_unique<rt::MetadataLedger>();assert(rt::Snapshot(*snapshot));assert(snapshot->Find(512)->geometry->At(0)->vertices==4096);
 const auto planned=rt::InspectScenePlan();assert(planned.snapshot&&planned.planned&&planned.fenceValue==token);
 assert(planned.blas==2&&planned.tlas==0&&planned.ranges==4&&planned.resolved+planned.missing==4);
 assert(planned.missing==planned.missingByKind[0]+planned.missingByKind[1]+planned.missingByKind[2]+planned.missingByKind[3]);
 std::puts("PASS R292 completed scene-plan inspection and bounded owner coverage accounting; no GPU bytes read");
 // Real queue blocked on a CPU-controlled fence: both replay submissions must
 // retain the recorded AS resource after registry retirement and metadata reset.
 // Commands remain empty on WARP; AS addresses here are metadata, not DXR work.
 ComPtr<ID3D12Fence> blocker;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&blocker)));
 Check(queue->Wait(blocker.Get(),1));rt::Reset(list.Get());
 const auto savedDestination=build.DestAccelerationStructureData;
 build.DestAccelerationStructureData=defaults[4]->GetGPUVirtualAddress();
 rt::RecordBuild(list.Get(),&build);auto heldJournal=rt::Read(list.Get());
 assert(heldJournal->asLeases.size()==1&&heldJournal->missingASLeases==0);
 const auto heldToken=heldJournal->asTokens[0].first;
 gi_reuse::ASResourceOwners<>::Lease retiredNative;
 assert(rt::asOwners.Retire(defaults[4].Get(),heldToken,retiredNative));
 const auto leaseSubmission=rt::BeforeExecute(queue.Get(),1,lists);assert(leaseSubmission);
 queue->ExecuteCommandLists(1,lists);rt::AfterExecute(queue.Get(),leaseSubmission);
 const auto replaySubmission=rt::BeforeExecute(queue.Get(),1,lists);assert(replaySubmission>leaseSubmission);
 queue->ExecuteCommandLists(1,lists);rt::AfterExecute(queue.Get(),replaySubmission);
 rt::Reset(list.Get());heldJournal.Reset();retiredNative={};rt::PollASLeases();
 unsigned blockedFlights=0;
 for(const auto& flight:rt::asFlights)if(flight.journal&&flight.fenceValue>=leaseSubmission){
  ++blockedFlights;assert(flight.journal->asLeases.at(heldToken).resource.Get()==defaults[4].Get());
 }
 assert(blockedFlights==2&&rt::fence->GetCompletedValue()<leaseSubmission);
 Check(blocker->Signal(1));event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
 Check(rt::fence->SetEventOnCompletion(replaySubmission,event));assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
 rt::PollASLeases();for(const auto& flight:rt::asFlights)assert(!flight.journal);
 build.DestAccelerationStructureData=savedDestination;
 std::puts("PASS AS submission leases: blocked real WARP queue, replay, registry retirement, list reset, fence-gated release");
 // More than 512 events now survive recording, snapshot, and ordered submission.
 rt::Reset(list.Get());for(unsigned i=0;i<1024;++i)rt::RecordBuild(list.Get(),&build);
 assert(!rt::fault&&rt::Read(list.Get())->data.size==1024);
 const auto grown=rt::BeforeExecute(queue.Get(),1,lists);assert(grown&&!rt::fault);
 rt::Reset(list.Get());assert(rt::pending.size==1024);queue->ExecuteCommandLists(1,lists);rt::AfterExecute(queue.Get(),grown);assert(!rt::fault);
 // Known empty submissions on another direct queue cannot invalidate AS metadata,
 // including while an AS-bearing submission is between Before/AfterExecute.
 ComPtr<ID3D12CommandQueue> unrelated;Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&unrelated)));
 ComPtr<ID3D12GraphicsCommandList> empty;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&empty)));
 rt::Initialize(empty.Get());Check(empty->Close());ID3D12CommandList* emptyLists[]={empty.Get()};
 assert(!rt::BeforeExecute(unrelated.Get(),1,emptyLists)&&!rt::fault);
 auto copyDesc=qd;copyDesc.Type=D3D12_COMMAND_LIST_TYPE_COPY;ComPtr<ID3D12CommandQueue> copyQueue;Check(device->CreateCommandQueue(&copyDesc,IID_PPV_ARGS(&copyQueue)));
 ComPtr<ID3D12CommandAllocator> copyAllocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,IID_PPV_ARGS(&copyAllocator)));
 ComPtr<ID3D12GraphicsCommandList> copyList;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_COPY,copyAllocator.Get(),nullptr,IID_PPV_ARGS(&copyList)));
 ID3D12CommandList* copyLists[]={copyList.Get()};assert(!rt::BeforeExecute(copyQueue.Get(),1,copyLists)&&!rt::fault);
 rt::RecordBuild(list.Get(),&build);const auto concurrent=rt::BeforeExecute(queue.Get(),1,lists);assert(concurrent);
 const auto preservedSize=rt::pending.size;
 assert(!rt::BeforeExecute(unrelated.Get(),1,emptyLists)&&!rt::fault&&rt::pending.size==preservedSize&&rt::activeToken==concurrent);
 assert(!rt::BeforeExecute(copyQueue.Get(),1,copyLists)&&!rt::fault&&rt::activeToken==concurrent);
 queue->ExecuteCommandLists(1,lists);rt::AfterExecute(queue.Get(),concurrent);assert(!rt::fault);
 // A genuinely AS-bearing second queue remains rejected.
 rt::failureReason=nullptr;assert(!rt::BeforeExecute(unrelated.Get(),1,lists)&&rt::fault);
 assert(std::string(rt::failureReason.load())=="multiple_as_queues");rt::fault=false;rt::failureReason=nullptr;
 // Missing coverage must invalidate even an otherwise valid prior snapshot.
 ComPtr<ID3D12GraphicsCommandList> unknown;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&unknown)));
 ID3D12CommandList* missing[]={unknown.Get()};assert(!rt::BeforeExecute(queue.Get(),1,missing)&&rt::fault);assert(!rt::Snapshot(*snapshot));
 // Isolated rejection fixtures reset fault solely for testing, never in runtime.
 rt::fault=false;build.Inputs.Flags=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
 rt::failureReason=nullptr;rt::RecordBuild(list.Get(),&build);assert(rt::fault&&std::string(rt::failureReason.load())=="update_missing_source");rt::fault=false;build.Inputs.Flags=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
 rt::Reset(list.Get());rt::RecordBuild(list.Get(),&build);
 const auto pending=rt::BeforeExecute(queue.Get(),1,lists);assert(pending);
 assert(!rt::BeforeExecute(queue.Get(),1,lists)&&rt::fault);rt::AfterExecute(queue.Get(),pending);assert(!rt::submitting);
 rt::fault=false;rt::Reset(list.Get());rt::RecordCopy(list.Get(),1024,256,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE);assert(rt::fault);
 rt::fault=false;rt::failureReason=nullptr;rt::Reset(list.Get());
 for(unsigned i=0;i<65537;++i)rt::RecordBuild(list.Get(),&build);
 assert(rt::fault&&std::string(rt::failureReason.load())=="build_journal_capacity");
 std::puts("PASS runtime journal ownership, descriptor snapshots, submission freeze, real queue fence, reset lifetime and fail-closed coverage/update/concurrency/copy modes; no GPU geometry bytes tested");
}
