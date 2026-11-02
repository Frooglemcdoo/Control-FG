#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <fstream>
#include <iterator>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include "../src/gi_reuse/d3d12_index_snapshot.h"
using Microsoft::WRL::ComPtr;
void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("HRESULT "+std::to_string(static_cast<unsigned long>(h)));}
void Require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,UINT64 size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=type;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=size;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 ComPtr<ID3D12Resource> out;Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&out)));return out;
}
int main(int argc,char** argv) try {
 Require(argc==2,"usage: test index-snapshot.dxil");
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
 D3D12_FEATURE_DATA_D3D12_OPTIONS4 features{};Check(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4,&features,sizeof(features)));
 Require(features.Native16BitShaderOpsSupported!=FALSE,"WARP native 16-bit operations unavailable; GPU proof is required, not skipped");
 std::ifstream file(argv[1],std::ios::binary);Require(file.good(),"shader");std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});
 gi_reuse::IndexSnapshotPipeline pipeline;Check(pipeline.Initialize(device.Get(),bytes.data(),bytes.size()));
 D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 // Different native root layout: descriptor table plus root UAV and constant.
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER params[3]{};
 params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={1,&range};params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={0,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{3,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,errors;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors));
 ComPtr<ID3D12RootSignature> nativeRoot;Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&nativeRoot)));
 const char shader[]="Buffer<uint> source:register(t0); RWByteAddressBuffer outbuf:register(u0); cbuffer C:register(b0){uint value;} [numthreads(1,1,1)] void main(){outbuf.Store(0,source[0]+value);}";
 ComPtr<ID3DBlob> code;Check(D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=nativeRoot.Get();pd.CS={code->GetBufferPointer(),code->GetBufferSize()};ComPtr<ID3D12PipelineState> native;Check(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&native)));
 auto nativeInput=Buffer(device.Get(),256,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);void* mapped=nullptr;D3D12_RANGE none{0,0};Check(nativeInput->Map(0,&none,&mapped));*static_cast<UINT*>(mapped)=123;nativeInput->Unmap(0,nullptr);
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ComPtr<ID3D12DescriptorHeap> heap;Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
 D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_UINT;srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=1;device->CreateShaderResourceView(nativeInput.Get(),&srv,heap->GetCPUDescriptorHandleForHeapStart());
 struct Case {UINT64 owner,offset,count;};
 const Case cases[]={{6,0,6},{6,4,2},{266,2,264},{262150,2,262148},{16*1024*1024+6,0,16*1024*1024+6}};
 for(const auto& c:cases){
  gi_reuse::IndexSnapshotPlan expected;Require(gi_reuse::IndexSnapshotPlan::Make(c.owner,c.offset,c.count,expected),"plan");const UINT64 outputBytes=UINT64(expected.copyBytes)+64;
  auto upload=Buffer(device.Get(),c.owner,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
  Check(upload->Map(0,&none,&mapped));for(UINT64 i=0;i<c.owner;++i)static_cast<unsigned char*>(mapped)[i]=static_cast<unsigned char>((i*17+39)%251);upload->Unmap(0,nullptr);
  auto guard=Buffer(device.Get(),outputBytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);Check(guard->Map(0,&none,&mapped));std::memset(mapped,0xcd,static_cast<std::size_t>(outputBytes));guard->Unmap(0,nullptr);
  auto source=Buffer(device.Get(),c.owner,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
  auto output=Buffer(device.Get(),outputBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  auto sentinel=Buffer(device.Get(),4,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  auto read=Buffer(device.Get(),outputBytes+4,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
  ComPtr<ID3D12GraphicsCommandList4> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
  auto transition=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);};
  list->CopyBufferRegion(source.Get(),0,upload.Get(),0,c.owner);list->CopyBufferRegion(output.Get(),0,guard.Get(),0,outputBytes);
  transition(source.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDEX_BUFFER);transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(nativeRoot.Get());list->SetPipelineState(native.Get());list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());list->SetComputeRootUnorderedAccessView(1,sentinel->GetGPUVirtualAddress());list->SetComputeRoot32BitConstant(2,456,0);
  gi_reuse::ComputeBindingTracker tracker;tracker.Reset(native.Get());tracker.Heaps(1,heaps);tracker.Root(nativeRoot.Get());tracker.Descriptor(0,gi_reuse::RootArgumentKind::Table,heap->GetGPUDescriptorHandleForHeapStart().ptr);tracker.Descriptor(1,gi_reuse::RootArgumentKind::UAV,sentinel->GetGPUVirtualAddress());const UINT value=456;tracker.Constants(2,0,1,&value);gi_reuse::ComputeBindingSnapshot saved;Require(tracker.Capture(saved),"bindings");
  gi_reuse::IndexSnapshotPlan actual;
  Require(!pipeline.RecordRestoring(list.Get(),source.Get(),c.offset,1,output.Get(),saved,actual),"odd-byte refusal");
  Require(!pipeline.RecordRestoring(list.Get(),source.Get(),c.owner,2,output.Get(),saved,actual),"overrun refusal");
  Require(pipeline.RecordRestoring(list.Get(),source.Get(),c.offset,c.count,output.Get(),saved,actual),"record exact input");list->Dispatch(1,1,1);
  transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(sentinel.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(read.Get(),0,output.Get(),0,outputBytes);list->CopyBufferRegion(read.Get(),outputBytes,sentinel.Get(),0,4);Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
  ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");Check(queue->Signal(fence.Get(),1));Check(fence->SetEventOnCompletion(1,event));auto wait=WaitForSingleObject(event,30000);CloseHandle(event);Require(wait==WAIT_OBJECT_0,"GPU timeout");
  D3D12_RANGE readRange{0,static_cast<SIZE_T>(outputBytes+4)};Check(read->Map(0,&readRange,&mapped));const auto* result=static_cast<unsigned char*>(mapped);
  for(UINT64 i=0;i<c.count;++i)Require(result[i]==static_cast<unsigned char>(((i+c.offset)*17+39)%251),"index bytes differ");
  for(UINT64 i=c.count;i<expected.copyBytes;++i)Require(result[i]==0,"odd index padding must be zero");
  for(UINT64 i=expected.copyBytes;i<outputBytes;++i)Require(result[i]==0xcd,"guard bytes overwritten");
  UINT restored=0;std::memcpy(&restored,result+outputBytes,4);Require(restored==579,"native descriptor table, root or PSO not restored");read->Unmap(0,&none);
 }
 ComPtr<ID3D12InfoQueue> messages;Check(device.As(&messages));for(UINT64 i=0;i<messages->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T n=0;Check(messages->GetMessage(i,nullptr,&n));std::vector<unsigned char> data(n);auto* m=reinterpret_cast<D3D12_MESSAGE*>(data.data());Check(messages->GetMessage(i,m,&n));Require(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR,m->pDescription);}
 std::puts("PASS exact uint16 GPU snapshot: 6-byte source, final 2-byte element, offset 2, 2-D dispatch, >16 MiB copy, padded output, untouched guards, native table/root/PSO restoration, debug layer clean");
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
