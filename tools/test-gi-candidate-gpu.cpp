#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cmath>
#include <cstring>
#include "../build/gi_candidate_fixture.h"
using Microsoft::WRL::ComPtr;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(h)));}
void Require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
float Float(UINT bits){float f;memcpy(&f,&bits,4);return f;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=type;hp.CreationNodeMask=hp.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 ComPtr<ID3D12Resource> r;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&r)));return r;
}
void Transition(ID3D12GraphicsCommandList* l,ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){D3D12_RESOURCE_BARRIER x{};x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;x.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};l->ResourceBarrier(1,&x);}
int main()try{
 constexpr UINT stride=520,outputWords=12;
 constexpr UINT validCount=static_cast<UINT>(sizeof(fixtureWords)/sizeof(UINT)/stride);
 std::vector<UINT> input(std::begin(fixtureWords),std::end(fixtureWords));
 // Invalid count, sentinel, ray order, NaN, negative and overflow cases must fail closed.
 for(UINT k=0;k<7;++k){std::vector<UINT> bad(fixtureWords,fixtureWords+stride);
  if(k==0)bad[0]=0;if(k==1)bad[0]=65;if(k==2)bad[7]=65534;
  if(k==3)bad[11]=63;if(k==4)bad[4]=0x7fc00000;if(k==5)bad[4]=0xbf800000;
  if(k==6){bad[4]=0x7f7fffff;bad[8]=0x7f7fffff;}
  input.insert(input.end(),bad.begin(),bad.end());}
 const UINT count=static_cast<UINT>(input.size()/stride),totalBytes=(count*outputWords+64)*4;
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> warp;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
 ComPtr<ID3D12Device> d;Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d)));
 D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
 params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={0,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{3,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,errors;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors));
 ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 ComPtr<ID3DBlob> shader;HRESULT compiled=D3DCompileFromFile(L"src/shaders/gi_candidate_no_reuse.hlsl",nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_WARNINGS_ARE_ERRORS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&shader,&errors);
 if(errors)std::cerr.write(static_cast<const char*>(errors->GetBufferPointer()),static_cast<std::streamsize>(errors->GetBufferSize()));Check(compiled);
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};ComPtr<ID3D12PipelineState> pso;Check(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 auto in=Buffer(d.Get(),input.size()*4,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE);
 auto init=Buffer(d.Get(),totalBytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE);
 auto out=Buffer(d.Get(),totalBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 auto readback=Buffer(d.Get(),totalBytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE);
 D3D12_RANGE none{0,0};void* mapped=nullptr;Check(in->Map(0,&none,&mapped));memcpy(mapped,input.data(),input.size()*4);in->Unmap(0,nullptr);
 Check(init->Map(0,&none,&mapped));memset(mapped,0xa5,totalBytes);init->Unmap(0,nullptr);
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(d->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(d->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso.Get(),IID_PPV_ARGS(&list)));
 list->CopyBufferRegion(out.Get(),0,init.Get(),0,totalBytes);Transition(list.Get(),out.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 list->SetComputeRootSignature(root.Get());list->SetComputeRootShaderResourceView(0,in->GetGPUVirtualAddress());list->SetComputeRootUnorderedAccessView(1,out->GetGPUVirtualAddress());list->SetComputeRoot32BitConstant(2,count,0);list->Dispatch((count+63)/64,1,1);
 Transition(list.Get(),out.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),0,out.Get(),0,totalBytes);Check(list->Close());
 ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);ComPtr<ID3D12Fence> fence;Check(d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Check(queue->Signal(fence.Get(),1));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");Check(fence->SetEventOnCompletion(1,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"GPU timeout");CloseHandle(event);
 D3D12_RANGE range{0,totalBytes};Check(readback->Map(0,&range,&mapped));auto* result=static_cast<UINT*>(mapped);double maxError=0;
 for(UINT pixel=0;pixel<count;++pixel){auto* value=result+pixel*outputWords;auto* source=input.data()+pixel*stride;
  if(pixel>=validCount){for(UINT i=0;i<outputWords;++i)Require(value[i]==0,"invalid input produced output");continue;}
  Require(value[3]==1,"valid input rejected");UINT hits=0,misses=0;double expectedHit[3]{},expectedMiss[3]{};
  for(UINT ray=0;ray<source[0];++ray){auto* r=source+4+ray*8;bool miss=r[3]==65535;if(miss)++misses;else ++hits;
   for(UINT c=0;c<3;++c)(miss?expectedMiss:expectedHit)[c]+=double(Float(r[c]))*Float(r[4+c])*Float(source[1])/source[0];}
  Require(value[7]==hits&&value[11]==misses,"hit/miss counts");
  for(UINT c=0;c<3;++c){double error=std::abs(double(Float(value[c]))-Float(source[516+c]));maxError=std::max(error,maxError);Require(error<1e-6,"native final RGB mismatch");
   Require(std::abs(Float(value[4+c])-expectedHit[c])<1e-6,"surface term mismatch");Require(std::abs(Float(value[8+c])-expectedMiss[c])<1e-6,"miss fallback mismatch");}}
 for(UINT i=count*outputWords;i<totalBytes/4;++i)Require(result[i]==0xa5a5a5a5,"dispatch touched guard");readback->Unmap(0,&none);
 ComPtr<ID3D12InfoQueue> messages;Check(d.As(&messages));
 for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T size=0;messages->GetMessage(i,nullptr,&size);std::vector<unsigned char> storage(size);auto* m=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Check(messages->GetMessage(i,m,&size));if(m->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(m->pDescription);}
 std::cout<<"PASS D3D12 no-reuse GI: "<<validCount<<" valid fixtures, 7 invalid cases, dispatch guard, native RGB max error="<<maxError<<", debug clean. WARP validation only; no game/performance claim.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
