#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_ray_constant_snapshot.h"
int main()try {
 using namespace gi_reuse;Device d(false);ComPtr<ID3DBlob> shader,error;
 Check(D3DCompileFromFile(L"src/shaders/gi_ray_constant_snapshot.hlsl",nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error));
 D3D12_ROOT_PARAMETER parameters[5]{};
 for(UINT i=0;i<2;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[i].Constants={i+2,0,1};
  parameters[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i+2].Descriptor={i,0};}
 parameters[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
 D3D12_ROOT_SIGNATURE_DESC desc{5,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> rootBytes;
 Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&rootBytes,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root)));
 const char* consumer="cbuffer A:register(b0){uint4 a[71];};cbuffer B:register(b1){uint4 b[2];};cbuffer I:register(b2){uint index;};RWStructuredBuffer<uint> outValue:register(u0);[numthreads(1,1,1)]void main(){outValue[index]=a[0].x;outValue[index+1]=a[70].y;outValue[index+2]=b[1].x;}";
 ComPtr<ID3DBlob> consumerBytes;Check(D3DCompile(consumer,strlen(consumer),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&consumerBytes,&error));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={consumerBytes->GetBufferPointer(),consumerBytes->GetBufferSize()};
 ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 std::vector<UINT> a(320),b(64);for(UINT i=0;i<a.size();++i)a[i]=i*71+13;for(UINT i=0;i<b.size();++i)b[i]=i*37+901;
 auto sourceA=d.Upload(a),sourceB=d.Upload(b);auto output=d.Buffer(24,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 ComputeBindingSnapshot native;native.root=root;native.pipeline=pso;native.arguments.Root(1);
 native.arguments.Descriptor(2,RootArgumentKind::CBV,sourceA->GetGPUVirtualAddress());native.arguments.Descriptor(3,RootArgumentKind::CBV,sourceB->GetGPUVirtualAddress());
 native.arguments.Descriptor(4,RootArgumentKind::UAV,output->GetGPUVirtualAddress());UINT index=0;native.arguments.Constants(0,0,1,&index);
 D3D12GIRayConstantSnapshot copy;Require(!copy.ApplyTo(native),"unrecorded constants exposed");
 // Root slots must actually be native b0/b1; pointer/layout equality alone is insufficient.
 parameters[3].Descriptor.ShaderRegister=7;ComPtr<ID3DBlob> wrong;
 Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&wrong,&error));
 Require(FAILED(copy.Initialize(d.device.Get(),root.Get(),wrong->GetBufferPointer(),wrong->GetBufferSize(),{shader->GetBufferPointer(),shader->GetBufferSize()})),"wrong CBV mapping accepted");
 D3D12GIRayConstantPipeline pipeline;Check(pipeline.Initialize(d.device.Get(),root.Get(),rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),{shader->GetBufferPointer(),shader->GetBufferSize()}));
 Check(copy.Initialize(d.device.Get(),pipeline));
 auto unaligned=native;unaligned.arguments.Descriptor(2,RootArgumentKind::CBV,sourceA->GetGPUVirtualAddress()+4);
 Require(!copy.RecordAndRestore(d.list.Get(),unaligned),"unaligned root CBV accepted");
 Require(copy.RecordAndRestore(d.list.Get(),native)&&!copy.RecordAndRestore(d.list.Get(),native),"one-shot copy violated");
 d.list->Dispatch(1,1,1); // Uses RESTORED native root, pipeline and arguments.
 d.Submit();
 void* mapped=nullptr;D3D12_RANGE empty{0,0};for(auto* resource:{sourceA.Get(),sourceB.Get()}){
  Check(resource->Map(0,&empty,&mapped));memset(mapped,0xee,static_cast<size_t>(resource->GetDesc().Width));resource->Unmap(0,nullptr);}
 Check(d.allocator->Reset());Check(d.list->Reset(d.allocator.Get(),nullptr));Check(d.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(d.fence.ReleaseAndGetAddressOf())));
 auto replay=native;Require(copy.ApplyTo(replay),"copied constants not applied");index=3;replay.arguments.Constants(0,0,1,&index);
 Require(replay.Restore(d.list.Get()),"replay restore");d.list->Dispatch(1,1,1);
 auto values=d.Buffer(24,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(values.Get(),0,output.Get(),0,24);
 auto bytes=d.Buffer(copy.Bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(copy.Resource(),D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(bytes.Get(),0,copy.Resource(),0,copy.Bytes);d.Submit();
 D3D12_RANGE valueRange{0,24};Check(values->Map(0,&valueRange,&mapped));const auto* actual=static_cast<UINT*>(mapped);
 for(UINT base:{0u,3u})Require(actual[base]==a[0]&&actual[base+1]==a[281]&&actual[base+2]==b[4],"native restore or immutable replay wrong");values->Unmap(0,&empty);
 D3D12_RANGE byteRange{0,copy.Bytes};Check(bytes->Map(0,&byteRange,&mapped));actual=static_cast<UINT*>(mapped);
 for(UINT i=0;i<384;++i){const UINT expected=i<282?a[i]:(i>=320&&i<325?b[i-320]:0);Require(actual[i]==expected,"snapshot bytes/padding changed");}bytes->Unmap(0,&empty);
 std::cout<<"PASS GPU ray constants: original binding restore, native overwrite, deferred CBV replay, full byte range and zero padding, ABI/alignment rejection. No scene lifetime claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
