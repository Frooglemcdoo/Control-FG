#include "gpu_test_device.h"
#include <array>
#include "../src/rt_lights/layout.h"
using Pixel=std::array<float,4>;
using Job=std::array<UINT,4>;
static ComPtr<ID3D12Resource> Texture(Device& gpu,UINT width,UINT height,DXGI_FORMAT format,const void* data,bool output=false) {
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=width;desc.Height=height;desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=format;desc.SampleDesc.Count=1;
    if(output)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ComPtr<ID3D12Resource> texture;
    Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,output?D3D12_RESOURCE_STATE_UNORDERED_ACCESS:D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&texture)));
    gpu.retained.push_back(texture);
    if(data) {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
        gpu.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
        std::vector<unsigned char> padded(static_cast<size_t>(bytes));
        for(UINT y=0;y<height;++y)std::memcpy(padded.data()+size_t(y)*footprint.Footprint.RowPitch,static_cast<const unsigned char*>(data)+size_t(y)*width*16,size_t(width)*16);
        auto upload=gpu.Upload(padded);
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=texture.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
        gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        gpu.Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    return texture;
}
static void TextureView(Device& gpu,ID3D12Resource* resource,UINT slot,bool output=false) {
    auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=SIZE_T(slot)*gpu.descriptorSize;
    if(output) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC d{};d.Format=resource->GetDesc().Format;d.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        gpu.device->CreateUnorderedAccessView(resource,nullptr,&d,cpu);
    } else {
        D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.Format=resource->GetDesc().Format;d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        d.Texture2D.MipLevels=1;d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        gpu.device->CreateShaderResourceView(resource,&d,cpu);
    }
}
struct Readback { ComPtr<ID3D12Resource> buffer;UINT rowPitch; };
static Readback CopyOutput(Device& gpu,ID3D12Resource* texture) {
    auto desc=texture->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;
    gpu.device->GetCopyableFootprints(&desc,0,1,0,&fp,nullptr,nullptr,&bytes);
    auto buffer=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.Transition(texture,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=buffer.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=fp;
    D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=texture;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);return {buffer,fp.Footprint.RowPitch};
}
int main() try {
    std::cout<<std::unitbuf<<"Creating WARP device\n";
    Device gpu(false);
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,17,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,17}};
    D3D12_ROOT_PARAMETER params[3]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={2,ranges};
    for(UINT i=1;i<3;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor={i-1,0};}
    D3D12_STATIC_SAMPLER_DESC samplers[3]{};const UINT registers[]={0,6,11};
    for(UINT i=0;i<3;++i){auto& s=samplers[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxLOD=D3D12_FLOAT32_MAX;s.ShaderRegister=registers[i];s.RegisterSpace=1;}
    D3D12_ROOT_SIGNATURE_DESC desc{3,params,3,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    std::cout<<"Root signature created; disassembling native shader\n";
    auto shader=Read("rt_lights_native_spot.dxbc");ComPtr<ID3DBlob> assembly;Check(D3DDisassemble(shader.data(),shader.size(),0,nullptr,&assembly));
    std::cout<<"Native shader disassembled; creating pipeline\n";
    D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=root.Get();ps.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pipeline;Check(gpu.device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&pipeline)));
    std::cout<<"Native pipeline created; creating fixture resources\n";
    std::vector<float> constants(320,0);constants[0]=constants[1]=2;constants[2]=constants[3]=0.5f;
    for(UINT i=0;i<4;++i)constants[56+i*5]=1; // Native clip-to-view matrix at byte 224.
    auto cb=gpu.Upload(constants);auto cb1=gpu.Upload(std::vector<float>(64,0));
    const std::array<Pixel,4> white={Pixel{1,1,1,1},Pixel{1,1,1,1},Pixel{1,1,1,1},Pixel{1,1,1,1}};
    auto whiteTexture=Texture(gpu,2,2,DXGI_FORMAT_R32G32B32A32_FLOAT,white.data());
    for(UINT i=0;i<17;++i)TextureView(gpu,whiteTexture.Get(),i);
    auto setTexture=[&](UINT slot,Pixel pixel){std::array<Pixel,4> values;values.fill(pixel);auto t=Texture(gpu,2,2,DXGI_FORMAT_R32G32B32A32_FLOAT,values.data());TextureView(gpu,t.Get(),slot);};
    setTexture(0,{0.5f,0.5f,0.5f,0});setTexture(1,{0.5f,0.5f,0,0});setTexture(3,{0.5f,0,0,0});setTexture(16,{0.5f,0.5f,0,0});
    auto material=gpu.Upload(std::vector<UINT>{0x80808000,0x000080ff});gpu.View(material.Get(),6,8,1,false);
    auto material3=gpu.Upload(std::vector<float>(84,0));gpu.View(material3.Get(),7,336,1,false);
    auto point=gpu.Upload(std::vector<float>(40,0));gpu.View(point.Get(),9,160,1,false);
    std::vector<float> lights(160,0);
    for(UINT i=0;i<2;++i){const UINT base=i*80;lights[base+12+i*2]=2;lights[base+15]=10;lights[base+17]=1;lights[base+18]=0.05f;for(UINT j=0;j<4;++j)lights[base+20+j*5]=1;}
    auto lightBuffer=gpu.Upload(lights);gpu.View(lightBuffer.Get(),13,320,2,false);
    // Two lights at the same receiver, another receiver, invalid index/coordinates,
    // disabled work and the native sentinel. All remaining atlas entries disabled.
    std::array<Job,64> jobs{};jobs[0]={0,0,0,1};jobs[1]={1,0,0,1};jobs[2]={0,1,1,1};jobs[3]={2,0,0,1};jobs[4]={0,2,0,1};jobs[5]={0,0,0,0};jobs[6]={65535,0,0,1};
    auto jobTexture=Texture(gpu,8,8,DXGI_FORMAT_R32G32B32A32_UINT,jobs.data());TextureView(gpu,jobTexture.Get(),14);
    auto diffuse=Texture(gpu,8,8,DXGI_FORMAT_R32G32B32A32_FLOAT,nullptr,true);TextureView(gpu,diffuse.Get(),17,true);
    auto specular=Texture(gpu,8,8,DXGI_FORMAT_R32G32B32A32_FLOAT,nullptr,true);TextureView(gpu,specular.Get(),18,true);
    std::cout<<"Fixture resources created; recording native dispatch\n";
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pipeline.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    gpu.list->SetComputeRootConstantBufferView(1,cb->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(2,cb1->GetGPUVirtualAddress());gpu.list->Dispatch(1,1,1);
    auto d=CopyOutput(gpu,diffuse.Get());auto s=CopyOutput(gpu,specular.Get());
    gpu.Transition(diffuse.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    gpu.Transition(specular.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    std::vector<rt_lights::ShadowTask> tasks(64);
    for(UINT i=0;i<64;++i){tasks[i].lightId=jobs[i][0];tasks[i].receiverId=jobs[i][2]*2+jobs[i][1];tasks[i].frameTag=19;}
    // A second valid extraction with a stale task must not reach composition.
    tasks[2].frameTag=18;
    auto taskBuffer=gpu.Upload(tasks);
    auto contributionBuffer=gpu.Buffer(64*sizeof(rt_lights::Contribution),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    TextureView(gpu,jobTexture.Get(),19);TextureView(gpu,diffuse.Get(),20);TextureView(gpu,specular.Get(),21);
    gpu.View(taskBuffer.Get(),22,sizeof(rt_lights::ShadowTask),64,false);
    gpu.View(contributionBuffer.Get(),23,sizeof(rt_lights::Contribution),64,true);
    D3D12_DESCRIPTOR_RANGE packRanges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,4}};
    D3D12_ROOT_PARAMETER packParams[2]{};
    packParams[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;packParams[0].DescriptorTable={2,packRanges};
    packParams[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;packParams[1].Constants={0,0,4};
    D3D12_ROOT_SIGNATURE_DESC packDesc{2,packParams,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    Check(D3D12SerializeRootSignature(&packDesc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> packRoot;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&packRoot)));
    auto packShader=Read("rt_lights_native_pack.dxil");ps.pRootSignature=packRoot.Get();ps.CS={packShader.data(),packShader.size()};
    ComPtr<ID3D12PipelineState> packPipeline;Check(gpu.device->CreateComputePipelineState(&ps,IID_PPV_ARGS(&packPipeline)));
    gpu.list->SetComputeRootSignature(packRoot.Get());gpu.list->SetPipelineState(packPipeline.Get());
    auto packTable=gpu.views->GetGPUDescriptorHandleForHeapStart();packTable.ptr+=19ULL*gpu.descriptorSize;
    gpu.list->SetComputeRootDescriptorTable(0,packTable);const UINT batch[]={2,2,19,64};gpu.list->SetComputeRoot32BitConstants(1,4,batch,0);gpu.list->Dispatch(1,1,1);
    gpu.Transition(contributionBuffer.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto packedReadback=gpu.Buffer(64*sizeof(rt_lights::Contribution),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->CopyResource(packedReadback.Get(),contributionBuffer.Get());std::cout<<"Submitting GPU work\n";gpu.Submit();std::cout<<"GPU work completed\n";
    void* packedData=nullptr;Check(packedReadback->Map(0,nullptr,&packedData));auto* packed=static_cast<rt_lights::Contribution*>(packedData);
    for(UINT i=0;i<64;++i) {
        if(i<2){Require(packed[i].visibilityIndex==i && packed[i].receiverId==0 && packed[i].lightId==i && packed[i].frameTag==19,"Packed identity mismatch");}
        else Require(packed[i].visibilityIndex==0xffffffff,"Invalid or stale extraction reached composition");
    }
    Require(packed[0].diffuse.x>0 && packed[1].diffuse.z>0,"Packed native energy missing");
    packedReadback->Unmap(0,nullptr);
    for(auto* result:{&d,&s}) {
        void* mapped=nullptr;Check(result->buffer->Map(0,nullptr,&mapped));auto* pixels=static_cast<Pixel*>(mapped);
        for(UINT i=0;i<8;++i){auto p=pixels[i];std::cout<<(result==&d?"diffuse":"specular")<<" task="<<i<<" rgb="<<p[0]<<','<<p[1]<<','<<p[2]<<" valid="<<p[3]<<'\n';for(float x:p)Require(std::isfinite(x),"Nonfinite native contribution");Require(p[3]==(i<3?1.0f:0.0f),"Wrong extraction validity");}
        Require(pixels[0][0]>0 && pixels[1][2]>0,"Native lit fixture produced no energy");
        Require(pixels[0][1]==0 && pixels[0][2]==0 && pixels[1][0]==0 && pixels[1][1]==0,"Light contributions mixed");
        Require(std::abs(pixels[0][0]-pixels[1][2])<1e-5f,"Identical lights disagree after color swap");
        for(UINT i=3;i<8;++i)Require(pixels[i]==Pixel{},"Invalid work retained output");
        result->buffer->Unmap(0,nullptr);
    }
    std::cout<<"PASS: native spotlight extraction on WARP; separate colors, receiver mapping, invalid work rejection and frame-keyed contribution packing\n";return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
