#include "gpu_test_textures.h"
#include "../src/rt_lights/layout.h"
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
    // Validate extraction against the captured native global root as well as
    // the smaller fixture root. Dispatch below still uses the fixture bindings.
    auto nativeRootBytes=Read("native_global.rootsig");
    ComPtr<ID3D12RootSignatureDeserializer> deserializer;
    Check(D3D12CreateRootSignatureDeserializer(nativeRootBytes.data(),nativeRootBytes.size(),IID_PPV_ARGS(&deserializer)));
    const auto* nativeDesc=deserializer->GetRootSignatureDesc();
    Require(nativeDesc && nativeDesc->NumParameters==8,"Native root layout changed");
    const auto& table=nativeDesc->pParameters[0];
    Require(table.ParameterType==D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && table.DescriptorTable.NumDescriptorRanges==3,"Native global table changed");
    const auto* nativeRanges=table.DescriptorTable.pDescriptorRanges;
    Require(nativeRanges[0].RangeType==D3D12_DESCRIPTOR_RANGE_TYPE_CBV && nativeRanges[0].BaseShaderRegister==4 && nativeRanges[0].NumDescriptors==5 && nativeRanges[0].RegisterSpace==0,"Native b4 slot absent");
    Require(nativeRanges[1].RangeType==D3D12_DESCRIPTOR_RANGE_TYPE_SRV && nativeRanges[1].BaseShaderRegister==0 && nativeRanges[1].NumDescriptors==36 && nativeRanges[1].RegisterSpace==0,"Native t6/t8 slots absent");
    Require(nativeRanges[2].RangeType==D3D12_DESCRIPTOR_RANGE_TYPE_UAV && nativeRanges[2].BaseShaderRegister==0 && nativeRanges[2].NumDescriptors==8 && nativeRanges[2].RegisterSpace==0,"Native u2 slot absent");
    ComPtr<ID3D12RootSignature> nativeRoot;
    Check(gpu.device->CreateRootSignature(0,nativeRootBytes.data(),nativeRootBytes.size(),IID_PPV_ARGS(&nativeRoot)));
    auto nativePipelineDesc=ps;nativePipelineDesc.pRootSignature=nativeRoot.Get();
    ComPtr<ID3D12PipelineState> nativePipeline;Check(gpu.device->CreateComputePipelineState(&nativePipelineDesc,IID_PPV_ARGS(&nativePipeline)));
    std::cout<<"PASS: captured native global root and extraction pipeline compatibility\n";
    for(const char* file:{"rt_lights_native_base.dxbc","rt_lights_native_point.dxbc"}) {
        auto replay=Read(file);auto replayDesc=nativePipelineDesc;replayDesc.CS={replay.data(),replay.size()};
        ComPtr<ID3D12PipelineState> replayPso;Check(gpu.device->CreateComputePipelineState(&replayDesc,IID_PPV_ARGS(&replayPso)));
        std::cout<<"PASS: private native replay pipeline "<<file<<'\n';
    }

    std::vector<float> constants(320,0);constants[0]=constants[1]=2;constants[2]=constants[3]=0.5f;
    for(UINT i=0;i<4;++i)constants[56+i*5]=1; // Native clip-to-view matrix at byte 224.
    auto cb=gpu.Upload(constants);auto cb1=gpu.Upload(std::vector<float>(64,0));
    const std::array<Pixel,4> white={Pixel{1,1,1,1},Pixel{1,1,1,1},Pixel{1,1,1,1},Pixel{1,1,1,1}};
    auto whiteTexture=Texture(gpu,2,2,DXGI_FORMAT_R32G32B32A32_FLOAT,white.data());
    for(UINT i=0;i<17;++i)TextureView(gpu,whiteTexture.Get(),i);
    auto setTexture=[&](UINT slot,Pixel pixel){std::array<Pixel,4> values;values.fill(pixel);auto t=Texture(gpu,2,2,DXGI_FORMAT_R32G32B32A32_FLOAT,values.data());TextureView(gpu,t.Get(),slot);};
    setTexture(0,{0.5f,0.5f,0.5f,0});setTexture(1,{0.5f,0.5f,0,0});
    const std::array<Pixel,4> depth={Pixel{0.5f,0,0,0},Pixel{0.5f,0,0,0},Pixel{0.5f,0,0,0},Pixel{2,0,0,0}};
    auto depthTexture=Texture(gpu,2,2,DXGI_FORMAT_R32G32B32A32_FLOAT,depth.data());TextureView(gpu,depthTexture.Get(),3);
    // Blocked map/contact inputs must not erase the unshadowed extraction.
    setTexture(10,{0,0,0,0});setTexture(11,{0,0,0,0});setTexture(15,{0,0,0,0});setTexture(16,{0.5f,0.5f,0,0});
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
        Require(pixels[2]==Pixel{0,0,0,1},"Atlas slot did not use its assigned receiver pixel");
        for(UINT i=3;i<8;++i)Require(pixels[i]==Pixel{},"Invalid work retained output");
        result->buffer->Unmap(0,nullptr);
    }
    std::cout<<"PASS: native spotlight extraction on WARP; separate colors, receiver mapping, invalid work rejection and frame-keyed contribution packing\n";return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
