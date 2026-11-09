#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/receiver_input.h"
#include "../src/gi_reuse/d3d12_visibility_pipeline.h"
using namespace gi_reuse;
static ComPtr<ID3D12InfoQueue> debugMessages;
static bool ReportDebugErrors() {
    if(!debugMessages) return false;
    bool errors=false;
    const UINT64 count=debugMessages->GetNumStoredMessagesAllowedByRetrievalFilter();
    for(UINT64 i=0;i<count&&i<1024;++i) {
        SIZE_T size=0;if(FAILED(debugMessages->GetMessage(i,nullptr,&size))) continue;
        std::vector<unsigned char> storage(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        if(FAILED(debugMessages->GetMessage(i,message,&size))) continue;
        if(message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING) std::cerr<<"D3D12: "<<message->pDescription<<'\n';
        errors|=message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR;
    }
    return errors;
}
static ComPtr<ID3D12RootSignature> Root(Device& gpu,bool ray) {
    D3D12_ROOT_PARAMETER p[4]{};D3D12_DESCRIPTOR_RANGE ranges[3]{};
    for(UINT i=0;i<3;++i) {
        ranges[i].RangeType=i==2?D3D12_DESCRIPTOR_RANGE_TYPE_UAV:D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[i].NumDescriptors=1;ranges[i].BaseShaderRegister=i==2?0:i;
        p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i].DescriptorTable={1,&ranges[i]};
    }
    if(ray) {
        p[0]={};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[0].Descriptor={0,7};
        ranges[1].RegisterSpace=ranges[2].RegisterSpace=7;
        p[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[3].Constants={0,7,8};
    }
    D3D12_ROOT_SIGNATURE_DESC d{ray?4u:3u,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&d,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));return root;
}
static ComPtr<ID3D12Resource> Scene(Device& gpu) {
    // Opaque left half and alpha-tested right half of a plane between receiver
    // and lights. Dedicated hit records ensure cutout decisions stay per geometry.
    std::vector<GIPoint> vertices={
        {-2,-4,2},{0,-4,2},{0,4,2},{-2,-4,2},{0,4,2},{-2,4,2},
        {0,-4,2},{2,-4,2},{2,4,2},{0,-4,2},{2,4,2},{0,4,2}};
    auto vb=gpu.Upload(vertices);
    D3D12_RAYTRACING_GEOMETRY_DESC geometry[2]{};
    for(UINT i=0;i<2;++i) {
        auto& g=geometry[i];g.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        g.Flags=i==0?D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE:D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.VertexCount=6;
        g.Triangles.VertexBuffer={vb->GetGPUVirtualAddress()+i*6*sizeof(GIPoint),sizeof(GIPoint)};
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
    inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=2;inputs.pGeometryDescs=geometry;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO size{};gpu.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&size);
    Require(size.ResultDataMaxSizeInBytes>0,"BLAS prebuild returned no size");
    auto blas=gpu.Buffer(size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,true);
    auto scratch=gpu.Buffer(size.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};build.Inputs=inputs;build.DestAccelerationStructureData=blas->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
    gpu.list->BuildRaytracingAccelerationStructure(&build,0,nullptr);gpu.Uav(blas.Get());
    D3D12_RAYTRACING_INSTANCE_DESC instance{};instance.Transform[0][0]=instance.Transform[1][1]=instance.Transform[2][2]=1;
    instance.InstanceMask=1;instance.AccelerationStructure=blas->GetGPUVirtualAddress();
    auto instances=gpu.Upload(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>{instance});
    inputs={};inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=1;inputs.InstanceDescs=instances->GetGPUVirtualAddress();
    gpu.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&size);Require(size.ResultDataMaxSizeInBytes>0,"TLAS prebuild returned no size");
    auto tlas=gpu.Buffer(size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,true);
    scratch=gpu.Buffer(size.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    build={};build.Inputs=inputs;build.DestAccelerationStructureData=tlas->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
    gpu.list->BuildRaytracingAccelerationStructure(&build,0,nullptr);gpu.Uav(tlas.Get());return tlas;
}

