#include "gpu_test_device.h"
#include <d3d12sdklayers.h>
#include "../src/gi_reuse/d3d12_producer_pipeline.h"
#include "../src/gi_reuse/d3d12_producer_table.h"
#include "../src/gi_reuse/producer_ray.h"
using namespace gi_reuse;
int main()try {
    std::cout<<"Owned GI producer DXR test: private SBT, accepted geometry, native alpha policy and finite-range PDF.\n";
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    std::cout<<std::unitbuf;std::cerr<<std::unitbuf;
    Device gpu;ComPtr<ID3D12InfoQueue> messages;gpu.device.As(&messages);
    // Device binds its default heap while recording. Retain it through Submit,
    // even though this fixture switches to a larger heap before dispatch.
    auto initialViews=gpu.views;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=128;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Check(gpu.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&gpu.views)));ID3D12DescriptorHeap* heaps[]={gpu.views.Get()};gpu.list->SetDescriptorHeaps(1,heaps);
    auto cpu=[&](UINT slot){auto h=gpu.views->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(slot)*gpu.descriptorSize;return h;};
    auto handle=[&](UINT slot){auto h=gpu.views->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(slot)*gpu.descriptorSize;return h;};
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,5,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,27,0,0},
        {D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,2,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,105,0,1,0}};
    D3D12_ROOT_PARAMETER parameters[5]{};
    for(UINT i=0;i<4;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[i].DescriptorTable={1,ranges+i};}
    parameters[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[4].Constants={5,0,4};
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderRegister=5;sampler.RegisterSpace=1;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{5,parameters,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> rootBytes,error;
    Check(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&rootBytes,&error));ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto code=Read("gi-producer-trace.dxil");D3D12ProducerPipeline pipeline;
    Check(pipeline.Initialize(gpu.device.Get(),root.Get(),{code.data(),code.size()}));
    // Two overlapping planes: cutout R16 in front, opaque R32 behind.
    auto positions=gpu.Upload(std::vector<GIPoint>{{-4,-4,1},{4,-4,1},{0,4,1},{-4,-4,2},{4,-4,2},{0,4,2}});
    auto indices16=gpu.Upload(std::vector<std::uint16_t>{0,1,2,0});
    auto indices32=gpu.Upload(std::vector<uint>{0,1,2});
    struct Attribute {uint pad,normalXY,normalZ,uv;};
    auto attributes=gpu.Upload(std::vector<Attribute>(3,Attribute{0,0,32767,1024u|(3071u<<16)}));
    for(auto* resource:{positions.Get(),indices16.Get(),indices32.Get(),attributes.Get()})RetainCreatedStorage(resource,nullptr);
    D3D12_RAYTRACING_GEOMETRY_DESC geometries[2]{};
    for(UINT i=0;i<2;++i){auto& g=geometries[i];g.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        g.Flags=i?D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE:D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        g.Triangles.IndexFormat=i?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16_UINT;g.Triangles.IndexCount=3;
        g.Triangles.IndexBuffer=(i?indices32:indices16)->GetGPUVirtualAddress();
        g.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;g.Triangles.VertexCount=3;
        g.Triangles.VertexBuffer={positions->GetGPUVirtualAddress()+UINT64(i)*3*sizeof(GIPoint),sizeof(GIPoint)};
    }
    auto buildAS=[&](const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs){
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO size{};gpu.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&size);
        Require(size.ResultDataMaxSizeInBytes&&size.ScratchDataSizeInBytes,"AS prebuild failed");
        auto result=gpu.Buffer(size.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,true);
        auto scratch=gpu.Buffer(size.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC b{};b.Inputs=inputs;b.DestAccelerationStructureData=result->GetGPUVirtualAddress();b.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
        gpu.list->BuildRaytracingAccelerationStructure(&b,0,nullptr);gpu.Uav(result.Get());return result;
    };
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=2;inputs.pGeometryDescs=geometries;auto blas=buildAS(inputs);
    D3D12_RAYTRACING_INSTANCE_DESC instance{};instance.Transform[0][0]=instance.Transform[1][1]=instance.Transform[2][2]=1;
    instance.InstanceID=321;instance.InstanceMask=1;instance.AccelerationStructure=blas->GetGPUVirtualAddress();
    auto instances=gpu.Upload(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>{instance});inputs={};inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=1;inputs.InstanceDescs=instances->GetGPUVirtualAddress();auto scene=buildAS(inputs);
    std::vector<uint> materials(3*84,0);const float one=1;std::memcpy(&materials[2*84+16],&one,4);
    auto materialBuffer=gpu.Upload(materials);gpu.View(materialBuffer.Get(),0,336,3,false);
    // All three material texture slots refer to an opaque white texture. Only
    // native vColorMultiplier.w differs: zero rejects, one accepts.
    D3D12_RESOURCE_DESC textureDesc{};textureDesc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;textureDesc.Width=textureDesc.Height=1;
    textureDesc.DepthOrArraySize=textureDesc.MipLevels=1;textureDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;textureDesc.SampleDesc.Count=1;
    D3D12_HEAP_PROPERTIES textureHeap{};textureHeap.Type=D3D12_HEAP_TYPE_DEFAULT;textureHeap.CreationNodeMask=textureHeap.VisibleNodeMask=1;
    ComPtr<ID3D12Resource> texture;Check(gpu.device->CreateCommittedResource(&textureHeap,D3D12_HEAP_FLAG_NONE,&textureDesc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&texture)));
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 textureBytes=0;gpu.device->GetCopyableFootprints(&textureDesc,0,1,0,&footprint,nullptr,nullptr,&textureBytes);
    auto texUpload=gpu.Upload(std::vector<unsigned char>(static_cast<std::size_t>(textureBytes),255));
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=texUpload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
    to.pResource=texture.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    gpu.Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    D3D12_SHADER_RESOURCE_VIEW_DESC texView{};texView.Format=textureDesc.Format;texView.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    texView.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;texView.Texture2D.MipLevels=1;
    for(UINT i=0;i<105;++i)gpu.device->CreateShaderResourceView(texture.Get(),&texView,cpu(16+i));
    constexpr UINT count=6;std::vector<GIProducerRay> proposals(count);
    for(UINT i=0;i<count;++i){auto& p=proposals[i];p.direction={0,0,1};p.baseRange=5;p.cosine=1;
        p.primaryBrdfCos={.318309886f,.318309886f,.318309886f};p.frameTag=19;p.sceneRevision=7;
        p.rayIndex=i;p.configuredCount=count;p.depthScale=1;p.selectionRandom=.5f;p.rangeRandom=.5f;p.proposalVerified=1;}
    proposals[1].origin.x=10;proposals[2].direction.z=-1;proposals[3].frameTag=18;
    proposals[4].cosine=2;proposals[5].baseRange=.5f;
    auto rays=gpu.Upload(proposals);gpu.View(rays.Get(),1,sizeof(GIProducerRay),count,false);
    D3D12_SHADER_RESOURCE_VIEW_DESC as{};as.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    as.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;as.RaytracingAccelerationStructure.Location=scene->GetGPUVirtualAddress();
    gpu.device->CreateShaderResourceView(nullptr,&as,cpu(2));
    PTShaderTableAdapter::Exports identifiers{};identifiers.nativeOpaque[0]=1;identifiers.nativeAlpha[0]=2;
    identifiers.privateOpaque=pipeline.Opaque();identifiers.privateAlpha=pipeline.Alpha();
    std::array<D3D12ProducerTable,2> tables;
    std::array<ComPtr<ID3D12Resource>,2> readbacks;
    std::vector<unsigned char> sentinel((count+1)*sizeof(GIProducerShadingRequest),0xcd);
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState1(pipeline.State());
    gpu.list->SetComputeRootDescriptorTable(0,handle(0));gpu.list->SetComputeRootDescriptorTable(1,handle(1));gpu.list->SetComputeRootDescriptorTable(3,handle(16));
    const uint controls[]={19,7,count,0};gpu.list->SetComputeRoot32BitConstants(4,4,controls,0);
    for(UINT mode=0;mode<2;++mode){
        std::vector<PTSceneAdmission::Geometry> mapped(2);std::vector<PTNativeBinding::Binding> bindings(2);
        for(UINT i=0;i<2;++i){auto& g=mapped[i];auto* ib=(i?indices32:indices16).Get();
            g.indices={{ib->GetGPUVirtualAddress(),ib->GetDesc().Width,1},ib->GetGPUVirtualAddress()};
            g.positions={{positions->GetGPUVirtualAddress(),positions->GetDesc().Width,1},positions->GetGPUVirtualAddress()+UINT64(i)*36};
            g.attributes={{attributes->GetGPUVirtualAddress(),attributes->GetDesc().Width,1},attributes->GetGPUVirtualAddress()};
            g.indexStride=i?4:2;g.indexCount=g.vertexCount=3;g.positionStride=12;g.attributeStride=16;g.normalOffset=4;g.uvOffset=12;
            g.material=i?1:(mode?2:0);g.materialCount=3;g.epoch=1;g.float3Positions=g.sharedVertexIndexing=g.noGeometryTransform=true;
            PTSceneAdmission::Record record;Require(PTSceneAdmission::Admit(g,record)==PTSceneAdmission::Error::none,"Geometry admission failed");
            auto& b=bindings[i];b.type=i?0:1;b.indices=record.indices;b.attributes=record.attributes;std::memcpy(b.constants,record.constants,40);
        }
        NativeGITable native;Require(BuildNativeGITable(1,mode+1,1,bindings,identifiers.nativeOpaque,identifiers.nativeAlpha,native),"Native table preparation failed");
        GIProducerTable plan;Require(PrepareGIProducerTable(native,1,mode+1,reinterpret_cast<std::uintptr_t>(pipeline.State()),1,identifiers,mapped,plan)==GIProducerTableError::None,"Private table preparation failed");
        Check(tables[mode].Initialize(gpu.device.Get(),pipeline.State(),plan,{positions.Get(),indices16.Get(),indices32.Get(),attributes.Get()}));
        auto output=gpu.Buffer(sentinel.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);auto initial=gpu.Upload(sentinel);
        gpu.list->CopyResource(output.Get(),initial.Get());gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gpu.list->SetComputeRootDescriptorTable(2,gpu.View(output.Get(),3+mode,sizeof(GIProducerShadingRequest),count+1,true));
        D3D12_DISPATCH_RAYS_DESC dispatch{};Require(pipeline.DispatchDescription(tables[mode].Range(),count,dispatch),"Dispatch preparation failed");gpu.list->DispatchRays(&dispatch);
        gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        readbacks[mode]=gpu.Buffer(sentinel.size(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);gpu.list->CopyResource(readbacks[mode].Get(),output.Get());
    }
    gpu.Submit();
    for(UINT mode=0;mode<2;++mode){GIProducerShadingRequest* out=nullptr;Check(readbacks[mode]->Map(0,nullptr,reinterpret_cast<void**>(&out)));
        const auto& h=out[0].hit;
        Require(out[0].producerVerified==1&&h.valid==1&&h.material==(mode?2u:1u),"Alpha cutout/acceptance or material mapping failed");
        Require(h.geometry==(mode?0u:1u)&&h.instanceId==321&&h.primitive==0,"Geometry/instance identity changed");
        Require(std::abs(h.rayT-(mode?1.f:2.f))<1e-5f&&std::abs(h.position.z-h.rayT)<1e-5f,"True hit position includes a native bias or wrong geometry");
        Require(std::abs(h.geometricNormal.z-1)<1e-5f&&std::abs(h.shadingNormal.z-1)<1e-5f,"Geometric/shading normal decode failed");
        Require(std::abs(h.texcoordU-1024.f/4095.f)<1e-5f&&std::abs(h.texcoordV-3071.f/4095.f)<1e-5f,"Packed UV decode failed");
        Require(std::abs(out[0].sourcePdf-.318309886f)<1e-6f&&std::abs(out[0].primaryBrdfCos.x/out[0].sourcePdf-1)<1e-5f,"Finite-range source density mismatch");
        for(UINT i:{1u,2u,5u})Require(out[i].producerVerified==1&&out[i].hit.material==65535,"Miss endpoint was treated as a surface");
        Require(out[2].missEndpoint.z==-5&&out[5].missEndpoint.z==.5f,"Miss endpoint or randomized range changed");
        for(UINT i:{3u,4u})Require(out[i].producerVerified==0,"Stale/malformed proposal admitted");
        Require(!std::memcmp(out+count,sentinel.data()+count*sizeof(*out),sizeof(*out)),"Output guard overwritten");
        readbacks[mode]->Unmap(0,nullptr);
    }
    if(messages)for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T size=0;Check(messages->GetMessage(i,nullptr,&size));std::vector<unsigned char> bytes(size);
        auto* m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(messages->GetMessage(i,m,&size));Require(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR,m->pDescription);}
    std::cout<<"PASS owned producer: real private DXR state/SBT, R16 cutout rejection and acceptance, R32 opaque hit, material/UV/normals/true position, finite-range PDF, misses, stale rejection, output guard and completion.\n";
    std::cout<<"This does not validate the live Control scene adapter or spatial/temporal reuse.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
