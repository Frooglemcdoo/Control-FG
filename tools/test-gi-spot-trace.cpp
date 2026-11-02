#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <fstream>
#include <string>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cmath>
#include "../build/gi_spot_trace.generated.h"
using Microsoft::WRL::ComPtr;
void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(hr)));}
void Require(bool b,const char* what){if(!b)throw std::runtime_error(what);}
std::vector<unsigned char> Read(const char* p){std::ifstream f(p,std::ios::binary);Require(bool(f),p);return {(std::istreambuf_iterator<char>(f)),{}};}
constexpr UINT capacity=65536,stride=256,bytes=16+capacity*stride,allocated=bytes+256;
ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=type;hp.CreationNodeMask=hp.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=allocated;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 ComPtr<ID3D12Resource> p;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&p)));return p;
}
void Transition(ID3D12GraphicsCommandList* l,ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){D3D12_RESOURCE_BARRIER x{};x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;x.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};l->ResourceBarrier(1,&x);}
int main()try{
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
 ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d)));
 // Exact observed eight-parameter native layout, plus our raw UAV in space 2.
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,64,0,1,UINT_MAX},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,244000,0,1,UINT_MAX}};
 D3D12_ROOT_PARAMETER parameters[10]{};
 parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={3,ranges};
 parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,ranges+3};
 for(UINT i=2;i<6;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor={i-2,0};}
 parameters[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[6].DescriptorTable={1,ranges+4};
 parameters[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[7].DescriptorTable={1,ranges+5};
 parameters[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[8].Descriptor={0,2};
 parameters[9].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[9].Constants={0,2,4};
#ifdef CONTROL_TEST_GI_JOINT_TRACE
 constexpr UINT rootCount=10;
#else
 constexpr UINT rootCount=9;
#endif
 D3D12_ROOT_SIGNATURE_DESC rd{rootCount,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
#ifdef CONTROL_TEST_GI_JOINT_TRACE
 const auto& shader=gi_spot_trace_shader::jointShader;const auto embedded=Read("build/gi-joint-light-6.dxbc");
#elif defined(CONTROL_TEST_GI_RAY_TRACE)
 const auto& shader=gi_spot_trace_shader::rayShader;const auto embedded=Read("build/gi-ray-trace-6.dxbc");
#else
 const auto& shader=gi_spot_trace_shader::shaders[0];const auto embedded=Read("build/gi-spot-trace-6.dxbc");
#endif
Require(embedded.size()==shader.size&&!memcmp(embedded.data(),shader.bytes,shader.size),"embedded bytes differ");
 ComPtr<ID3DBlob> disassembly;Check(D3DDisassemble(shader.bytes,shader.size,0,nullptr,&disassembly));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.bytes,shader.size};ComPtr<ID3D12PipelineState> nativeTrace;Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&nativeTrace)));
#ifdef CONTROL_TEST_GI_JOINT_TRACE
 auto oracle=Read("build/gi-joint-light-oracle.dxbc");
 constexpr UINT expectedRecords=11;
#elif defined(CONTROL_TEST_GI_RAY_TRACE)
 auto oracle=Read("build/gi-ray-trace-oracle.dxbc");
 constexpr UINT expectedRecords=3;
#else
 auto oracle=Read("build/gi-spot-trace-oracle.dxbc");
 constexpr UINT expectedRecords=4;
#endif
pd.CS={oracle.data(),oracle.size()};ComPtr<ID3D12PipelineState> oraclePso;Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&oraclePso)));
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
 ComPtr<ID3D12Fence> fence;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");
#ifdef CONTROL_TEST_GI_JOINT_TRACE
 auto volumeConstants=Buffer(d.Get(),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE);
 void* constants=nullptr;D3D12_RANGE emptyRange{0,0};Check(volumeConstants->Map(0,&emptyRange,&constants));memset(constants,0,256);
 const UINT volumeData[8]={0,0,0,1,0x3f000000,0x3f000000,0x3f000000,0};memcpy(constants,volumeData,sizeof(volumeData));volumeConstants->Unmap(0,nullptr);
