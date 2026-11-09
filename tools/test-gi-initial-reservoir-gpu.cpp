#include "gpu_test_device.h"
#include "../src/gi_reuse/initial_reservoir.h"
using namespace gi_reuse;
struct ProducerSample {GIInitialInput shading;GIPoint normal;uint geometry;float random;uint reserved[3];};
struct Batch {uint first,count,frame;float scale;};
static_assert(sizeof(ProducerSample)==112&&sizeof(Batch)==16);
int main()try{
    Device gpu(false);
    std::vector<ProducerSample> samples;std::vector<Batch> batches;
    std::vector<GIInitialReservoir> expected;
    constexpr uint quadrature=1024,count=quadrature+3;
    for(uint b=0;b<count;++b){
        batches.push_back({static_cast<uint>(samples.size()),4,19,2});
        auto r=GIBeginInitialReservoir(19,4,2);
        for(uint j=0;j<4;++j){
            ProducerSample v{};v.shading.secondaryPosition={0,0,1};v.shading.sourcePdf=j==1?.75f:.25f;
            v.shading.secondaryRadiance=j==0?GIPoint{2,0,0}:j==1?GIPoint{0,3,0}:GIPoint{};
            const float response=j==1?.25f:.5f;v.shading.primaryBrdfCos={response,response,response};
            v.shading.material=j==3?65535:0;v.shading.fallbackContribution=j==3?GIPoint{.1f,.2f,.3f}:GIPoint{};
            v.shading.rayIndex=j;v.shading.frameTag=19;v.shading.verified=7;v.shading.configuredCount=4;v.shading.depthScale=2;
            v.normal={0,0,-1};v.geometry=1;v.random=j==1?(float(b%quadrature)+.5f)/float(quadrature):.5f;
            if(b==quadrature&&j==1)v.shading.rayIndex=0;
            if(b==quadrature+1&&j==0)v.shading.frameTag=18;
            if(b==quadrature+2&&j==0)v.geometry=0;
            samples.push_back(v);GIStreamInitialReservoir(r,v.shading,v.normal,v.geometry,v.random);
        }
        expected.push_back(r);
    }
    auto in=gpu.Upload(samples),batch=gpu.Upload(batches);
    auto reservoirs=gpu.Buffer(count*sizeof(GIInitialReservoir),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    auto output=gpu.Buffer((count+1)*sizeof(GIInitialResult),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
    std::vector<unsigned char> sentinel((count+1)*sizeof(GIInitialResult),0xcd);auto init=gpu.Upload(sentinel);
    gpu.list->CopyResource(output.Get(),init.Get());gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    D3D12_DESCRIPTOR_RANGE ranges[4]{};D3D12_ROOT_PARAMETER params[4]{};
    for(UINT i=0;i<4;++i){ranges[i].RangeType=i<2?D3D12_DESCRIPTOR_RANGE_TYPE_SRV:D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        ranges[i].NumDescriptors=1;ranges[i].BaseShaderRegister=i%2;
        params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,&ranges[i]};}
    D3D12_ROOT_SIGNATURE_DESC rd{4,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> bytes,error;
    Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read("gi-initial-reservoir.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC cp{};cp.pRootSignature=root.Get();cp.CS={shader.data(),shader.size()};ComPtr<ID3D12PipelineState> pipeline;
    Check(gpu.device->CreateComputePipelineState(&cp,IID_PPV_ARGS(&pipeline)));
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pipeline.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.View(in.Get(),0,sizeof(ProducerSample),static_cast<UINT>(samples.size()),false));
    gpu.list->SetComputeRootDescriptorTable(1,gpu.View(batch.Get(),1,sizeof(Batch),count,false));
    gpu.list->SetComputeRootDescriptorTable(2,gpu.View(reservoirs.Get(),2,sizeof(GIInitialReservoir),count,true));
    gpu.list->SetComputeRootDescriptorTable(3,gpu.View(output.Get(),3,sizeof(GIInitialResult),count,true));gpu.list->Dispatch((count+63)/64+1,1,1);
    gpu.Transition(reservoirs.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto rr=gpu.Buffer(count*sizeof(GIInitialReservoir),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    auto ro=gpu.Buffer((count+1)*sizeof(GIInitialResult),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->CopyResource(rr.Get(),reservoirs.Get());gpu.list->CopyResource(ro.Get(),output.Get());gpu.Submit();
    GIInitialReservoir* r=nullptr;GIInitialResult* out=nullptr;Check(rr->Map(0,nullptr,reinterpret_cast<void**>(&r)));Check(ro->Map(0,nullptr,reinterpret_cast<void**>(&out)));
    double mean[3]{};
    for(uint b=0;b<count;++b){
        const auto cpu=GIResolveInitialReservoir(expected[b]);Require(out[b].valid==cpu.valid,"GPU admitted/rejected wrong batch");
        if(!cpu.valid)continue;
        Require(r[b].selected.rayIndex==expected[b].selected.rayIndex&&r[b].processed==4,"Reservoir selection/coverage ABI mismatch");
        const GIPoint a=out[b].surfaceContribution,c=cpu.surfaceContribution;
        Require(std::abs(a.x-c.x)<1e-5f&&std::abs(a.y-c.y)<1e-5f&&std::abs(a.z-c.z)<1e-5f,"GPU RIS reconstruction mismatch");
        mean[0]+=a.x+out[b].fallbackContribution.x;mean[1]+=a.y+out[b].fallbackContribution.y;mean[2]+=a.z+out[b].fallbackContribution.z;
    }
    Require(std::abs(mean[0]/quadrature-2.05)<.004&&std::abs(mean[1]/quadrature-.6)<.004&&std::abs(mean[2]/quadrature-.15)<.004,"GPU RIS expected energy changed");
    Require(!std::memcmp(out+count,sentinel.data()+count*sizeof(GIInitialResult),sizeof(GIInitialResult)),"Output guard overwritten");
    rr->Unmap(0,nullptr);ro->Unmap(0,nullptr);
    std::cout<<"PASS WARP initial reservoir: 1024 colored unequal-PDF batches, actual selection/reconstruction, fallback energy, malformed batches and output guard\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
