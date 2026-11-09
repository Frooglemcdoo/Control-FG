#include "../../src/rt_ptlas_nvapi.h"
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace rt_ptlas;
struct GPU {
 ComPtr<ID3D12Device5> device;
 ComPtr<ID3D12CommandQueue> queue;
 ComPtr<ID3D12CommandAllocator> allocator;
 ComPtr<ID3D12GraphicsCommandList4> list;
 ComPtr<ID3D12Fence> fence;
 ComPtr<ID3D12InfoQueue> errors;
 HANDLE event=nullptr;
 UINT64 serial=0;
 GPU(){
  ComPtr<ID3D12Debug> debug;
  if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
  ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"DXGI factory");
  ComPtr<IDXGIAdapter1> adapter;
  for(UINT i=0;factory->EnumAdapters1(i,&adapter)!=DXGI_ERROR_NOT_FOUND;++i){
   DXGI_ADAPTER_DESC1 desc{};Check(adapter->GetDesc1(&desc),"adapter desc");
   if(desc.VendorId==0x10de && !(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE) && SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_1,IID_PPV_ARGS(&device)))){
    std::cout<<"NVIDIA adapter device_id="<<desc.DeviceId<<" dedicated_vram="<<desc.DedicatedVideoMemory<<"\n";break;
   }adapter.Reset();
  }
  if(!device)throw std::runtime_error("No NVIDIA DXR device");
  D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};Check(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&options,sizeof(options)),"DXR tier");
  if(options.RaytracingTier<D3D12_RAYTRACING_TIER_1_1)throw std::runtime_error("DXR 1.1 required by validation shader");
  device.As(&errors);
  D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
  Check(device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator)),"allocator");
  Check(device->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
  Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");
  event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("fence event");
 }
 ~GPU(){if(event)CloseHandle(event);}
 void Flush(){
  Check(list->Close(),"close");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
  Check(queue->Signal(fence.Get(),++serial),"signal");Check(fence->SetEventOnCompletion(serial,event),"set fence event");
  if(WaitForSingleObject(event,30000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU completion failed or timed out");
  Check(device->GetDeviceRemovedReason(),"device removal");
  if(errors){for(UINT64 i=0;i<errors->GetNumStoredMessages();++i){SIZE_T bytes=0;Check(errors->GetMessage(i,nullptr,&bytes),"debug message size");std::vector<char> b(bytes);
   auto* m=reinterpret_cast<D3D12_MESSAGE*>(b.data());Check(errors->GetMessage(i,m,&bytes),"debug message");
   if(m->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(m->pDescription);
  }errors->ClearStoredMessages();}
  Check(allocator->Reset(),"reset allocator");Check(list->Reset(allocator.Get(),nullptr),"reset list");
 }
};
static ComPtr<ID3D12Resource> Upload(GPU& g,const void* bytes,std::size_t size){
 auto r=Buffer(g.device.Get(),size,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
 void* p=nullptr;D3D12_RANGE noRead{0,0};Check(r->Map(0,&noRead,&p),"upload map");std::memcpy(p,bytes,size);r->Unmap(0,nullptr);return r;
}
using Hits=std::array<std::array<std::uint32_t,4>,8>;
struct Probe {
 ComPtr<ID3D12RootSignature> root;
 ComPtr<ID3D12PipelineState> pipeline;
 ComPtr<ID3D12Resource> output,readback;
 explicit Probe(GPU& g){
  std::ifstream f("probe.dxil",std::ios::binary);std::vector<char> code((std::istreambuf_iterator<char>(f)),{});if(code.empty())throw std::runtime_error("probe.dxil missing");
  D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
  for(auto& v:p){v.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;v.Descriptor.ShaderRegister=0;}
  D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=2;desc.pParameters=p;
  ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"serialize root");
  Check(g.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)),"root");
  D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=root.Get();ps.CS={code.data(),code.size()};
  Check(g.device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipeline)),"probe pipeline");
  output=Buffer(g.device.Get(),sizeof(Hits),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  readback=Buffer(g.device.Get(),sizeof(Hits),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 }
 Hits Trace(GPU& g,D3D12_GPU_VIRTUAL_ADDRESS scene){
  g.list->SetComputeRootSignature(root.Get());g.list->SetPipelineState(pipeline.Get());
  g.list->SetComputeRootShaderResourceView(0,scene);g.list->SetComputeRootUnorderedAccessView(1,output->GetGPUVirtualAddress());g.list->Dispatch(1,1,1);
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=output.Get();
  b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
  g.list->ResourceBarrier(1,&b);g.list->CopyResource(readback.Get(),output.Get());
  std::swap(b.Transition.StateBefore,b.Transition.StateAfter);g.list->ResourceBarrier(1,&b);g.Flush();
  void* data=nullptr;D3D12_RANGE range{0,sizeof(Hits)};Check(readback->Map(0,&range,&data),"read hits");Hits hits{};std::memcpy(&hits,data,sizeof(hits));D3D12_RANGE noWrite{0,0};readback->Unmap(0,&noWrite);return hits;
 }
};
static ComPtr<ID3D12Resource> MakeBLAS(GPU& g){
 const float vertices[]={-0.75f,-0.75f,0, 0,0.75f,0, 0.75f,-0.75f,0};auto upload=Upload(g,vertices,sizeof(vertices));
 D3D12_RAYTRACING_GEOMETRY_DESC geometry{};geometry.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;geometry.Flags=D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
 geometry.Triangles.VertexBuffer={upload->GetGPUVirtualAddress(),12};geometry.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;geometry.Triangles.VertexCount=3;
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS in{};in.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;in.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;in.NumDescs=1;in.pGeometryDescs=&geometry;
 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO size{};g.device->GetRaytracingAccelerationStructurePrebuildInfo(&in,&size);
 auto result=Buffer(g.device.Get(),size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 auto scratch=Buffer(g.device.Get(),size.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};d.Inputs=in;d.DestAccelerationStructureData=result->GetGPUVirtualAddress();d.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
 g.list->BuildRaytracingAccelerationStructure(&d,0,nullptr);UAVBarrier(g.list.Get(),result.Get());g.Flush();return result;
}
static Hits Native(GPU& g,Probe& probe,const std::vector<Instance>& instances){
 static_assert(sizeof(Instance)==sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
 // A valid zero-count TLAS is deliberately included in the fixture sequence.
 Instance empty{};auto input=Upload(g,instances.empty()?&empty:instances.data(),instances.empty()?sizeof(empty):instances.size()*sizeof(Instance));
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS in{};in.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;in.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;
 in.NumDescs=static_cast<UINT>(instances.size());in.InstanceDescs=input->GetGPUVirtualAddress();
 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO size{};g.device->GetRaytracingAccelerationStructurePrebuildInfo(&in,&size);
 auto scene=Buffer(g.device.Get(),size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 auto scratch=Buffer(g.device.Get(),(std::max)(size.ScratchDataSizeInBytes,UINT64(256)),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};d.Inputs=in;d.DestAccelerationStructureData=scene->GetGPUVirtualAddress();d.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
 g.list->BuildRaytracingAccelerationStructure(&d,0,nullptr);UAVBarrier(g.list.Get(),scene.Get());
 // Trace waits for the fence before any input, scratch or scene is released.
 return probe.Trace(g,scene->GetGPUVirtualAddress());
}
int main(){try{
 GPU g;Probe probe(g);auto blas=MakeBLAS(g);
 CheckNv(NvAPI_Initialize(),"NvAPI initialize");
 NVAPI_D3D12_RAYTRACING_CLUSTER_OPERATIONS_CAPS cluster{};const auto clusterStatus=NvAPI_D3D12_GetRaytracingCaps(g.device.Get(),NVAPI_D3D12_RAYTRACING_CAPS_TYPE_CLUSTER_OPERATIONS,&cluster,sizeof(cluster));
 std::cout<<"Cluster query status="<<clusterStatus<<" caps="<<unsigned(cluster)<<"\n";
 std::vector<Instance> v(4);for(unsigned i=0;i<4;++i){v[i].transform[0]=v[i].transform[5]=v[i].transform[10]=1;v[i].transform[3]=float(i)*2;v[i].idMask=0xff000007;v[i].contributionFlags=0x01000000;v[i].blas=blas->GetGPUVirtualAddress();}
 std::vector<std::vector<Instance>> scenes{v,v};v[1].transform[3]=8;scenes.push_back(v);v[2].idMask=7;scenes.push_back(v);
 v.resize(2);scenes.push_back(v);v.push_back(scenes[0][2]);v[2].transform[3]=10;scenes.push_back(v);scenes.push_back({});
 const char* names[]={"initial","unchanged","rigid move","mask off","destroy/remove","slot reuse","empty scene"};
 std::vector<Hits> expected;for(const auto& s:scenes)expected.push_back(Native(g,probe,s));
 // All native TLAS objects have been fenced and released before PTLAS exists.
 NvBackend replacement(g.device.Get(),4,2);Planner planner(4,2);
 for(std::size_t i=0;i<scenes.size();++i){std::vector<std::uint32_t> partitions(scenes[i].size());for(std::size_t j=0;j<partitions.size();++j)partitions[j]=j==1?0xffffffffu:static_cast<std::uint32_t>(j%2);
  auto plan=planner.Prepare(scenes[i],partitions);auto keepAlive=replacement.Record(g.list.Get(),plan);
  const auto actual=probe.Trace(g,replacement.Address());replacement.Complete();planner.Commit(std::move(plan));
  if(actual!=expected[i]){for(unsigned r=0;r<8;++r)if(actual[r]!=expected[i][r])std::cerr<<"mismatch case="<<names[i]<<" ray="<<r<<" actual_id="<<actual[r][0]<<" expected_id="<<expected[i][r][0]<<" actual_index="<<actual[r][1]<<" expected_index="<<expected[i][r][1]<<"\n";throw std::runtime_error("native/PTLAS hit mismatch");}
  std::cout<<"PASS "<<names[i]<<"\n";
 }
 if(replacement.recordedBuilds!=6)throw std::runtime_error("Unexpected PTLAS build count; unchanged scene must not rebuild");
 std::cout<<"PASS PTLAS parity; native_tlas_builds_during_ptlas=0; ptlas_builds=6\n";
 std::cout<<"Standalone synthetic validation only. Control integration and Mega Geometry are not enabled.\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