#endif
 for(UINT run=0;run<2;++run){
  auto gpu=Buffer(d.Get(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  auto upload=Buffer(d.Get(),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE);
  auto readback=Buffer(d.Get(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE);
  void* mapped=nullptr;D3D12_RANGE none{0,0};Check(upload->Map(0,&none,&mapped));memset(mapped,0,allocated);if(run){UINT seed=capacity-1;memcpy(mapped,&seed,4);}memset(static_cast<unsigned char*>(mapped)+bytes,0xa5,256);
#ifdef CONTROL_TEST_GI_JOINT_TRACE
  const UINT coordinate=run?31:16;UINT header[3]={0x283,coordinate,coordinate};memcpy(static_cast<unsigned char*>(mapped)+4,header,sizeof(header));
#endif
  upload->Unmap(0,nullptr);
  Check(allocator->Reset());Check(list->Reset(allocator.Get(),oraclePso.Get()));list->CopyBufferRegion(gpu.Get(),0,upload.Get(),0,allocated);
  Transition(list.Get(),gpu.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  list->SetComputeRootSignature(root.Get());list->SetComputeRootUnorderedAccessView(8,gpu->GetGPUVirtualAddress());
#ifdef CONTROL_TEST_GI_JOINT_TRACE
  list->SetComputeRootConstantBufferView(4,volumeConstants->GetGPUVirtualAddress());
  const UINT roi[4]={coordinate,coordinate,1,1};list->SetComputeRoot32BitConstants(9,4,roi,0);
#endif
  list->Dispatch(1,1,1);
  Transition(list.Get(),gpu.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),0,gpu.Get(),0,allocated);Check(list->Close());
  ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(fence.Get(),run+1));Check(fence->SetEventOnCompletion(run+1,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"GPU timeout");
  D3D12_RANGE range{0,allocated};Check(readback->Map(0,&range,&mapped));auto* p=static_cast<unsigned char*>(mapped);UINT total=0;memcpy(&total,p,4);Require(total==(run?capacity+expectedRecords-1:expectedRecords),"record allocation count");
  for(UINT j=0;j<256;++j)Require(p[bytes+j]==0xa5,"overflow touched guard");
  for(UINT sample=0;sample<(run?1u:expectedRecords);++sample){const UINT index=run?capacity-1:sample;auto* record=p+16+index*stride;UINT id[4]{},valid=0;memcpy(id,record,16);memcpy(&valid,record+192,4);
#if defined(CONTROL_TEST_GI_RAY_TRACE) || defined(CONTROL_TEST_GI_JOINT_TRACE)
   UINT raySample=sample,expectedCoordinate=16;
#ifdef CONTROL_TEST_GI_JOINT_TRACE
   expectedCoordinate=coordinate;
   if(sample==0){
    auto f=[&](UINT offset){float value=0;memcpy(&value,record+offset,4);return value;};
    auto u=[&](UINT offset){UINT value=0;memcpy(&value,record+offset,4);return value;};
    Require(id[0]==coordinate&&id[1]==coordinate&&id[2]==0&&id[3]==3,"joint spotlight identity/ROI");
    Require(f(16)==.5f&&f(20)==1&&f(24)==2,"joint weighted spotlight term");
    Require(f(80)==10&&f(112)==2&&f(128)==6&&f(132)==7&&f(136)==8&&f(140)==.5f,"joint native operands");
    Require(u(144)==6&&u(148)==2&&u(152)==7&&u(156)==42&&u(172)==0x281,"joint context");
    continue;
   }
   if(sample>=1&&sample<=3){
    auto f=[&](UINT offset){float value=0;memcpy(&value,record+offset,4);return value;};
    auto u=[&](UINT offset){UINT value=0;memcpy(&value,record+offset,4);return value;};
    Require(id[0]==coordinate&&id[1]==coordinate&&id[2]==0&&id[3]==4,"candidate identity/ROI");
    Require(u(100)==5+sample&&u(104)==7&&u(108)==42&&u(176)==0x282&&u(180)==2,"candidate context");
    Require(u(76)==(sample==1?63u:sample==2?7u:1u),"candidate validity/reset");
    if(sample==1){
     Require(f(16)==.25f&&f(20)==.75f&&f(24)==.5f&&f(28)==2,"candidate projection");
     Require(f(32)==1&&f(36)==.5f&&f(40)==.25f&&f(44)==1,"candidate visibility");
     Require(f(48)==.8f&&f(52)==.5f&&f(56)==.5f&&f(60)==1,"candidate response");
     Require(f(64)==.5f&&f(68)==.25f&&f(72)==1&&u(96)==257,"candidate cookie/flags");
     Require(f(80)==.25f&&f(84)==.75f&&f(88)==.5f&&f(92)==.6f,"candidate shadow lookup");
     Require(f(112)==0&&f(120)==1&&f(136)==1&&f(144)==.1f&&f(148)==.2f,"candidate directions/UV");
     Require(f(192)==4&&f(196)==8&&f(200)==2&&f(208)==2&&f(224)==2,"candidate product chain");
    }else{
     Require(f(44)==0&&f(64)==1&&f(68)==1&&f(72)==1&&f(192)==0&&f(208)==0&&f(224)==0,"candidate reject stale values");
    }
    continue;
   }
   if(sample==4||sample==5||sample==7||sample==8){
    auto f=[&](UINT offset){float value=0;memcpy(&value,record+offset,4);return value;};
    auto u=[&](UINT offset){UINT value=0;memcpy(&value,record+offset,4);return value;};
    const bool summary=sample==5||sample==8;const UINT ray=sample>=7?1:0;
    Require(id[0]==coordinate&&id[1]==coordinate&&id[2]==ray&&id[3]==(summary?5u:6u),"volume identity");
    if(summary){
     Require(u(144)==42&&u(148)==2&&u(152)==(ray?65535u:7u)&&u(156)==0x283&&u(160)==1,"volume summary metadata");
     Require(f(16)==5&&f(24)==7&&f(36)==1&&f(56)==1,"volume lookup snapshots");
     Require(f(64)==1&&f(72)==3&&f(76)==2&&f(80)==.5f&&f(88)==1.5f,"volume blend snapshots");
     Require(f(96)==.5f&&f(104)==.5f&&f(112)==1&&f(120)==2&&f(140)==2,"volume ambient and final");
    }else{
     Require(u(80)==0&&u(84)==123&&u(96)==42&&u(100)==2&&u(104)==(ray?65535u:7u)&&u(108)==0x283,"volume metadata");
     Require(f(16)==1&&f(24)==3&&f(32)==.1f&&f(40)==.3f,"volume coordinate snapshots");
     Require(f(48)==.5f&&f(56)==1.5f&&f(60)==2&&f(64)==1&&f(72)==3&&f(76)==2&&f(88)==1,"volume weighted result");
    }
    continue;
   }
   raySample=sample==6?0:sample==9?1:2;
#endif
   Require(id[0]==expectedCoordinate&&id[1]==expectedCoordinate&&id[3]==(raySample==2?2u:1u),"ray record identity");
   auto scalar=[&](UINT offset){float f=0;memcpy(&f,record+offset,4);return f;};
   auto integer=[&](UINT offset){UINT n=0;memcpy(&n,record+offset,4);return n;};
   if(integer(108)!=42||integer(148)!=2)std::cerr<<"raySample="<<raySample<<" material="<<integer(108)<<" count="<<integer(148)<<'\n';
   Require(integer(108)==42&&integer(148)==2,"material and ray count");
   Require(scalar(160)==1&&scalar(172)==4,"depth constants");
   if(raySample<2){
    Require(id[2]==raySample&&integer(144)==(raySample?65535u:7u),"hit/miss sentinel");
    Require(scalar(16)==1&&scalar(20)==3&&scalar(24)==8,"weighted contribution");
    Require(scalar(64)==float(raySample+1)&&scalar(72)==float(8*(raySample+1)),"native accumulation");
    Require(scalar(80)==1&&scalar(92)==4&&scalar(112)==5,"primary and secondary geometry");
    Require(scalar(136)==(raySample?0.f:1.f),"miss normal must be zero");
   }else{
    Require(id[2]==UINT_MAX,"summary sentinel");
    Require(scalar(176)==.75f&&scalar(180)==2.25f&&scalar(184)==6&&scalar(188)==1,"final GI");
    Require(scalar(192)==2&&scalar(200)==16&&scalar(208)==.75f,"sum and normalization");
   }
   if(raySample<2){for(UINT offset=224;offset<256;offset+=4)Require(integer(offset)==0,"unwritten record padding");}
   else{Require(integer(236)==0x278&&integer(252)==0,"accounting schema/padding");
    for(UINT offset : {224u,240u})Require(scalar(offset)==.375f&&scalar(offset+4)==1.125f&&scalar(offset+8)==3.f,"runtime surface/fallback accounting");}
#else
   if(!(id[0]==16&&id[1]==16&&id[2]==sample/2&&id[3]==100+sample))std::cerr<<"run="<<run<<" sample="<<sample<<" index="<<index<<" id="<<id[0]<<","<<id[1]<<","<<id[2]<<","<<id[3]<<" valid="<<valid<<"\n";
   Require(id[0]==16&&id[1]==16&&id[2]==sample/2&&id[3]==100+sample,"pixel/ray/light identity");
   Require(valid==(sample==0?3u:sample==1?16255u:16383u),"valid-stage mask");
   auto scalar=[&](UINT offset){float f=0;memcpy(&f,record+offset,4);return f;};
   Require(scalar(16)==.125f&&scalar(20)==.25f&&scalar(24)==.5f,"coefficients");
   if(sample){Require(scalar(48)==3.75f&&scalar(52)==4.75f&&scalar(56)==5.75f&&scalar(60)==6.75f,"attenuation-stage scalar stores");Require(scalar(96)==.4f&&scalar(112)==2&&scalar(128)==.5f&&scalar(144)==.9f,"MAD operands/material factors");}
   else Require(scalar(64)==0&&scalar(96)==0,"shadow early-out");
#endif
  }
#ifdef CONTROL_TEST_GI_JOINT_TRACE
  if(!run){std::ofstream capture("build/gi-joint-oracle-readback.bin",std::ios::binary);capture.write(reinterpret_cast<const char*>(p),16+expectedRecords*stride);Require(bool(capture),"oracle export");}
#endif
  readback->Unmap(0,&none);
 }
 CloseHandle(event);ComPtr<ID3D12InfoQueue> messages;Check(d.As(&messages));
 for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T size=0;messages->GetMessage(i,nullptr,&size);std::vector<unsigned char> storage(size);auto* m=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Check(messages->GetMessage(i,m,&size));if(m->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(m->pDescription);}
 std::cout<<"PASS GPU trace: native trace PSO, exact inserted GPU blocks, stage masks, shadow early-out, cookie skip, multiple rays/lights, overflow guard; D3D12 debug clean\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
