// WARP compute test of the generated phase arithmetic, not a DXR scene test.
#include "gpu_test_device.h"
#include <array>
#include <cstdint>
#include <random>
using Job=std::array<UINT,4>;
using Result=std::array<float,4>;
static UINT Bits(float x){UINT u;std::memcpy(&u,&x,4);return u;}
int main(){try{
    std::vector<Job> jobs;
    const UINT frames[]={0,1,50093,56810,1000000,0xFFFFFFFFu};
    const UINT counts[]={1,2,4,8,12,16};
    for(UINT frame:frames)for(UINT count:counts)for(UINT sample=0;sample<count;++sample)
        for(UINT j=0;j<256;++j)
            jobs.push_back({frame*count+sample,Bits(j/256.0f),Bits((255-j)/256.0f),Bits(j?1.0f-1.0f/16777216.0f:0.0f)});
    std::mt19937 rng(185);
    for(UINT i=0;i<65536;++i)jobs.push_back({rng(),Bits((rng()&0xFFFFFFu)/16777216.0f),Bits((rng()&0xFFFFFFu)/16777216.0f),Bits((rng()&0xFFFFFFu)/16777216.0f)});
    Device gpu(false);
    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[2].Constants.Num32BitValues=1;
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=3;desc.pParameters=params;
    ComPtr<ID3DBlob> signature,error;
    Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&signature,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,signature->GetBufferPointer(),signature->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read("build/spec-phase-fixture.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    auto input=gpu.Upload(jobs);
    const UINT64 bytes=jobs.size()*sizeof(Result);
    auto output=gpu.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    auto readback=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->SetPipelineState(pso.Get());gpu.list->SetComputeRootSignature(root.Get());
    gpu.list->SetComputeRootShaderResourceView(0,input->GetGPUVirtualAddress());
    gpu.list->SetComputeRootUnorderedAccessView(1,output->GetGPUVirtualAddress());
    gpu.list->SetComputeRoot32BitConstant(2,static_cast<UINT>(jobs.size()),0);
    gpu.list->Dispatch(static_cast<UINT>((jobs.size()+63)/64),1,1);
    gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    gpu.list->CopyBufferRegion(readback.Get(),0,output.Get(),0,bytes);gpu.Submit();
    void* data=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(bytes)};Check(readback->Map(0,&range,&data));
    const auto* results=static_cast<const Result*>(data);
    // Independent constants encoded as IEEE double source values. Compute
    // exact fractional products with 64-bit integers, then float addition.
    const double constants[]={0.75487768650054931640625,0.56984031200408935546875,1.61803400516510009765625};
    for(size_t i=0;i<jobs.size();++i){
        for(UINT d=0;d<3;++d){
            const auto numerator=static_cast<uint64_t>(constants[d]*16777216.0);
            const double exact=double((uint64_t(jobs[i][0])*numerator)%16777216ull)/16777216.0;
            float shift;std::memcpy(&shift,&jobs[i][d+1],4);
            const float sum=static_cast<float>(exact)+shift;
            const float expected=sum-std::floor(sum);
            Require(results[i][d]==expected,"GPU phase differs from 64-bit CPU oracle");
        }
        Require(results[i][3]==1.0f,"Fixture output unwritten");
    }
    readback->Unmap(0,nullptr);
    std::cout<<"PASS: "<<jobs.size()<<" WARP phase cases; capture-age/long-run/index-wrap/shift-boundary inputs\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
