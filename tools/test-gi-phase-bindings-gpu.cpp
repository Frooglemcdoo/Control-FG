#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_phase_binding_pair.h"
int main()try {
 using namespace gi_reuse;Device d(false);
 const char* shader="cbuffer Input:register(b0){uint value;};cbuffer Index:register(b1){uint index;};RWStructuredBuffer<uint> output:register(u0);[numthreads(1,1,1)]void main(){output[index]=value;}";
 ComPtr<ID3DBlob> code,error;Check(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&error));
 D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[0].Descriptor={0,0};
 params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor={0,0};
 params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants={1,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=params;
 ComPtr<ID3DBlob> blob;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code->GetBufferPointer(),code->GetBufferSize()};
 ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 std::vector<UINT> av(64),bv(64);av[0]=17;bv[0]=29;auto a=d.Upload(av),b=d.Upload(bv);
 auto output=d.Buffer(8,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto back=d.Buffer(8,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 ComputeBindingTracker tracker;tracker.Reset(pso.Get());tracker.Root(root.Get());ID3D12DescriptorHeap* heaps[]={d.views.Get()};tracker.Heaps(1,heaps);
 tracker.Descriptor(0,RootArgumentKind::CBV,a->GetGPUVirtualAddress());tracker.Descriptor(1,RootArgumentKind::UAV,output->GetGPUVirtualAddress());
 UINT index=0;tracker.Constants(2,0,1,&index);ComputeBindingSnapshot first,second;Require(tracker.Capture(first),"first snapshot");
 tracker.Descriptor(0,RootArgumentKind::CBV,b->GetGPUVirtualAddress());index=1;tracker.Constants(2,0,1,&index);Require(tracker.Capture(second),"second snapshot");
 D3D12PhaseBindingPair pair;Require(pair.Remember(0,7,100,first),"remember first");
 Require(!pair.Remember(1,7,101,second)&&!pair.Remember(1,8,100,second),"stale invocation admitted");
 Require(pair.Remember(1,7,100,second)&&pair.Seal(),"seal pair");Require(!pair.Remember(0,7,100,second),"sealed snapshot overwritten");
 // Mutate the caller's original copy; the pair must retain its earlier values.
 first.arguments.Descriptor(0,RootArgumentKind::CBV,b->GetGPUVirtualAddress());
 Require(!pair.Restore(0,7,101,d.list.Get(),second),"stale restore");
 auto wrongHeaps=second;wrongHeaps.heaps[0].Reset();Require(!pair.Restore(0,7,100,d.list.Get(),wrongHeaps),"heap mismatch restore");
 Require(pair.Restore(0,7,100,d.list.Get(),second),"restore first");d.list->Dispatch(1,1,1);d.Uav(output.Get());
 Require(pair.Restore(1,7,100,d.list.Get(),second),"restore second");d.list->Dispatch(1,1,1);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(back.Get(),0,output.Get(),0,8);d.Submit();
 UINT* values=nullptr;D3D12_RANGE range{0,8};Check(back->Map(0,&range,reinterpret_cast<void**>(&values)));
 Require(values[0]==17&&values[1]==29,"phase constants were mixed");D3D12_RANGE empty{0,0};back->Unmap(0,&empty);
 std::cout<<"PASS GPU phase bindings: independent CBVs/constants, immutable snapshot copies, epoch/heap rejection. Resource/SBT lifetime not proven.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
