#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <cstdlib>
#include "../src/gi_reuse/d3d12_submission_metadata.h"
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
int main(){
 SetEnvironmentVariableA("CONTROLTOH_GI_METADATA","1");
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
 // Actual creation interception discovers a CPU-written buffer without any copy.
 fixture::gi_resource_hooks::Install(device.Get());const auto created=rt::creationObservations.load();
 ComPtr<ID3D12Resource> createdBuffer;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&createdBuffer)));
 assert(rt::creationObservations==created+1);assert(rt::owners.Resolve(createdBuffer->GetGPUVirtualAddress(),64,owner,offset));
 // Placed-resource path has a distinct native signature and vtable slot.
 D3D12_HEAP_DESC placedHeapDesc{};placedHeapDesc.SizeInBytes=65536;placedHeapDesc.Properties=gpuHeap;placedHeapDesc.Flags=D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
 ComPtr<ID3D12Heap> placedHeap;Check(device->CreateHeap(&placedHeapDesc,IID_PPV_ARGS(&placedHeap)));
 ComPtr<ID3D12Resource> placed;Check(device->CreatePlacedResource(placedHeap.Get(),0,&buffer,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&placed)));
 assert(rt::owners.Resolve(placed->GetGPUVirtualAddress(),64,owner,offset));
 // Buffers created before hook installation are discovered at native barriers.
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barrier.UAV.pResource=scene.Get();
 rt::ObserveBarriers(1,&barrier);assert(rt::owners.Resolve(scene->GetGPUVirtualAddress(),64,owner,offset));
 rt::Initialize(list.Get());auto initial=rt::Read(list.Get());assert(initial&&initial->data.size==0);
 const auto generation=initial->data.generation;rt::Initialize(list.Get());assert(rt::Read(list.Get())->data.generation==generation);
 D3D12_RAYTRACING_GEOMETRY_DESC geometry{};geometry.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
 geometry.Triangles.VertexBuffer={4096,12};geometry.Triangles.VertexCount=3;geometry.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;
 geometry.Triangles.IndexBuffer=8192;geometry.Triangles.IndexCount=3;geometry.Triangles.IndexFormat=DXGI_FORMAT_R16_UINT;
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.DestAccelerationStructureData=256;
 build.Inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;build.Inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;
 build.Inputs.NumDescs=1;build.Inputs.pGeometryDescs=&geometry;
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
 auto snapshot=std::make_unique<gi_reuse::ASLedger<8192>>();assert(rt::Snapshot(*snapshot));assert(snapshot->Find(512)->geometry->At(0)->vertices==4096);
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
