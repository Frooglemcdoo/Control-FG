#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include <limits>
#include "../src/gi_reuse/output_resolve.h"
using namespace gi_reuse;
struct Batch {uint first,count,frame;float scale;};
struct Controls {uint frame,scene,width,height,first,pixels,rays;float range;};
int main()try {
    ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
    Device gpu(false);ComPtr<ID3D12InfoQueue> messages;Check(gpu.device.As(&messages));
    constexpr UINT width=8,height=2,pixels=10,rays=2,first=3;
    std::vector<GIInitialResult> results(pixels);
    std::vector<GIShadedProducerSample> samples(pixels*rays);
    std::vector<Batch> batches(pixels);
    for(UINT i=0;i<pixels;++i){
        results[i].surfaceContribution={1,2,3};results[i].fallbackContribution={.1f,.2f,.3f};
        results[i].valid=1;results[i].isSurface=1;results[i].frameTag=19;
        batches[i]={i*rays,rays,19,2};
        for(UINT j=0;j<rays;++j){auto& s=samples[i*rays+j];s.hitDistance=j?10.f:2.f;
            s.random=.5f;s.geometryVerified=1;s.geometricNormal={0,0,1};
            s.shading.secondaryPosition={0,0,1};s.shading.sourcePdf=1;s.shading.material=j?65535:0;
            s.shading.secondaryRadiance={1,1,1};s.shading.primaryBrdfCos={1,1,1};
            s.shading.rayIndex=j;s.shading.frameTag=19;s.shading.verified=7;
            s.shading.configuredCount=rays;s.shading.depthScale=2;
        }
    }
    results[1].valid=0;samples[2*rays].shading.frameTag=18;
    samples[3*rays+1].shading.rayIndex=0;samples[4*rays].hitDistance=-1;
    results[5].surfaceContribution.x=std::numeric_limits<float>::quiet_NaN();
    batches[6].first=UINT_MAX;samples[7*rays].reserved1=1;
    results[8].surfaceContribution={};results[8].fallbackContribution={};results[8].isSurface=0;
    results[9].frameTag=18;
    auto input=gpu.Upload(results),sampleBuffer=gpu.Upload(samples),batchBuffer=gpu.Upload(batches);
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width=width;td.Height=height;td.DepthOrArraySize=td.MipLevels=1;
    td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
    gpu.device->GetCopyableFootprints(&td,0,1,0,&footprint,nullptr,nullptr,&bytes);
    std::vector<float> initial(static_cast<std::size_t>((bytes+3)/4),-7.f);auto initialBuffer=gpu.Upload(initial);
    ComPtr<ID3D12Resource> color,hit;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    auto make=[&](ComPtr<ID3D12Resource>& target,UINT slot){
        Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&target)));
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=initialBuffer.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=target.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu.Transition(target.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        D3D12_UNORDERED_ACCESS_VIEW_DESC view{};view.Format=td.Format;view.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slot)*gpu.descriptorSize;
        gpu.device->CreateUnorderedAccessView(target.Get(),nullptr,&view,cpu);
    };
    make(color,3);make(hit,4);
    D3D12_DESCRIPTOR_RANGE ranges[5]{};D3D12_ROOT_PARAMETER params[6]{};
    for(UINT i=0;i<5;++i){ranges[i].RangeType=i<3?D3D12_DESCRIPTOR_RANGE_TYPE_SRV:D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        ranges[i].NumDescriptors=1;ranges[i].BaseShaderRegister=i<3?27+i:i-3;
        params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,&ranges[i]};}
    params[5].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[5].Constants={5,0,8};
    D3D12_ROOT_SIGNATURE_DESC rd{6,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> rootBytes,error;
    Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&rootBytes,&error));ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read("gi-output-resolve.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pipeline;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline)));
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pipeline.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.View(input.Get(),0,sizeof(GIInitialResult),pixels,false));
    gpu.list->SetComputeRootDescriptorTable(1,gpu.View(sampleBuffer.Get(),1,sizeof(GIShadedProducerSample),pixels*rays,false));
    gpu.list->SetComputeRootDescriptorTable(2,gpu.View(batchBuffer.Get(),2,sizeof(Batch),pixels,false));
    for(UINT i=3;i<5;++i){auto h=gpu.views->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(i)*gpu.descriptorSize;gpu.list->SetComputeRootDescriptorTable(i,h);}
    Controls control{19,7,width,height,first,pixels,rays,10};
    gpu.list->SetComputeRoot32BitConstants(5,8,&control,0);gpu.list->Dispatch(2,2,1);
    gpu.Uav(color.Get());gpu.Uav(hit.Get());
    // A disarmed dispatch must leave the native/current outputs unchanged.
    control.scene=0;gpu.list->SetComputeRoot32BitConstants(5,8,&control,0);gpu.list->Dispatch(2,1,1);
    gpu.Uav(color.Get());gpu.Uav(hit.Get());
    control.scene=7;control.width=UINT_MAX;gpu.list->SetComputeRoot32BitConstants(5,8,&control,0);gpu.list->Dispatch(2,1,1);
    auto read=[&](ID3D12Resource* target){
        gpu.Transition(target,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        auto out=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=target;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=out.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
        gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);return out;
    };
    auto colorRead=read(color.Get()),hitRead=read(hit.Get());gpu.Submit();
    unsigned char *c=nullptr,*h=nullptr;Check(colorRead->Map(0,nullptr,reinterpret_cast<void**>(&c)));Check(hitRead->Map(0,nullptr,reinterpret_cast<void**>(&h)));
    for(UINT p=0;p<width*height;++p){
        const auto offset=(p/width)*footprint.Footprint.RowPitch+(p%width)*16;
        float actual[4],distance[4];std::memcpy(actual,c+offset,16);std::memcpy(distance,h+offset,16);
        const bool lit=p==first,black=p==first+8;
        for(UINT k=0;k<4;++k){const float expected=lit?(k==3?1.f:float(k+1)*1.1f):black?(k==3?1.f:0.f):-7.f;
            Require(std::abs(actual[k]-expected)<1e-5f,"Colour, validity alpha or untouched native pixel mismatch");
            Require(distance[k]==(lit||black?6.f:-7.f),"Hit distance must average all rays including misses");}
    }
    colorRead->Unmap(0,nullptr);hitRead->Unmap(0,nullptr);
    for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T n=0;Check(messages->GetMessage(i,nullptr,&n));std::vector<unsigned char> data(n);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(data.data());Check(messages->GetMessage(i,message,&n));
        Require(message->Severity>D3D12_MESSAGE_SEVERITY_ERROR,"D3D12 output resolve debug error");}
    std::cout<<"PASS WARP native GI outputs: colour plus fallback, validity alpha, all-ray hit-distance mean, tile offset, malformed/stale/duplicate rejection, disarm and untouched native output\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
