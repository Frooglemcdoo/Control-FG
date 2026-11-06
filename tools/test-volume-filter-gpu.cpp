#include "gpu_test_device.h"
#include "../src/volume_filter.generated.h"
#include <array>

static constexpr UINT width=8,height=8,depth=4;
struct Volume {
    ComPtr<ID3D12Resource> resource;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
    UINT64 bytes=0;
};
static Volume Texture(Device& gpu,UINT index,bool output,bool checker,float value) {
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    desc.Width=width;desc.Height=height;desc.DepthOrArraySize=depth;desc.MipLevels=1;
    desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.SampleDesc.Count=1;
    if(output)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    Volume volume;
    Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,
        output?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,IID_PPV_ARGS(&volume.resource)));
    gpu.device->GetCopyableFootprints(&desc,0,1,0,&volume.layout,nullptr,nullptr,&volume.bytes);
    auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr+=SIZE_T(index)*gpu.descriptorSize;
    if(output){
        D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
        view.Format=desc.Format;view.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE3D;view.Texture3D.WSize=depth;
        gpu.device->CreateUnorderedAccessView(volume.resource.Get(),nullptr,&view,cpu);
    }else{
        std::vector<unsigned char> data(static_cast<size_t>(volume.bytes));
        for(UINT z=0;z<depth;++z)for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x)for(UINT c=0;c<4;++c){
            // Distinct channel values catch accidental coefficient swizzles.
            float v=value+float(c)*.1f+(checker?(((x+y)%2)?1.f:-1.f):0.f);
            auto offset=volume.layout.Offset+(z*height+y)*volume.layout.Footprint.RowPitch+(x*4+c)*sizeof(float);
            std::memcpy(data.data()+offset,&v,sizeof(v));
        }
        auto upload=gpu.Upload(data);
        D3D12_TEXTURE_COPY_LOCATION from{},to{};
        from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=volume.layout;
        to.pResource=volume.resource.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu.Transition(volume.resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.Format=desc.Format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE3D;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture3D.MipLevels=1;
        gpu.device->CreateShaderResourceView(volume.resource.Get(),&view,cpu);
    }
    gpu.retained.push_back(volume.resource);
    return volume;
}
int main(){try{
    Device gpu(false);
    D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,0}};
    D3D12_ROOT_PARAMETER params[4]{};
    for(UINT i=0;i<2;++i){
        params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;
        params[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i+2].DescriptorTable={1,&ranges[i]};
    }
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderRegister=6;sampler.RegisterSpace=1;sampler.MaxLOD=D3D12_FLOAT32_MAX;
    sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxAnisotropy=1;
    D3D12_ROOT_SIGNATURE_DESC rs{4,params,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&rs,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    ComPtr<ID3D12PipelineState> native,filtered;
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root.Get();
    pipeline.CS={kVolumeNativeShader,sizeof(kVolumeNativeShader)};
    Check(gpu.device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&native)));
    pipeline.CS={kVolumeFilteredShader,sizeof(kVolumeFilteredShader)};
    Check(gpu.device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&filtered)));
    struct Case {bool filter,checker,outside,history;};
    const Case cases[]={{false,false,false,false},{true,false,false,false},{false,true,false,false},
                        {true,true,false,false},{true,false,false,true},{true,false,true,true}};
    struct Result {ComPtr<ID3D12Resource> buffer;D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;UINT test,coefficient;};
    std::vector<Result> results;
    for(UINT i=0;i<6;++i){
        const auto c=cases[i];const UINT first=6*i;
        Texture(gpu,first,false,c.checker,2.f);
        Texture(gpu,first+1,false,c.checker,2.f);
        Texture(gpu,first+2,false,false,4.f);
        Texture(gpu,first+3,false,false,4.f);
        std::array<float,128> cb0{};
        const UINT dimensions[]={width,height,depth};
        std::memcpy(cb0.data()+18*4,dimensions,sizeof(dimensions));
        cb0[21*4]=1.f/width;cb0[21*4+1]=1.f/height;cb0[21*4+2]=1.f/depth;
        cb0[23*4]=1;cb0[23*4+1]=1;cb0[23*4+2]=.1f;
        cb0[24*4]=.1f;cb0[24*4+1]=.1f;cb0[24*4+2]=2;
        std::array<float,64> cb1{};
        cb1[4*4]=c.outside?2.f:.5f;cb1[4*4+1]=.5f;cb1[4*4+3]=1;
        cb1[5*4]=c.history?.5f:0;cb1[5*4+1]=100;
        auto a=gpu.Buffer(sizeof(cb0),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,cb0.data());
        auto b=gpu.Buffer(sizeof(cb1),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,cb1.data());
        Volume outputs[]={Texture(gpu,first+4,true,false,0),Texture(gpu,first+5,true,false,0)};
        auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();srv.ptr+=UINT64(first)*gpu.descriptorSize;
        auto uav=srv;uav.ptr+=4*gpu.descriptorSize;
        gpu.list->SetComputeRootSignature(root.Get());
        gpu.list->SetComputeRootConstantBufferView(0,a->GetGPUVirtualAddress());
        gpu.list->SetComputeRootConstantBufferView(1,b->GetGPUVirtualAddress());
        gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,uav);
        gpu.list->SetPipelineState(c.filter?filtered.Get():native.Get());gpu.list->Dispatch(2,2,1);
        for(UINT j=0;j<2;++j){
            const auto& out=outputs[j];
            gpu.Transition(out.resource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
            auto read=gpu.Buffer(out.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION from{},to{};
            from.pResource=out.resource.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            to.pResource=read.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=out.layout;
            gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
            results.push_back({read,out.layout,i,j});
        }
    }
    gpu.Submit();
    for(const auto& result:results){
        void* data=nullptr;Check(result.buffer->Map(0,nullptr,&data));
        for(UINT y=1;y<height-1;++y)for(UINT x=1;x<width-1;++x)for(UINT c=0;c<4;++c){
            const auto offset=result.layout.Offset+(height+y)*result.layout.Footprint.RowPitch+(4*x+c)*sizeof(float);
            float actual;std::memcpy(&actual,static_cast<unsigned char*>(data)+offset,sizeof(actual));
            float expected=2.f+.1f*c;
            if(result.test==2)expected+=((x+y)%2)?1.f:-1.f;
            if(result.test==4)expected+=1;
            if(result.coefficient==0 && c==3)expected=0;
            Require(std::isfinite(actual)&&std::abs(actual-expected)<.0001f,"volume filter pixel mismatch");
        }
        result.buffer->Unmap(0,nullptr);
    }
    std::cout<<"PASS native and filtered volume PSOs, coefficient channels, flat lighting, checker attenuation, valid history and out-of-view rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
