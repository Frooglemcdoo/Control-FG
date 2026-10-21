#include "gpu_test_textures.h"
#include "../src/rt_lights/layout.h"
using namespace rt_lights;
static UINT Bits(float value) { UINT bits=0;std::memcpy(&bits,&value,4);return bits; }
static float Length(float3 value) { return std::sqrt(value.x*value.x+value.y*value.y+value.z*value.z); }
static float Dot(float3 a,float3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static bool Near(float a,float b) { return std::abs(a-b)<0.00001f; }
static ComPtr<ID3D12Resource> CopyBuffer(Device& gpu,ID3D12Resource* source) {
    auto copy=gpu.Buffer(source->GetDesc().Width,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.Transition(source,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    gpu.list->CopyResource(copy.Get(),source);return copy;
}
enum class Case { Normal, Capacity, Unterminated, InvalidLight, InvalidInverseSize, StaleComposition, Overflow };
static void Run(Case test,bool compact=false) {
    Device gpu(false);
    D3D12_DESCRIPTOR_RANGE bindings[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,4},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,3,0,11}};
    D3D12_ROOT_PARAMETER parameters[4]{};
    parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={compact?3u:2u,bindings};
    for(UINT i=1;i<3;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor={i-1,0};}
    parameters[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[3].Constants={2,0,12};
    D3D12_ROOT_SIGNATURE_DESC desc{4,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read(compact?"rt_lights_native_compact.dxil":"rt_lights_native_batch.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};pipelineDesc.pRootSignature=root.Get();pipelineDesc.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pipeline;Check(gpu.device->CreateComputePipelineState(&pipelineDesc,IID_PPV_ARGS(&pipeline)));
    // Two input pixels are deliberately outside the first 64-pixel tile. The
    // batch begins at receiver 62, so receiver IDs cannot equal local indices.
    constexpr UINT width=67,height=1,firstReceiver=62,receiverCount=5;
    constexpr UINT atlasWidth=8,atlasHeight=8;
    const UINT taskCapacity=test==Case::Overflow?3u:16u;
    const UINT slots=test==Case::Capacity?2:3;
    std::vector<float> system(320,0);system[0]=float(width);system[1]=float(height);
    system[2]=test==Case::InvalidInverseSize?1.0f:1.0f/width;system[3]=1.0f/height;
    for(UINT i=0;i<4;++i)system[56+i*5]=1;
    // Rotation around Z followed by translation; native columns at byte 96.
    system[25]=1;system[28]=-1;system[34]=1;system[36]=10;system[37]=20;system[38]=30;system[39]=1;
    auto sys=gpu.Upload(system);
    std::vector<UINT> deferred(128,0);
    deferred[40]=Bits(1);deferred[41]=Bits(0);deferred[42]=2;deferred[50]=2;deferred[54]=Bits(0.5f);
    deferred[57]=3;deferred[65]=2;
    auto def=gpu.Upload(deferred);
    std::vector<Pixel> normals(width,Pixel{0.5f,0.5f,0.5f,0});
    std::vector<Pixel> depths(width,Pixel{0.25f,0,0,0});depths[65][0]=0.75f;
    auto normalTexture=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_FLOAT,normals.data());TextureView(gpu,normalTexture.Get(),0);
    auto depthTexture=Texture(gpu,width,height,DXGI_FORMAT_R32G32B32A32_FLOAT,depths.data());TextureView(gpu,depthTexture.Get(),1);
    // Tile 0 has both lights. Tile 1 near has only light 1; its far slice is empty.
    std::array<Job,12> cells;cells.fill(Job{65535,0,0,0});cells[0][0]=0;cells[1][0]=1;cells[3][0]=1;
    if(test==Case::Unterminated)cells[2][0]=0;
    if(test==Case::InvalidLight)cells[1][0]=2;
    auto cellTexture=Texture(gpu,6,2,DXGI_FORMAT_R32G32B32A32_UINT,cells.data());TextureView(gpu,cellTexture.Get(),2);
    std::vector<float> lights(160,0);
    for(UINT light=0;light<2;++light){const UINT offset=80*light;lights[offset]=99;lights[offset+4]=float(light)*2;lights[offset+6]=2;lights[offset+18]=0.25f*float(light+1);}
    auto lightBuffer=gpu.Upload(lights);gpu.View(lightBuffer.Get(),3,320,2,false);
    auto jobs=Texture(gpu,atlasWidth,atlasHeight,DXGI_FORMAT_R32G32B32A32_UINT,nullptr,true);TextureView(gpu,jobs.Get(),4,true);
    auto tasks=gpu.Buffer(taskCapacity*sizeof(ShadowTask),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);gpu.View(tasks.Get(),5,sizeof(ShadowTask),taskCapacity,true);
    auto ranges=gpu.Buffer(receiverCount*sizeof(ReceiverRange),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);gpu.View(ranges.Get(),6,sizeof(ReceiverRange),receiverCount,true);
    ComPtr<ID3D12Resource> counter;
    if(compact) {
        counter=gpu.Buffer(4,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
        auto zero=gpu.Upload(std::vector<UINT>{0});gpu.list->CopyBufferRegion(counter.Get(),0,zero.Get(),0,4);
        gpu.Transition(counter.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gpu.View(counter.Get(),11,4,1,true);
    }
    const UINT batch[]={firstReceiver,receiverCount,slots,19,32,1,Bits(0.001f),Bits(0.002f),atlasWidth,atlasHeight,Bits(2),0};
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pipeline.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    gpu.list->SetComputeRootConstantBufferView(1,sys->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(2,def->GetGPUVirtualAddress());
    gpu.list->SetComputeRoot32BitConstants(3,12,batch,0);gpu.list->Dispatch(1,1,1);
    ComPtr<ID3D12Resource> counterCopy;
    if(compact)counterCopy=CopyBuffer(gpu,counter.Get());
    auto taskCopy=CopyBuffer(gpu,tasks.Get());auto rangeCopy=CopyBuffer(gpu,ranges.Get());auto workCopy=CopyOutput(gpu,jobs.Get());
    gpu.Transition(ranges.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    std::vector<Contribution> contributions(16);std::vector<Visibility> visibility(16);
    for(UINT local=0;local<receiverCount;++local)for(UINT slot=0;slot<slots;++slot) {
        const UINT index=local*slots+slot,receiver=firstReceiver+local,light=receiver<64?slot:1;
        auto& c=contributions[index];c.visibilityIndex=index;c.receiverId=receiver;c.lightId=light;c.frameTag=19;c.diffuse={1,2,3};c.specular={2,4,6};
        auto& v=visibility[index];v.value=.5f;v.valid=1;v.receiverId=receiver;v.lightId=light;v.frameTag=19;
    }
    auto contributionBuffer=gpu.Upload(contributions);auto visibilityBuffer=gpu.Upload(visibility);
    auto lighting=gpu.Buffer(receiverCount*sizeof(Lighting),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    gpu.View(visibilityBuffer.Get(),7,sizeof(Visibility),16,false);gpu.View(contributionBuffer.Get(),8,sizeof(Contribution),16,false);
    gpu.View(ranges.Get(),9,sizeof(ReceiverRange),receiverCount,false);gpu.View(lighting.Get(),10,sizeof(Lighting),receiverCount,true);
    D3D12_DESCRIPTOR_RANGE composeBindings[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,3}};
    D3D12_ROOT_PARAMETER composeParameters[2]{};
    composeParameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;composeParameters[0].DescriptorTable={2,composeBindings};
    composeParameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;composeParameters[1].Constants={0,0,4};
    D3D12_ROOT_SIGNATURE_DESC composeDesc{2,composeParameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    Check(D3D12SerializeRootSignature(&composeDesc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> composeRoot;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&composeRoot)));
    auto composeShader=Read("rt_lights_batch_compose.dxil");pipelineDesc.pRootSignature=composeRoot.Get();pipelineDesc.CS={composeShader.data(),composeShader.size()};
    ComPtr<ID3D12PipelineState> composePipeline;Check(gpu.device->CreateComputePipelineState(&pipelineDesc,IID_PPV_ARGS(&composePipeline)));
    gpu.list->SetComputeRootSignature(composeRoot.Get());gpu.list->SetPipelineState(composePipeline.Get());
    auto composeTable=gpu.views->GetGPUDescriptorHandleForHeapStart();composeTable.ptr+=7ULL*gpu.descriptorSize;
    gpu.list->SetComputeRootDescriptorTable(0,composeTable);
    const UINT composeBatch[]={firstReceiver,receiverCount,test==Case::StaleComposition?20u:19u,0};
    gpu.list->SetComputeRoot32BitConstants(1,4,composeBatch,0);gpu.list->Dispatch(1,1,1);
    auto lightingCopy=CopyBuffer(gpu,lighting.Get());gpu.Submit();
    void* mappedLighting=nullptr;Check(lightingCopy->Map(0,nullptr,&mappedLighting));auto* actualLighting=static_cast<Lighting*>(mappedLighting);
    void* mappedTasks=nullptr;void* mappedRanges=nullptr;void* mappedWork=nullptr;
    Check(taskCopy->Map(0,nullptr,&mappedTasks));Check(rangeCopy->Map(0,nullptr,&mappedRanges));Check(workCopy.buffer->Map(0,nullptr,&mappedWork));
    auto* actualTasks=static_cast<ShadowTask*>(mappedTasks);auto* actualRanges=static_cast<ReceiverRange*>(mappedRanges);
    UINT used=0;std::vector<bool> claimed(taskCapacity,false);
    if(compact) {void* data=nullptr;Check(counterCopy->Map(0,nullptr,&data));used=*static_cast<UINT*>(data);counterCopy->Unmap(0,nullptr);Require(used<=taskCapacity,"Compact counter exceeded capacity");}
    UINT validTasks=0;
    for(UINT local=0;local<receiverCount;++local) {
        const UINT receiver=firstReceiver+local;
        const bool invalid=(test==Case::Overflow && actualRanges[local].first==0xffffffff) || test==Case::Capacity || test==Case::InvalidInverseSize ||
            ((test==Case::Unterminated || test==Case::InvalidLight) && receiver<64);
        const UINT expectedCount=receiver<64?2:receiver==65?0:1;
        const auto& range=actualRanges[local];
        Require(range.frameTag==19,"Range frame mismatch");
        if(invalid)Require(range.first==0xffffffff && range.count==1,"Incomplete native list was accepted");
        else {
            Require((compact || range.first==local*slots) && range.count==expectedCount,"Wrong tile/depth list");
            if(compact)Require(range.first<=used && range.count<=used-range.first,"Range escaped compact allocation");
            validTasks+=range.count;
        }
        const bool composeInvalid=invalid || test==Case::StaleComposition;
        const auto& result=actualLighting[local];
        if(!compact)Require(result.valid==(composeInvalid?0u:1u),"Invalid or stale receiver was composed");
        if(!compact && !composeInvalid)Require(result.lightCount==expectedCount && Near(result.diffuse.x,.5f*expectedCount) && Near(result.specular.z,3.0f*expectedCount),"Global receiver batch composition failed");
        for(UINT slot=0;slot<(compact?(invalid?0:range.count):slots);++slot) {
            const UINT index=(compact?range.first:local*slots)+slot;
            if(compact){Require(index<taskCapacity && !claimed[index],"Compact ranges overlap");claimed[index]=true;}
            auto* row=reinterpret_cast<Job*>(static_cast<unsigned char*>(mappedWork)+(index/atlasWidth)*workCopy.rowPitch);
            const auto& job=row[index%atlasWidth];const auto& task=actualTasks[index];
            if(invalid || slot>=expectedCount){Require(job==Job{} && task.sampleCount==0,"Unused/invalid slot retained work");continue;}
            const UINT light=receiver<64?slot:1;
            Require(job==Job{light,receiver,0,1},"Wrong work identity");
            Require(task.lightId==light && task.receiverId==receiver && task.frameTag==19 && task.sampleCount==32 && task.instanceMask==1,"Wrong task identity/policy");
            const float viewX=(float(receiver)+0.5f)*2.0f/width-1;
            Require(Near(task.receiver.x,10) && Near(task.receiver.y,20+viewX) && Near(task.receiver.z,30.25f),"Incorrect receiver world transform");
            Require(Near(task.lightCenter.x,10) && Near(task.lightCenter.y,20+2.0f*light) && Near(task.lightCenter.z,32),"Wrong physical emitter or world transform");
            Require(Near(Length(task.normal),1) && task.normal.z<-.99f,"Packed normal not decoded");
            const float radius=0.5f*float(light+1);
            Require(Near(Length(task.emitterAxisU),radius) && Near(Length(task.emitterAxisV),radius) && Near(Dot(task.emitterAxisU,task.emitterAxisV),0),"Emitter disk basis invalid");
            Require(Near(task.originBias,0.001f) && Near(task.endpointBias,0.002f),"Ray bias changed");
        }
    }
    if(compact)Require(validTasks==used,"Compact tasks were lost or multiply owned");
    lightingCopy->Unmap(0,nullptr);taskCopy->Unmap(0,nullptr);rangeCopy->Unmap(0,nullptr);workCopy.buffer->Unmap(0,nullptr);
    std::cout<<"PASS: native batch case="<<int(test)<<" compact="<<compact<<'\n';
}
int main() try {
    std::cout<<std::unitbuf;
    for(Case test:{Case::Normal,Case::Capacity,Case::Unterminated,Case::InvalidLight,Case::InvalidInverseSize,Case::StaleComposition})Run(test);
    for(Case test:{Case::Normal,Case::Capacity,Case::Unterminated,Case::InvalidLight,Case::InvalidInverseSize,Case::Overflow})Run(test,true);
    std::cout<<"PASS: native tile/depth enumeration, complete-list rejection, receiver/emitter transforms and deterministic task generation\n";return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
