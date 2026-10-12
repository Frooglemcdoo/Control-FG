#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <climits>
using Microsoft::WRL::ComPtr;
static void ok(HRESULT hr,const char* name){if(FAILED(hr)){std::printf("FAIL %s HRESULT=0x%08lx\n",name,(unsigned long)hr);throw std::runtime_error(name);}}
static void need(bool b,const char* name){if(!b)throw std::runtime_error(name);}
struct Ray {float origin[3],tmin,direction[3],tmax;};
struct Result {float distance,uv[2],normal[3];UINT material,status,visibility;};
static_assert(sizeof(Ray)==32&&sizeof(Result)==36,"Shader ABI");
struct Vertex {short normal[4];short uv[2];};
static_assert(sizeof(Vertex)==12,"Packed native-style fixture");
struct GPU {
    ComPtr<ID3D12Device5> device;ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList4> list;
    std::vector<ComPtr<ID3D12Resource>> keep;
    ComPtr<ID3D12Resource> buffer(UINT64 size,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE,const void* data=nullptr){
        D3D12_HEAP_PROPERTIES hp{};hp.Type=heap;D3D12_RESOURCE_DESC d{};
        d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;
        ComPtr<ID3D12Resource> r;ok(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)),"Create buffer");
        if(data){void* p;D3D12_RANGE empty{0,0};ok(r->Map(0,&empty,&p),"Map upload");memcpy(p,data,(size_t)size);r->Unmap(0,nullptr);}keep.push_back(r);return r;
    }
    void barrier(ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){D3D12_RESOURCE_BARRIER x{};x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;x.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};list->ResourceBarrier(1,&x);}
    void uav(ID3D12Resource* r){D3D12_RESOURCE_BARRIER x{};x.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;x.UAV.pResource=r;list->ResourceBarrier(1,&x);}
    ComPtr<ID3D12Resource> acceleration(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs){
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs,&info);
        need(info.ResultDataMaxSizeInBytes>0,"AS prebuild size");
        auto result=buffer(info.ResultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        auto scratch=buffer(info.ScratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc{};desc.Inputs=inputs;desc.DestAccelerationStructureData=result->GetGPUVirtualAddress();desc.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
        list->BuildRaytracingAccelerationStructure(&desc,0,nullptr);uav(result.Get());return result;
    }
    ComPtr<ID3D12Resource> texture(float alpha){
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=1;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;
        ComPtr<ID3D12Resource> t;ok(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&t)),"Create alpha texture");keep.push_back(t);
        std::array<unsigned char,256> bytes{};float rgba[4]={1,1,1,alpha};memcpy(bytes.data(),rgba,sizeof(rgba));auto upload=buffer(bytes.size(),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,bytes.data());
        D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=t.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint.Footprint={d.Format,1,1,1,256};
        list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);barrier(t.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);return t;
    }
    void execute(){
        ok(list->Close(),"Close");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
        ComPtr<ID3D12Fence> f;ok(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&f)),"Fence");ok(queue->Signal(f.Get(),1),"Signal");
        HANDLE e=CreateEventW(nullptr,FALSE,FALSE,nullptr);need(e!=nullptr,"Fence event");HRESULT hr=f->SetEventOnCompletion(1,e);
        if(FAILED(hr)){CloseHandle(e);ok(hr,"Set fence event");}
        DWORD wait=WaitForSingleObject(e,30000);CloseHandle(e);
        need(wait==WAIT_OBJECT_0,"GPU timeout (30 seconds)");ok(device->GetDeviceRemovedReason(),"GPU device health");
    }
};
static ComPtr<ID3D12RootSignature> root(ID3D12Device* d,D3D12_ROOT_SIGNATURE_DESC desc){
    ComPtr<ID3DBlob> b,e;HRESULT hr=D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&b,&e);
    if(e)std::printf("Root signature: %s\n",(char*)e->GetBufferPointer());ok(hr,"Serialize root");
    ComPtr<ID3D12RootSignature> r;ok(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&r)),"Create root");return r;
}
int main(int argc,char** argv){try{
    const bool allowSkip=argc>1&&strcmp(argv[1],"--allow-no-dxr")==0;
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))){debug->EnableDebugLayer();std::puts("D3D12 debug layer enabled");}
    ComPtr<IDXGIFactory6> factory;ok(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");GPU g;
    for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> a;if(factory->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&a))==DXGI_ERROR_NOT_FOUND)break;
        DXGI_ADAPTER_DESC1 desc{};a->GetDesc1(&desc);if(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)continue;
        ComPtr<ID3D12Device5> d;if(FAILED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&d))))continue;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};if(FAILED(d->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&options,sizeof(options)))||options.RaytracingTier==D3D12_RAYTRACING_TIER_NOT_SUPPORTED)continue;
        g.device=d;std::printf("GPU: %ls, DXR tier %u\n",desc.Description,(UINT)options.RaytracingTier);break;
    }
    if(!g.device){std::puts("SKIP: No hardware DXR adapter. GPU tests did NOT run.");return allowSkip?0:2;}
    std::ifstream file("pt-scene-query-test.dxil",std::ios::binary);need(bool(file),"Missing pt-scene-query-test.dxil; run beside the shader");
    std::vector<char> bytecode((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());need(!bytecode.empty(),"Empty shader library");
    D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;ok(g.device->CreateCommandQueue(&q,IID_PPV_ARGS(&g.queue)),"Queue");
    ok(g.device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&g.allocator)),"Allocator");ok(g.device->CreateCommandList(0,q.Type,g.allocator.Get(),nullptr,IID_PPV_ARGS(&g.list)),"Command list");
    float positions[12][3]{};Vertex attributes[12]{};UINT indices32[12]{};unsigned short indices16[12]{};
    for(UINT i=0;i<4;++i){float x=3.0f*i;float p[3][3]={{x-1,-1,2},{x+1,-1,2},{x,1,2}};
        for(UINT j=0;j<3;++j){UINT n=i*3+j;memcpy(positions[n],p[j],12);indices32[n]=n;indices16[n]=(unsigned short)n;attributes[n].normal[2]=-32767;attributes[n].uv[0]=j==1?4095:0;attributes[n].uv[1]=j==2?4095:0;}}
    auto pos=g.buffer(sizeof(positions),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,positions);
    auto verts=g.buffer(sizeof(attributes),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,attributes);
    auto idx32=g.buffer(sizeof(indices32),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,indices32);
    auto idx16=g.buffer(sizeof(indices16),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,indices16);
    D3D12_RAYTRACING_GEOMETRY_DESC geometry[4]{};
    for(UINT i=0;i<4;++i){auto& d=geometry[i];d.Type=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;d.Flags=i==0?D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE:D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        d.Triangles.IndexFormat=DXGI_FORMAT_R32_UINT;d.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;d.Triangles.IndexCount=3;d.Triangles.VertexCount=12;
        d.Triangles.IndexBuffer=idx32->GetGPUVirtualAddress()+i*12;d.Triangles.VertexBuffer={pos->GetGPUVirtualAddress(),12};}
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=4;inputs.pGeometryDescs=geometry;
    auto blas=g.acceleration(inputs);
    D3D12_RAYTRACING_INSTANCE_DESC instance{};instance.Transform[0][0]=instance.Transform[1][1]=instance.Transform[2][2]=1;instance.InstanceMask=1;instance.AccelerationStructure=blas->GetGPUVirtualAddress();
    auto inst=g.buffer(sizeof(instance),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,&instance);
    inputs={};inputs.Type=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;inputs.DescsLayout=D3D12_ELEMENTS_LAYOUT_ARRAY;inputs.NumDescs=1;inputs.InstanceDescs=inst->GetGPUVirtualAddress();auto tlas=g.acceleration(inputs);
    UINT material[3][84]{};float one=1;for(auto& m:material)memcpy(&m[16],&one,4);
    auto materials=g.buffer(sizeof(material),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,material);
    auto solid=g.texture(1),hole=g.texture(0);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=106;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;ok(g.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)),"Descriptor heap");UINT step=g.device->GetDescriptorHandleIncrementSize(hd.Type);
    auto cpu=heap->GetCPUDescriptorHandleForHeapStart();D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Buffer.NumElements=3;srv.Buffer.StructureByteStride=336;
    g.device->CreateShaderResourceView(materials.Get(),&srv,cpu);srv={};srv.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    for(UINT i=0;i<105;++i){cpu.ptr+=step;g.device->CreateShaderResourceView(i==35?hole.Get():solid.Get(),&srv,cpu);}
    D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,3,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,UINT_MAX,0,1,0}};
    D3D12_ROOT_PARAMETER rp[5]{};for(UINT i=0;i<2;++i){rp[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;rp[i].DescriptorTable={1,&ranges[i]};}
    rp[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;rp[2].Descriptor={5,0};rp[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;rp[3].Descriptor={0,4};rp[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;rp[4].Descriptor={0,4};
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderRegister=5;sampler.RegisterSpace=1;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=5;rd.pParameters=rp;rd.NumStaticSamplers=1;rd.pStaticSamplers=&sampler;auto global=root(g.device.Get(),rd);
    D3D12_ROOT_PARAMETER localParams[3]{};localParams[0].ParameterType=localParams[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;localParams[0].Descriptor={0,3};localParams[1].Descriptor={1,3};localParams[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;localParams[2].Constants={0,3,10};
    rd={};rd.NumParameters=3;rd.pParameters=localParams;rd.Flags=D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;auto local=root(g.device.Get(),rd);
    const wchar_t* groupNames[]={L"SurfaceOpaque",L"ShadowOpaque",L"SurfaceAlpha",L"ShadowAlpha"};D3D12_HIT_GROUP_DESC groups[4]{};
    for(UINT i=0;i<4;++i){groups[i].HitGroupExport=groupNames[i];groups[i].Type=D3D12_HIT_GROUP_TYPE_TRIANGLES;groups[i].ClosestHitShaderImport=(i%2)==0?L"PTSceneClosestHit":L"PTVisibilityClosestHit";if(i>=2)groups[i].AnyHitShaderImport=(i%2)==0?L"PTSceneAlphaHit":L"PTVisibilityAlphaHit";}
    D3D12_DXIL_LIBRARY_DESC library{};library.DXILLibrary={bytecode.data(),bytecode.size()};D3D12_RAYTRACING_SHADER_CONFIG config{32,8};D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};
    ID3D12RootSignature* globalPtr=global.Get();ID3D12RootSignature* localPtr=local.Get();std::vector<D3D12_STATE_SUBOBJECT> objects;objects.reserve(12);
    objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library});for(auto& group:groups)objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&group});
    objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config});objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&globalPtr});
    objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&localPtr});D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{&objects.back(),4,groupNames};objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&association});objects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline});
    D3D12_STATE_OBJECT_DESC stateDesc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,(UINT)objects.size(),objects.data()};ComPtr<ID3D12StateObject> state;ok(g.device->CreateStateObject(&stateDesc,IID_PPV_ARGS(&state)),"Create private DXR state object");
    ComPtr<ID3D12StateObjectProperties> properties;ok(state.As(&properties),"State object properties");
    std::array<unsigned char,960> records{};
    auto identifier=[&](UINT offset,const wchar_t* name){const void* id=properties->GetShaderIdentifier(name);need(id!=nullptr,"Missing shader identifier");memcpy(records.data()+offset,id,32);};
    identifier(0,L"PTSceneQueryRayGeneration");identifier(64,L"PTSceneMiss");identifier(96,L"PTVisibilityMiss");
    for(UINT i=0;i<4;++i)for(UINT type=0;type<2;++type){UINT offset=192+(2*i+type)*96;identifier(offset,groupNames[(i==0?0:2)+type]);
        bool shortIndex=i==0;UINT64 addr=(shortIndex?idx16:idx32)->GetGPUVirtualAddress();memcpy(records.data()+offset+32,&addr,8);addr=verts->GetGPUVirtualAddress();memcpy(records.data()+offset+40,&addr,8);
        UINT constants[10]={i*3*(shortIndex?2u:4u),shortIndex?2u:4u,12,i==3?99u:i,0,0,8,0,shortIndex?(UINT)sizeof(indices16):(UINT)sizeof(indices32),(UINT)sizeof(attributes)};
        memcpy(records.data()+offset+48,constants,sizeof(constants));}
    auto sbt=g.buffer(records.size(),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,records.data());
    Ray rays[]={{{0,0,0},0,{0,0,1},10},{{12,0,0},0,{0,0,1},10},{{3,0,0},0,{0,0,1},10},{{6,0,0},0,{0,0,1},10},{{0,0,0},0,{0,0,1},1},{{0,0,0},2.1f,{0,0,1},10},{{0,0,0},0,{0,0,0},10},{{9,0,0},0,{0,0,1},10}};
    auto rayBuffer=g.buffer(sizeof(rays),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,rays);auto output=g.buffer(8*sizeof(Result),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);auto readback=g.buffer(8*sizeof(Result),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    ID3D12DescriptorHeap* heaps[]={heap.Get()};g.list->SetDescriptorHeaps(1,heaps);g.list->SetComputeRootSignature(global.Get());auto gpu=heap->GetGPUDescriptorHandleForHeapStart();g.list->SetComputeRootDescriptorTable(0,gpu);gpu.ptr+=step;g.list->SetComputeRootDescriptorTable(1,gpu);
    g.list->SetComputeRootShaderResourceView(2,tlas->GetGPUVirtualAddress());g.list->SetComputeRootShaderResourceView(3,rayBuffer->GetGPUVirtualAddress());g.list->SetComputeRootUnorderedAccessView(4,output->GetGPUVirtualAddress());g.list->SetPipelineState1(state.Get());
    auto base=sbt->GetGPUVirtualAddress();D3D12_DISPATCH_RAYS_DESC dispatch{};dispatch.RayGenerationShaderRecord={base,32};dispatch.MissShaderTable={base+64,64,32};dispatch.HitGroupTable={base+192,8*96,96};dispatch.Width=8;dispatch.Height=dispatch.Depth=1;g.list->DispatchRays(&dispatch);
    g.barrier(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);g.list->CopyBufferRegion(readback.Get(),0,output.Get(),0,8*sizeof(Result));g.execute();
    Result* results;D3D12_RANGE range{0,8*sizeof(Result)};ok(readback->Map(0,&range,(void**)&results),"Read GPU results");
    const char* names[]={"opaque hit 16-bit","geometric miss","alpha cutout 32-bit","solid alpha 32-bit","short shadow segment","near clip","invalid ray","invalid material"};UINT expected[]={1,0,0,1,0,0,2,2};UINT vis[]={0,1,1,0,1,1,2,2};bool pass=true;
    for(UINT i=0;i<8;++i){auto& r=results[i];bool match=r.status==expected[i]&&r.visibility==vis[i];if(r.status==1)match=match&&std::fabs(r.distance-2)<1e-4f&&std::fabs(r.normal[2]+1)<1e-4f&&std::fabs(r.uv[0]-.25f)<1e-4f&&std::fabs(r.uv[1]-.5f)<1e-4f;
        std::printf("%s: %s hit=%u visibility=%u distance=%.6f material=%u uv=(%.6f,%.6f) normal=(%.4f,%.4f,%.4f)\n",match?"PASS":"FAIL",names[i],r.status,r.visibility,r.distance,r.material,r.uv[0],r.uv[1],r.normal[0],r.normal[1],r.normal[2]);pass=pass&&match;}
    D3D12_RANGE noWrite{0,0};readback->Unmap(0,&noWrite);
    ComPtr<ID3D12InfoQueue> messages;
    if(SUCCEEDED(g.device.As(&messages)))for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T size=0;messages->GetMessage(i,nullptr,&size);std::vector<unsigned char> bytes(size);auto m=(D3D12_MESSAGE*)bytes.data();
        if(SUCCEEDED(messages->GetMessage(i,m,&size))&&m->Severity<=D3D12_MESSAGE_SEVERITY_ERROR){std::printf("D3D12 ERROR: %s\n",m->pDescription);pass=false;}}
    need(pass,"GPU result or D3D12 validation mismatch");std::puts("PASS: all 8 private DXR GPU cases. Control integration is NOT tested.");return 0;
}catch(const std::exception& e){std::printf("FAIL: %s\n",e.what());return 1;}}