static GIVisibilityRequest Request(uint index,GIPoint target) {
    GIVisibilityRequest r{};r.connection.secondary=target;r.connection.secondaryGeometricNormal={0,0,-1};
    r.connection.sourceMaxDistance=r.connection.targetMaxDistance=100;r.connection.sourceSolidAnglePdf=1;
    r.connection.frameTag=19;r.connection.connectionId=index+100;r.connection.verified=7;
    r.originTMin=r.endpointBias=0.0001f;r.instanceMask=r.nativeBindingsVerified=1;r.sceneRevision=7;return r;
}
static HRESULT STDMETHODCALLTYPE Create(ID3D12Device5* d,const D3D12_STATE_OBJECT_DESC* s,REFIID iid,void** out) {return d->CreateStateObject(s,iid,out);}
int main() try {
    std::cout<<"GI visibility + receiver shading GPU integration test. No game files modified.\n";
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) debug->EnableDebugLayer();
    Device gpu;gpu.device.As(&debugMessages);std::cout<<"Debug layer: "<<(debugMessages?"enabled":"unavailable")<<'\n';
    auto scene=Scene(gpu);auto root=Root(gpu,true);auto computeRoot=Root(gpu,false);
    auto bytes=Read("gi-visibility-fixture.dxil");
    D3D12_EXPORT_DESC exports[]={
        {L"nativeBlocked",L"giConnectionBlocked",D3D12_EXPORT_FLAG_NONE},
        {L"nativeCutout",L"giConnectionCutout",D3D12_EXPORT_FLAG_NONE},
        {L"nativeMiss",L"giConnectionMiss",D3D12_EXPORT_FLAG_NONE},
        {L"spotRayGeneration",L"giConnectionVisibility",D3D12_EXPORT_FLAG_NONE}};
    D3D12_DXIL_LIBRARY_DESC lib{{bytes.data(),bytes.size()},4,exports};
    D3D12_HIT_GROUP_DESC opaque{L"opaque",D3D12_HIT_GROUP_TYPE_TRIANGLES,nullptr,L"nativeBlocked",nullptr};
    D3D12_HIT_GROUP_DESC cutout{L"cutout",D3D12_HIT_GROUP_TYPE_TRIANGLES,L"nativeCutout",L"nativeBlocked",nullptr};
    D3D12_RAYTRACING_SHADER_CONFIG shader{4,8};D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};ID3D12RootSignature* ptr=root.Get();
    D3D12_STATE_SUBOBJECT objects[]={
        {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&lib},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&opaque},
        {D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&cutout},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&shader},
        {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline},
        {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&ptr}};
    D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,6,objects};
    VisibilityPipeline visibility;Check(visibility.Initialize(gpu.device.Get(),desc,{bytes.data(),bytes.size()},&Create));
    Require(visibility.Identifiers().size()==2,"Missing alpha/opaque mappings");
    std::vector<unsigned char> nativeTable(128,0),table;
    for(size_t i=0;i<2;++i)std::memcpy(nativeTable.data()+64*i,visibility.Identifiers()[i].native.data(),32);
    Require(visibility.RemapTable(visibility.State(),nativeTable.data(),nativeTable.size(),64,{1,1},table),"Native table remapping failed");
    Require(!visibility.RemapTable(nullptr,nativeTable.data(),nativeTable.size(),64,{1,1},table),"Unknown state table accepted");
    auto sbt=gpu.Upload(table);
    std::vector<GIVisibilityRequest> requests={Request(0,{-2,0,4}),Request(1,{2,0,4}),Request(2,{-2,0,1}),Request(3,{-2,0,4}),Request(4,{2,0,4}),Request(5,{2,0,4}),Request(6,{2,0,4}),Request(7,{2,0,4}),Request(8,{2,0,4}),Request(9,{2,0,4})};
    requests[3].instanceMask=2;requests[4].sceneRevision=6;requests[5].connection.frameTag=18;
    requests[6].nativeBindingsVerified=0;requests[7].endpointBias=100;requests[8].reserved0=1;requests[9].connection.secondaryGeometricNormal={0,0,0};
    const UINT n=static_cast<UINT>(requests.size());
    auto input=gpu.Upload(requests);
    auto output=gpu.Buffer((n+1)*sizeof(GIConnectionVisibility),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
    std::vector<unsigned char> guard((n+1)*sizeof(GIConnectionVisibility),0xcd);auto initial=gpu.Upload(guard);
    gpu.list->CopyResource(output.Get(),initial.Get());gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetComputeRootShaderResourceView(0,scene->GetGPUVirtualAddress());
    gpu.list->SetComputeRootDescriptorTable(1,gpu.View(input.Get(),0,sizeof(GIVisibilityRequest),n,false));
    gpu.list->SetComputeRootDescriptorTable(2,gpu.View(output.Get(),1,sizeof(GIConnectionVisibility),n,true));
    GIVisibilityConstants constants{0,1,0,19,7,1,0,0};gpu.list->SetComputeRoot32BitConstants(3,8,&constants,0);gpu.list->SetPipelineState1(visibility.State());
    D3D12_DISPATCH_RAYS_DESC rays{};rays.RayGenerationShaderRecord={visibility.Records()->GetGPUVirtualAddress(),32};rays.MissShaderTable={visibility.Records()->GetGPUVirtualAddress()+64,32,32};rays.HitGroupTable={sbt->GetGPUVirtualAddress(),128,64};rays.Width=n+1;rays.Height=rays.Depth=1;
    gpu.list->DispatchRays(&rays);gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    std::vector<GIReceiverInput> receivers;
    for(const auto& request:requests) {
        GIReceiverInput v{};v.connection=request.connection;v.receiver.secondaryRadiance={2,3,4};v.receiver.material=v.connection.material;
        v.sourceRange=v.receiverRange=10;v.sourceCosine=v.receiverCosine=request.connection.secondary.z/std::sqrt(GIDot(request.connection.secondary,request.connection.secondary));
        const float response=v.sourceCosine/3.14159265358979323846f;v.receiver.primaryBrdfCos={response,response,response};
        v.receiver.frameTag=v.frameTag=19;v.receiver.configuredCount=1;v.receiver.depthScale=1;v.receiver.verified=7;
        v.producerVerified=v.radianceVerified=1;v.sceneRevision=7;receivers.push_back(v);
    }
    auto receiverBuffer=gpu.Upload(receivers);auto shaded=gpu.Buffer(n*sizeof(GIInitialResult),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    auto cs=Read("gi-receiver-shading.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC cp{};cp.pRootSignature=computeRoot.Get();cp.CS={cs.data(),cs.size()};ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&cp,IID_PPV_ARGS(&pso)));
    gpu.list->SetComputeRootSignature(computeRoot.Get());gpu.list->SetPipelineState(pso.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.View(receiverBuffer.Get(),2,sizeof(GIReceiverInput),n,false));
    gpu.list->SetComputeRootDescriptorTable(1,gpu.View(output.Get(),3,sizeof(GIConnectionVisibility),n,false));
    gpu.list->SetComputeRootDescriptorTable(2,gpu.View(shaded.Get(),4,sizeof(GIInitialResult),n,true));gpu.list->Dispatch(1,1,1);
    gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.Transition(shaded.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto read=gpu.Buffer((n+1)*sizeof(GIConnectionVisibility),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    auto shadeRead=gpu.Buffer(n*sizeof(GIInitialResult),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->CopyResource(read.Get(),output.Get());gpu.list->CopyResource(shadeRead.Get(),shaded.Get());gpu.Submit();
    GIConnectionVisibility* result=nullptr;GIInitialResult* color=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&result)));Check(shadeRead->Map(0,nullptr,reinterpret_cast<void**>(&color)));
    for(UINT i=0;i<n;++i) {
        Require(result[i].connectionId==requests[i].connection.connectionId&&result[i].sceneRevision==requests[i].sceneRevision,"Lost trace identity");
        Require(result[i].completed==(i<4?1u:0u),"Invalid trace accepted or valid trace rejected");
        Require(result[i].visible==(i>0&&i<4?1u:0u),"Opaque/cutout/endpoint/mask visibility mismatch");
        Require(color[i].valid==(i>0&&i<4?1u:0u),"Receiver admitted invalid/blocked evidence");
        if(color[i].valid) Require(std::abs(color[i].surfaceContribution.x-2)<1e-5f&&std::abs(color[i].surfaceContribution.y-3)<1e-5f&&std::abs(color[i].surfaceContribution.z-4)<1e-5f,"Receiver energy changed");
    }
    Require(!std::memcmp(result+n,guard.data()+n*sizeof(GIConnectionVisibility),sizeof(GIConnectionVisibility)),"Dispatch wrote past output view");
    read->Unmap(0,nullptr);shadeRead->Unmap(0,nullptr);
    Require(!ReportDebugErrors(),"D3D12 debug validation errors");
    std::cout<<"PASS: actual DXR -> receiver shading; opaque, dirty-payload cutout, endpoint, mask, scene/frame rejection, energy and output guard. Synthetic scene, not game integration.\n";return 0;
} catch(const std::exception& e) {ReportDebugErrors();std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
