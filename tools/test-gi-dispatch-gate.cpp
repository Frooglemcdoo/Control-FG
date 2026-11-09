#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/d3d12_dispatch_gate.h"
static void Run(bool admit){
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 Device gpu(false);ComPtr<ID3D12InfoQueue> info;Check(gpu.device.As(&info));
 {gi_reuse::D3D12DispatchGate temporary;Check(temporary.Initialize(gpu.device.Get()));}
 const auto beforeReuse=gi_reuse::D3D12DispatchGate::PoolStatistics().reuses;
 gi_reuse::D3D12DispatchGate gate;Check(gate.Initialize(gpu.device.Get()));
 Require(gi_reuse::D3D12DispatchGate::PoolStatistics().reuses==beforeReuse+2,"Retired gate buffers were not reused");
 {gi_reuse::D3D12DispatchGate concurrent;Check(concurrent.Initialize(gpu.device.Get()));
  Require(concurrent.Resource()!=gate.Resource(),"Live gate buffer was reused prematurely");}
 auto one=gpu.Upload(std::vector<unsigned>{0}),zero=gpu.Upload(std::vector<unsigned>{0});
 auto output=gpu.Buffer(4,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 auto readback=gpu.Buffer(4,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 gpu.list->CopyBufferRegion(output.Get(),0,zero.Get(),0,4);
 gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 gate.Arm(gpu.list.Get(),one.Get(),0);Check(gpu.list->Close());
 ComPtr<ID3DBlob> shader,error,serialized;
 Check(D3DCompileFromFile(L"src/shaders/gi_dispatch_gate_test.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error));
 D3D12_ROOT_PARAMETER parameters[2]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;parameters[0].Descriptor={35,0};
 parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[1].Descriptor={0,0};
 D3D12_ROOT_SIGNATURE_DESC rd{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&error));ComPtr<ID3D12RootSignature> root;
 Check(gpu.device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};ComPtr<ID3D12PipelineState> pipeline;
 Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline)));
 ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList4> pass;
 Check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 Check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pipeline.Get(),IID_PPV_ARGS(&pass)));
 pass->SetComputeRootSignature(root.Get());pass->SetComputeRootShaderResourceView(0,gate.Resource()->GetGPUVirtualAddress());
 pass->SetComputeRootUnorderedAccessView(1,output->GetGPUVirtualAddress());pass->Dispatch(1,1,1);gate.Close(pass.Get());Check(pass->Close());
 const auto complete=[&](UINT64 value){
  Check(gpu.queue->Signal(gpu.fence.Get(),value));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"Fence event failed");
  Check(gpu.fence->SetEventOnCompletion(value,event));const auto wait=WaitForSingleObject(event,15000);CloseHandle(event);Require(wait==WAIT_OBJECT_0,"GPU timeout");Check(gpu.device->GetDeviceRemovedReason());
 };
 // R319: prefix and consumer are genuinely submitted before controls are
 // decided. Only the private GPU wait makes that later upload write legal.
 ComPtr<ID3D12Fence> hold;Check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&hold)));
 Check(gpu.queue->Wait(hold.Get(),1));
 ID3D12CommandList* first[]={gpu.list.Get(),pass.Get()};gpu.queue->ExecuteCommandLists(2,first);
 Check(gpu.queue->Signal(gpu.fence.Get(),1));
 Require(gpu.fence->GetCompletedValue()==0,"Private hold failed to block GPU reads");
 if(admit){void* control=nullptr;D3D12_RANGE empty{0,0};Check(one->Map(0,&empty,&control));
  *static_cast<unsigned*>(control)=1;D3D12_RANGE written{0,4};one->Unmap(0,&written);}
 Check(hold->Signal(1));complete(1);
 // D3D12 requires prior execution of a command list to finish before replay.
 // Only this standalone fixture waits; the live path adds no such CPU wait.
 ID3D12CommandList* replay=pass.Get();gpu.queue->ExecuteCommandLists(1,&replay);complete(2);
 gpu.queue->ExecuteCommandLists(1,&replay);
 ComPtr<ID3D12CommandAllocator> readAllocator;ComPtr<ID3D12GraphicsCommandList> read;
 Check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&readAllocator)));
 Check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,readAllocator.Get(),nullptr,IID_PPV_ARGS(&read)));
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
 barrier.Transition={output.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE};read->ResourceBarrier(1,&barrier);
 read->CopyBufferRegion(readback.Get(),0,output.Get(),0,4);Check(read->Close());ID3D12CommandList* last=read.Get();gpu.queue->ExecuteCommandLists(1,&last);
 complete(3);
 void* mapped=nullptr;D3D12_RANGE range{0,4};Check(readback->Map(0,&range,&mapped));Require(*static_cast<unsigned*>(mapped)==(admit?1u:0u),"Deferred admission/cancellation or replay count incorrect");range={0,0};readback->Unmap(0,&range);
 for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size=0;Check(info->GetMessage(i,nullptr,&size));std::vector<unsigned char> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(info->GetMessage(i,message,&size));if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error(message->pDescription);}
 std::cout<<"PASS deferred GPU hold: "<<(admit?"admitted":"cancelled")<<", submitted-prefix upload protection, single-use and replay validation\n";
}
int main()try{Run(true);Run(false);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
