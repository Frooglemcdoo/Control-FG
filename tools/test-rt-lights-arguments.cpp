#include "gpu_test_textures.h"
#include "../src/rt_lights/indirect_layout.h"
#include <cstddef>
using rt_lights::IndirectLayout;
static_assert(sizeof(D3D12_DISPATCH_RAYS_DESC)==104,"Unexpected ray dispatch ABI");
static_assert(offsetof(D3D12_DISPATCH_RAYS_DESC,Width)==IndirectLayout::RayDimensions,"Unexpected ray dimensions offset");
static void Run(UINT count,UINT width=16) {
    Device gpu(false);
    constexpr UINT height=16,capacity=200;
    auto counter=gpu.Upload(std::vector<UINT>{count});gpu.View(counter.Get(),0,4,1,false);
    std::vector<Job> stale(width*height,Job{123,456,789,1});
    auto work=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_UINT,stale.data(),true);
    TextureView(gpu,work.Get(),1,true);
    std::vector<UINT> initial(IndirectLayout::Bytes/4,0xa5a5a5a5);
    auto initialBuffer=gpu.Upload(initial);
    auto args=gpu.Buffer(IndirectLayout::Bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
    gpu.list->CopyResource(args.Get(),initialBuffer.Get());
    gpu.Transition(args.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    D3D12_UNORDERED_ACCESS_VIEW_DESC view{};view.Format=DXGI_FORMAT_R32_TYPELESS;view.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
    view.Buffer.NumElements=IndirectLayout::Bytes/4;view.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
    auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=2ULL*gpu.descriptorSize;
    gpu.device->CreateUnorderedAccessView(args.Get(),nullptr,&view,cpu);
    D3D12_DESCRIPTOR_RANGE bindings[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,1}};
    D3D12_ROOT_PARAMETER parameters[2]{};
    parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={2,bindings};
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[1].Constants={0,0,4};
    D3D12_ROOT_SIGNATURE_DESC desc{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read("rt_lights_native_arguments.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    const UINT limits[]={width,height,capacity,0};gpu.list->SetComputeRoot32BitConstants(1,4,limits,0);
    gpu.list->Dispatch(1,1,1);
    auto copied=gpu.Buffer(IndirectLayout::Bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.Transition(args.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    gpu.list->CopyResource(copied.Get(),args.Get());auto copiedWork=CopyOutput(gpu,work.Get());gpu.Submit();
    void* raw=nullptr;Check(copied->Map(0,nullptr,&raw));const auto* actual=static_cast<UINT*>(raw);
    const bool valid=width%8==0;const UINT expected=valid && count<=capacity && count<=width*height?count:0;
    const UINT rows=expected==0?0:(expected-1)/width+1,groups=(rows+7)/8;
    for(UINT i=0;i<22;++i)Require(actual[i]==initial[i],"Native shader-table address or size was modified");
    Require(actual[22]==expected && actual[23]==1 && actual[24]==1 && actual[25]==0,"Incorrect ray dispatch dimensions");
    Require(actual[26]==initial[26] && actual[27]==initial[27],"Argument padding overwritten");
    Require(actual[28]==(valid?width/8:0) && actual[29]==groups && actual[30]==1,"Incorrect extraction dispatch dimensions");
    Require(actual[31]==(expected+63)/64 && actual[32]==1 && actual[33]==1,"Incorrect pack dispatch dimensions");
    void* workData=nullptr;Check(copiedWork.buffer->Map(0,nullptr,&workData));
    for(UINT i=0;i<width*height;++i) {
        const auto* row=reinterpret_cast<const Job*>(static_cast<const unsigned char*>(workData)+(i/width)*copiedWork.rowPitch);
        const Job expectedJob=i>=expected && i<groups*8*width?Job{}:stale[i];
        Require(row[i%width]==expectedJob,"Tail clearing changed active jobs or retained stale jobs");
    }
    copied->Unmap(0,nullptr);copiedWork.buffer->Unmap(0,nullptr);
    std::cout<<"PASS: GPU indirect arguments count="<<count<<" width="<<width<<'\n';
}
int main() try {
    for(UINT count:{0u,1u,63u,64u,65u,128u,129u,200u,201u,0xffffffffu})Run(count);
    Run(1,15);return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
