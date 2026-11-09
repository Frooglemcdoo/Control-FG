#include "gpu_test_device.h"
#include <iomanip>
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
            float v=value+float(c)*.01f+(checker?(((x+y)%2)?value*.5f:-value*.5f):0.f);
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
static ComPtr<ID3DBlob> Compile(const char* mode) {
    D3D_SHADER_MACRO macros[]={{"MODE",mode},{nullptr,nullptr}};
    ComPtr<ID3DBlob> shader,error;
    auto hr=D3DCompileFromFile(L"src/owned_volume.hlsl",macros,D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error);
    if(error)std::cerr<<static_cast<const char*>(error->GetBufferPointer());
    Check(hr);return shader;
}
static void Header(std::ofstream& out,const char* name,const void* ptr,size_t bytes) {
    const auto* data=static_cast<const unsigned char*>(ptr);
    out<<"inline constexpr unsigned char "<<name<<"[] = {\n";
    for(size_t i=0;i<bytes;++i){out<<unsigned(data[i])<<',';if(i%24==23)out<<'\n';}
    out<<"\n};\n";
}
int main(){try{
    const auto original=Read("build/native-media.dxbc");
    auto reference=Compile("0"),owned=Compile("1"),neutral=Compile("2");
    Device gpu(false);
    D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
    D3D12_ROOT_PARAMETER params[5]{};
    for(UINT i=0;i<3;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;}
    for(UINT i=0;i<2;++i){params[i+3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i+3].DescriptorTable={1,&ranges[i]};}
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderRegister=6;sampler.RegisterSpace=1;sampler.MaxLOD=D3D12_FLOAT32_MAX;
    sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxAnisotropy=1;
    D3D12_ROOT_SIGNATURE_DESC rs{5,params,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rs,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    ComPtr<ID3D12PipelineState> pipelines[4];
    D3D12_SHADER_BYTECODE shaders[]={{original.data(),original.size()},
        {reference->GetBufferPointer(),reference->GetBufferSize()},
        {owned->GetBufferPointer(),owned->GetBufferSize()},
        {neutral->GetBufferPointer(),neutral->GetBufferSize()}};
    for(UINT i=0;i<4;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();desc.CS=shaders[i];Check(gpu.device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&pipelines[i])));}
    struct Result{ComPtr<ID3D12Resource> read;D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;};
    std::vector<Result> results;
    // Three scenes x four PSOs. Flat, checker density+lighting, zero fog.
    for(UINT scene=0;scene<3;++scene)for(UINT mode=0;mode<4;++mode){
        const UINT first=4*(scene*4+mode);
        Texture(gpu,first,false,scene==1,2.f);
        Texture(gpu,first+1,false,false,0);
        // Zero fog case must be exactly zero in every channel.
        auto den=Texture(gpu,first+2,false,scene==1,scene==2?0.f:.1f);
        if(scene==2){ // Upload zero density using the existing footprint.
            std::vector<unsigned char> zeros(static_cast<size_t>(den.bytes));auto upload=gpu.Upload(zeros);
            gpu.Transition(den.resource.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=den.layout;
            to.pResource=den.resource.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
            gpu.Transition(den.resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        auto output=Texture(gpu,first+3,true,false,0);
        std::array<float,224> sys{};sys[22*4]=1;sys[23*4+1]=1;sys[24*4+2]=1;sys[25*4+3]=1;sys[54*4+1]=.25f;
        std::array<float,112> vol{};const UINT dimensions[]={width,height,depth};std::memcpy(vol.data()+18*4,dimensions,sizeof(dimensions));
        vol[21*4]=1.f/width;vol[21*4+1]=1.f/height;vol[21*4+2]=1.f/depth;
        vol[23*4+2]=.1f;vol[23*4+3]=1.f/(depth-1);
        vol[24*4]=.1f;vol[24*4+1]=1;vol[24*4+2]=10;
        std::array<float,48> atm{};atm[9*4+3]=4;atm[10*4]=3;atm[10*4+1]=.5f;
        atm[11*4]=1;atm[11*4+1]=.8f;atm[11*4+2]=.6f;
        if(scene==1){atm[4*4]=.02f;atm[7*4+1]=.3f;atm[7*4+2]=.2f;atm[7*4+3]=.1f;}
        const void* inputs[]={sys.data(),vol.data(),atm.data()};const UINT sizes[]={sizeof(sys),sizeof(vol),sizeof(atm)};
        gpu.list->SetComputeRootSignature(root.Get());
        for(UINT b=0;b<3;++b){auto cb=gpu.Buffer(sizes[b],D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,inputs[b]);gpu.list->SetComputeRootConstantBufferView(b,cb->GetGPUVirtualAddress());}
        auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();srv.ptr+=UINT64(first)*gpu.descriptorSize;auto uav=srv;uav.ptr+=3*gpu.descriptorSize;
        gpu.list->SetComputeRootDescriptorTable(3,srv);gpu.list->SetComputeRootDescriptorTable(4,uav);
        gpu.list->SetPipelineState(pipelines[mode].Get());gpu.list->Dispatch(1,1,1);
        gpu.Transition(output.resource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        auto read=gpu.Buffer(output.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=output.resource.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        to.pResource=read.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=output.layout;gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        results.push_back({read,output.layout});
    }
    gpu.Submit();
    std::vector<std::vector<float>> values;
    for(auto& result:results){void* ptr=nullptr;Check(result.read->Map(0,nullptr,&ptr));std::vector<float> data;
        for(UINT z=0;z<depth;++z)for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x)for(UINT c=0;c<4;++c){
            float value;auto offset=result.layout.Offset+(z*height+y)*result.layout.Footprint.RowPitch+(x*4+c)*sizeof(float);
            std::memcpy(&value,static_cast<unsigned char*>(ptr)+offset,sizeof(value));Require(std::isfinite(value),"Nonfinite media output");data.push_back(value);
        }result.read->Unmap(0,nullptr);values.push_back(data);
    }
    for(UINT scene=0;scene<3;++scene){
        float errorMax=0;
        for(size_t i=0;i<values[0].size();++i){
            errorMax=std::max(errorMax,std::abs(values[scene*4][i]-values[scene*4+1][i]));
            Require(std::abs(values[scene*4+3][i]-(i%4==3?1.f:0.f))<.00001f,"Fog neutral output mismatch");
            if(scene!=1)Require(std::abs(values[scene*4][i]-values[scene*4+2][i])<.0001f,"Owned flat/vacuum mismatch");
            if(i%4==3)Require(values[scene*4+2][i]>=0 && values[scene*4+2][i]<=1.00001f,"Invalid transmittance");
        }
        std::cout<<"Native/reference scene "<<scene<<" max error "<<errorMax<<'\n';Require(errorMax<.0001f,"Recovered native integration mismatch");
    }
    auto variation=[&](UINT result){double sum=0;for(UINT z=0;z<depth;++z)for(UINT y=1;y<height-1;++y)for(UINT x=1;x<width-2;++x){auto i=((z*height+y)*width+x)*4;sum+=std::abs(values[result][i]-values[result][i+4]);}return sum;};
    const auto oldNoise=variation(4),newNoise=variation(6);
    std::cout<<"Checker variation native="<<oldNoise<<" owned="<<newNoise<<'\n';Require(newNoise<oldNoise*.5,"Owned sampling did not attenuate checker variation");
    std::ofstream out("src/owned_volume.generated.h");Require(bool(out),"Cannot create shader header");out<<"#pragma once\n// Generated by test-owned-volume-gpu.cpp after WARP validation.\n";
    Header(out,"kVolumeNativeShader",original.data(),original.size());Header(out,"kVolumeOwnedShader",owned->GetBufferPointer(),owned->GetBufferSize());Header(out,"kVolumeNeutralShader",neutral->GetBufferPointer(),neutral->GetBufferSize());out.close();Require(bool(out),"Cannot finish shader header");
    std::cout<<"PASS native/reference parity, owned constant/vacuum preservation, finite colored transmission, density+light checker suppression, fog isolation and four original-layout PSOs\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
