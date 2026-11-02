#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include <string>
#include "../src/gi_reuse/d3d12_submission_metadata.h"
using Microsoft::WRL::ComPtr;
namespace rt=gi_reuse::runtime;
void Check(HRESULT hr){assert(SUCCEEDED(hr));}
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
 for(unsigned i=0;i<513;++i)rt::RecordBuild(list.Get(),&build);
 assert(rt::fault&&std::string(rt::failureReason.load())=="build_journal_capacity");
 std::puts("PASS runtime journal ownership, descriptor snapshots, submission freeze, real queue fence, reset lifetime and fail-closed coverage/update/concurrency/copy modes; no GPU geometry bytes tested");
}
