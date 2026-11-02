#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <limits>
#include "../src/gi_reuse/initial_sample.h"
using Microsoft::WRL::ComPtr;using namespace gi_reuse;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(h)));}
void Require(bool b,const char* text){if(!b)throw std::runtime_error(text);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,UINT64 size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=type;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=size;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 ComPtr<ID3D12Resource> r;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&r)));return r;
}
int main()try{
 std::vector<GIInitialInput> data(67); // Nonmultiple dispatch exercises bounds.
 for(UINT i=0;i<data.size();++i){auto& v=data[i];v.secondaryPosition={1,2,3};v.sourcePdf=(float(i)+1)/100;
  v.secondaryRadiance={2,3,4};v.primaryBrdfCos={.5f*v.sourcePdf,v.sourcePdf,2*v.sourcePdf};v.material=7;v.frameTag=4;v.verified=7;v.configuredCount=6;v.depthScale=.75f;v.rayIndex=i%6;
  if(i%3==0){v.material=65535;v.fallbackContribution={1,2,3};}
 }
 data[1].verified=0;data[2].sourcePdf=0;data[4].frameTag=3;data[5].secondaryRadiance.x=std::numeric_limits<float>::quiet_NaN();data[7].material=65534;data[8].configuredCount=0;
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
 ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d)));
 // Structured buffers need descriptor views with their ABI strides.
 D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,1};
 D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={2,ranges};params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[1].Constants={0,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,errors;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors));
 ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 ComPtr<ID3DBlob> shader;auto hr=D3DCompileFromFile(L"src/shaders/gi_initial_sample.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",D3DCOMPILE_WARNINGS_ARE_ERRORS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&shader,&errors);
 if(errors)std::cerr.write(static_cast<const char*>(errors->GetBufferPointer()),static_cast<std::streamsize>(errors->GetBufferSize()));Check(hr);
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};ComPtr<ID3D12PipelineState> pso;Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 const UINT count=static_cast<UINT>(data.size()),bytes=(count+1)*sizeof(GIInitialResult);
 auto in=Buffer(d.Get(),data.size()*sizeof(GIInitialInput),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE);
 auto out=Buffer(d.Get(),bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 auto read=Buffer(d.Get(),bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE);
 void* mapped=nullptr;D3D12_RANGE none{0,0};Check(in->Map(0,&none,&mapped));memcpy(mapped,data.data(),data.size()*sizeof(GIInitialInput));in->Unmap(0,nullptr);
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.NumDescriptors=2;hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ComPtr<ID3D12DescriptorHeap> heap;Check(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
 auto handle=heap->GetCPUDescriptorHandleForHeapStart();D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=count;srv.Buffer.StructureByteStride=sizeof(GIInitialInput);d->CreateShaderResourceView(in.Get(),&srv,handle);
 handle.ptr+=d->GetDescriptorHandleIncrementSize(hd.Type);D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;uav.Buffer.NumElements=count+1;uav.Buffer.StructureByteStride=sizeof(GIInitialResult);d->CreateUnorderedAccessView(out.Get(),nullptr,&uav,handle);
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso.Get(),IID_PPV_ARGS(&list)));
 ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());list->SetComputeRoot32BitConstant(1,4,0);list->Dispatch(2,1,1);
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={out.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);list->CopyBufferRegion(read.Get(),0,out.Get(),0,bytes);Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
 ComPtr<ID3D12Fence> fence;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"GPU timeout");CloseHandle(event);
 D3D12_RANGE range{0,bytes};Check(read->Map(0,&range,&mapped));auto* results=static_cast<GIInitialResult*>(mapped);
 for(UINT i=0;i<=count;++i){auto expected=i<count?GIMakeInitialSample(data[i],4):GIInitialResult{};const auto& actual=results[i];Require(actual.valid==expected.valid&&actual.isSurface==expected.isSurface&&actual.frameTag==expected.frameTag&&actual.rayIndex==expected.rayIndex,"GPU metadata");
  const float a[]={actual.surfaceContribution.x,actual.surfaceContribution.y,actual.surfaceContribution.z,actual.fallbackContribution.x,actual.fallbackContribution.y,actual.fallbackContribution.z,actual.inverseSourcePdf,actual.targetDensity};
  const float b[]={expected.surfaceContribution.x,expected.surfaceContribution.y,expected.surfaceContribution.z,expected.fallbackContribution.x,expected.fallbackContribution.y,expected.fallbackContribution.z,expected.inverseSourcePdf,expected.targetDensity};
  for(UINT j=0;j<8;++j)Require(std::abs(a[j]-b[j])<1e-5f,"GPU numerical agreement");
 }
 read->Unmap(0,&none);ComPtr<ID3D12InfoQueue> messages;Check(d.As(&messages));for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T n=0;messages->GetMessage(i,nullptr,&n);std::vector<unsigned char> storage(n);auto* m=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Check(messages->GetMessage(i,m,&n));Require(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR,m->pDescription);}
 std::cout<<"PASS GPU initial GI adapter, CPU agreement, invalid inputs, dispatch bounds, debug layer clean\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
