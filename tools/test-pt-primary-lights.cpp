#include <cstdlib>
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
using Microsoft::WRL::ComPtr;
struct Input {float position[3],choose;};
struct Result {float direction[3],radiance[3],target[3],pdf;UINT valid,active,directional;float radius,distance;UINT castsShadow;float effectiveRoughness,specular;};
static_assert(sizeof(Result)==72,"shader result stride");
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state,const void* data=nullptr,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE){
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=type;ComPtr<ID3D12Resource> result;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&result))));
    if(data){void* mapped=nullptr;D3D12_RANGE none{};assert(SUCCEEDED(result->Map(0,&none,&mapped)));std::memcpy(mapped,data,static_cast<size_t>(bytes));result->Unmap(0,nullptr);}return result;
}
int main(int argc,char** argv){
    const unsigned shadowFlags=argc>1?static_cast<unsigned>(std::strtoul(argv[1],nullptr,0)):0;
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    ComPtr<ID3DBlob> shader,error;
    assert(SetCurrentDirectoryW(L"src/shaders"));
    const HRESULT compiled=D3DCompileFromFile(L"../../tools/test-pt-primary-lights.hlsl",nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error);
    if(FAILED(compiled)&&error)std::fprintf(stderr,"%s",static_cast<const char*>(error->GetBufferPointer()));assert(SUCCEEDED(compiled));
    D3D12_DESCRIPTOR_RANGE ranges[7]{};const UINT registers[]={10,15,18,14,17,0};
    for(UINT n=0;n<6;++n)ranges[n]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,registers[n],n==5?4u:0u,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    ranges[6]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,4,D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    D3D12_ROOT_PARAMETER parameters[3]{};parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[0].DescriptorTable={6,ranges};
    parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[1].Descriptor={2,0};
    parameters[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[2].DescriptorTable={1,ranges+6};
    D3D12_STATIC_SAMPLER_DESC samplers[2]{};
    for(UINT n=0;n<2;++n){auto& s=samplers[n];s.ShaderRegister=n==0?8:11;s.RegisterSpace=1;
        s.Filter=n==0?D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        s.AddressU=s.AddressV=s.AddressW=n==0?D3D12_TEXTURE_ADDRESS_MODE_WRAP:D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxLOD=D3D12_FLOAT32_MAX;}
    D3D12_ROOT_SIGNATURE_DESC rootDesc{3,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> rootBytes;
    assert(SUCCEEDED(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1_0,&rootBytes,&error)));
    ComPtr<ID3D12RootSignature> root;assert(SUCCEEDED(device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root))));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};ComPtr<ID3D12PipelineState> pso;
    assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso))));
    constexpr float pi=3.14159265358979323846f;
    std::array<UINT,40> point{};std::array<UINT,80> spot{};std::array<UINT,144> sun{};
    const auto set=[](UINT& word,float value){std::memcpy(&word,&value,4);};
    set(point[2],2);set(point[4],5*pi);set(point[5],10*pi);set(point[6],15*pi);set(point[7],10);set(point[36],1);
    point[39]=4|(shadowFlags&2u);set(point[14],-1);set(point[10],.5);set(point[11],.25);
    set(spot[2],2);set(spot[12],1.0625f*pi);set(spot[13],2.125f*pi);set(spot[14],3.1875f*pi);set(spot[15],10);set(spot[18],1);set(spot[19],.25);
    for(UINT n=0;n<4;++n)set(spot[20+n*5],1);spot[76]=1|shadowFlags;set(spot[66],1);set(spot[67],1);
    set(sun[1],1);set(sun[4],2);set(sun[5],4);set(sun[6],6);sun[26]=1;set(sun[24],.5);set(sun[20],.5);set(sun[21],.5);
    std::array<UINT,256> constants{};constants[64]=constants[65]=constants[66]=1;
    constants[128+64]=2;constants[128+65]=constants[128+66]=1; // Counts exceed the actual point buffer in dispatch two.
    const Input inputs[]={{{0,0,0},.1f},{{0,0,.5f},.5f},{{0,0,0},.9f},{{2,0,.5f},.5f},{{0,0,0},1},{{0,0,2},.1f},{{std::numeric_limits<float>::quiet_NaN(),0,0},.1f}};
    constexpr UINT count=static_cast<UINT>(sizeof(inputs)/sizeof(inputs[0]));
    auto pointBuffer=Buffer(device.Get(),sizeof(point),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,point.data());
    auto spotBuffer=Buffer(device.Get(),sizeof(spot),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,spot.data());
    auto sunBuffer=Buffer(device.Get(),sizeof(sun),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,sun.data());
    auto cb=Buffer(device.Get(),sizeof(constants),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,constants.data());
    auto in=Buffer(device.Get(),sizeof(inputs),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,inputs);
    auto out=Buffer(device.Get(),2*count*sizeof(Result),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    auto readback=Buffer(device.Get(),2*count*sizeof(Result),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hd.NumDescriptors=8;
    ComPtr<ID3D12DescriptorHeap> heap;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))));
    const auto increment=device->GetDescriptorHandleIncrementSize(hd.Type);
    const auto cpu=[&](UINT n){auto h=heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(n)*increment;return h;};
    const auto gpu=[&](UINT n){auto h=heap->GetGPUDescriptorHandleForHeapStart();h.ptr+=UINT64(n)*increment;return h;};
    const auto srv=[&](UINT slot,ID3D12Resource* r,UINT stride,UINT elements){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Buffer.NumElements=elements;v.Buffer.StructureByteStride=stride;device->CreateShaderResourceView(r,&v,cpu(slot));};
    srv(0,pointBuffer.Get(),160,1);srv(1,spotBuffer.Get(),320,1);srv(2,sunBuffer.Get(),576,1);srv(5,in.Get(),sizeof(Input),count);
    for(UINT n=0;n<2;++n){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.FirstElement=n*count;v.Buffer.NumElements=count;v.Buffer.StructureByteStride=sizeof(Result);device->CreateUnorderedAccessView(out.Get(),nullptr,&v,cpu(6+n));}
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),pso.Get(),IID_PPV_ARGS(&list))));
    std::array<ComPtr<ID3D12Resource>,2> textures,uploads;
    for(UINT n=0;n<2;++n){
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=td.Height=1;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&textures[n]))));
        std::array<float,64> pixels{};pixels[0]=n==0?.2f:.25f;pixels[1]=.4f;pixels[2]=.6f;pixels[3]=1;
        uploads[n]=Buffer(device.Get(),sizeof(pixels),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,pixels.data());
        D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=textures[n].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource=uploads[n].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint.Footprint={td.Format,1,1,1,256};
        list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={textures[n].Get(),0,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
        D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=td.Format;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=1;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        device->CreateShaderResourceView(textures[n].Get(),&v,cpu(3+n));
    }
    ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetComputeRootDescriptorTable(0,gpu(0));
    for(UINT n=0;n<2;++n){list->SetComputeRootConstantBufferView(1,cb->GetGPUVirtualAddress()+n*512);list->SetComputeRootDescriptorTable(2,gpu(6+n));list->Dispatch(1,1,1);
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=out.Get();list->ResourceBarrier(1,&b);}
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={out.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
    list->CopyResource(readback.Get(),out.Get());assert(SUCCEEDED(list->Close()));ID3D12CommandList* submitted[]={list.Get()};queue->ExecuteCommandLists(1,submitted);
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));assert(SUCCEEDED(queue->Signal(fence.Get(),1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);assert(SUCCEEDED(fence->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    Result* result=nullptr;const D3D12_RANGE range{0,2*count*sizeof(Result)};assert(SUCCEEDED(readback->Map(0,&range,reinterpret_cast<void**>(&result))));
    const float expected[3][3]={{.5625f,1.125f,1.6875f},{.2f,.8f,1.8f},{1.25f,2.5f,3.75f}};
    for(UINT n=0;n<3;++n){assert(result[n].valid==1&&result[n].active==1);assert(std::abs(result[n].pdf-1.0f/3)<1e-6f);
        for(UINT c=0;c<3;++c){if(std::abs(result[n].radiance[c]-expected[n][c])>=1e-4f)std::fprintf(stderr,"light %u channel %u got %.9g expected %.9g\n",n,c,result[n].radiance[c],expected[n][c]);assert(std::abs(result[n].radiance[c]-expected[n][c])<1e-4f);}}
    assert(std::abs(result[0].direction[2]-1)<1e-5f&&std::abs(result[1].direction[2]-1)<1e-5f&&std::abs(result[2].direction[1]-1)<1e-5f&&result[2].directional==1);
    // Independent double-precision GGX result at a known off-axis direction.
    // Point distance=2, spot distance=1.5; both radius=1 and nDotL=1.
    for(UINT n=0;n<3;++n){
        const double distance=n==0?2.0:1.5,bArea=1/(1+3/(2*distance*distance));
        const double rough=std::sqrt(.05*.05+(1-bArea*bArea)/(6*bArea-2*bArea*bArea*bArea));
        double r=n<2?rough:.05,alpha=r*r,cosine=1/std::sqrt(1.16),halfCosine=std::sqrt((1+cosine)/2);
        double den=1-halfCosine*halfCosine+alpha*alpha*halfCosine*halfCosine;
        double D=alpha*alpha/(3.14159265358979323846*den*den);
        double mask=2*cosine/(cosine+std::sqrt(alpha*alpha+(1-alpha*alpha)*cosine*cosine));
        double F=.04+.96*std::pow(1-halfCosine,5),spec=D*mask*F/4;
        assert(std::abs(result[n].effectiveRoughness-r)<1e-5);
        assert(std::abs(result[n].specular-spec)<1e-5);
    }
    assert(result[0].radius==1&&std::abs(result[0].distance-2)<1e-6f);
    assert(result[1].radius==1&&std::abs(result[1].distance-1.5f)<1e-6f);
    assert(result[2].radius==0&&result[2].distance==0);
    assert(result[0].castsShadow==((shadowFlags&2u)!=0?1u:0u));
    const unsigned mode=shadowFlags&0x300u;
    assert(result[1].castsShadow==((shadowFlags&2u)!=0||mode==0x100u||mode==0x200u?1u:0u));
    assert(result[2].castsShadow==1);
    assert(result[3].valid==1&&result[3].active==0);assert(result[4].valid==0);assert(result[5].valid==1&&result[5].active==0);assert(result[6].valid==0);
    for(UINT n=count;n<2*count;++n)assert(result[n].valid==0&&result[n].active==0);
    const D3D12_RANGE empty{};readback->Unmap(0,&empty);assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    puts("PASS: primary b2 counts, point/cone attenuation, spot projection/range, sun projection, uniform PDF, radius/distance ABI, effective roughness, direct GGX highlight and invalid-data rejection on WARP");
}
