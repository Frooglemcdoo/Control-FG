#include "gpu_test_textures.h"
#include "../src/rt_lights/layout.h"
#include <limits>
int main() try {
    Device gpu(false);
    constexpr UINT width=8,height=1,first=1,count=6;
    std::vector<rt_lights::Lighting> lighting(count);
    for(auto& v:lighting){v.valid=1;v.diffuse={2,3,4};v.specular={5,6,7};v.lightCount=2;}
    lighting[1].valid=0;
    lighting[2].specular.x=std::numeric_limits<float>::quiet_NaN();
    lighting[3].diffuse.x=65504;
    lighting[4].specular.x=std::numeric_limits<float>::infinity();
    auto buffer=gpu.Upload(lighting);gpu.View(buffer.Get(),0,sizeof(rt_lights::Lighting),count,false);
    std::vector<Pixel> baseD(width,Pixel{10,20,30,.25f}),baseS(width,Pixel{40,50,60,.75f});
    std::vector<Pixel> originalD(width,Pixel{100,200,300,.5f}),originalS(width,Pixel{400,500,600,1});
    auto bd=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_FLOAT,baseD.data());TextureView(gpu,bd.Get(),1);
    auto bs=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_FLOAT,baseS.data());TextureView(gpu,bs.Get(),2);
    auto od=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_FLOAT,originalD.data(),true);TextureView(gpu,od.Get(),3,true);
    auto os=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_FLOAT,originalS.data(),true);TextureView(gpu,os.Get(),4,true);
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,3}};
    D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={2,ranges};
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[1].Constants={0,0,4};
    D3D12_ROOT_SIGNATURE_DESC desc{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read("rt_lights_native_commit.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    const UINT batch[]={first,count,width,height};gpu.list->SetComputeRoot32BitConstants(1,4,batch,0);gpu.list->Dispatch(1,1,1);
    auto d=CopyOutput(gpu,od.Get()),s=CopyOutput(gpu,os.Get());gpu.Submit();
    void* dm=nullptr;void* sm=nullptr;Check(d.buffer->Map(0,nullptr,&dm));Check(s.buffer->Map(0,nullptr,&sm));
    const auto* da=static_cast<const Pixel*>(dm);const auto* sa=static_cast<const Pixel*>(sm);
    for(UINT pixel=0;pixel<width;++pixel) {
        const bool replaced=pixel==1 || pixel==6;
        Require(da[pixel]==(replaced?Pixel{12,23,34,.25f}:originalD[pixel]),"Diffuse commit overwrote fallback or added native lighting twice");
        Require(sa[pixel]==(replaced?Pixel{45,56,67,.75f}:originalS[pixel]),"Specular commit did not share diffuse validity");
    }
    d.buffer->Unmap(0,nullptr);s.buffer->Unmap(0,nullptr);
    std::cout<<"PASS: paired lighting commit, global batch indexing, native fallback, alpha preservation, NaN/infinity and half-float overflow rejection\n";
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
