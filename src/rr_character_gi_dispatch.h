#pragma once
// D3D12 recording shared by the renderer and the off-screen WARP regression.
namespace RRCharacterSource {
using Microsoft::WRL::ComPtr;
constexpr UINT MaxPasses=6;
static ComPtr<ID3D12RootSignature> root;
static ComPtr<ID3D12PipelineState> pipelines[6];
static ComPtr<ID3D12Resource> constants;
static DXGI_FORMAT ReadFormat(DXGI_FORMAT f){
    if(f==DXGI_FORMAT_R32_TYPELESS || f==DXGI_FORMAT_D32_FLOAT)return DXGI_FORMAT_R32_FLOAT;
    if(f==DXGI_FORMAT_R16_TYPELESS || f==DXGI_FORMAT_D16_UNORM)return DXGI_FORMAT_R16_UNORM;
    switch(f){
    case DXGI_FORMAT_R16G16B16A16_FLOAT:case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:case DXGI_FORMAT_R16G16_FLOAT:case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R32_FLOAT:case DXGI_FORMAT_R16_FLOAT:case DXGI_FORMAT_R16_UNORM:return f;
    default:return DXGI_FORMAT_UNKNOWN;
    }
}
// A rejected optional provider must not stop the six required GI inputs.
template<class Texture,class Lookup>
static bool HitDistanceInput(UINT w,UINT h,Texture& t,Lookup lookup){
    if(!lookup(t))return false;
    const auto& d=t.desc;
    return t.resource && d.Width==w && d.Height==h && d.MipLevels &&
        (t.state&(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) &&
        !(d.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) && ReadFormat(d.Format)!=DXGI_FORMAT_UNKNOWN;
}
static void HitView(ID3D12Device* device,D3D12_CPU_DESCRIPTOR_HANDLE handle,
                    ID3D12Resource* input,DXGI_FORMAT viewFormat){
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    srv.Format=input?viewFormat:DXGI_FORMAT_R32_FLOAT;
    device->CreateShaderResourceView(input,&srv,handle);
}
static bool Prepare(ID3D12Device* device){
    if(root)return true;
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,8,0,0,0},
                                     {D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,8}};
    D3D12_ROOT_PARAMETER p[3]{};
    for(UINT i=0;i<2;++i){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor.ShaderRegister=i;}
    p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[2].DescriptorTable={2,ranges};
    D3D12_STATIC_SAMPLER_DESC samplers[2]{};
    for(UINT i=0;i<2;++i){
        auto& s=samplers[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxLOD=D3D12_FLOAT32_MAX;
        s.ShaderRegister=i?11:6;s.RegisterSpace=1;
    }
    D3D12_ROOT_SIGNATURE_DESC desc{3,p,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;ComPtr<ID3D12RootSignature> signature;
    if(FAILED(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error))||
       FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&signature))))return false;
    D3D12_SHADER_BYTECODE shaders[]={{kGINoHistoryShader,sizeof(kGINoHistoryShader)},
        {kGIMotionHistoryShader,sizeof(kGIMotionHistoryShader)},
        {kGICharacterSteady0,sizeof(kGICharacterSteady0)},{kGICharacterSteady1,sizeof(kGICharacterSteady1)},
        {kGICharacterGeometry0,sizeof(kGICharacterGeometry0)},{kGICharacterGeometry1,sizeof(kGICharacterGeometry1)}};
    for(UINT i=0;i<_countof(shaders);++i){
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=signature.Get();pd.CS=shaders[i];
        if(FAILED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(pipelines[i].ReleaseAndGetAddressOf()))))return false;
    }
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width=256*MaxPasses*MaxPasses;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=buffer.SampleDesc.Count=1;
    buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,IID_PPV_ARGS(constants.ReleaseAndGetAddressOf()))))return false;
    UINT* data=nullptr;D3D12_RANGE empty{};
    if(FAILED(constants->Map(0,&empty,reinterpret_cast<void**>(&data))))return false;
    std::memset(data,0,SIZE_T(buffer.Width));
    for(UINT n=1;n<=MaxPasses;++n)for(UINT i=0;i<n;++i){
        auto* pass=data+64*((n-1)*MaxPasses+i);pass[4]=i;pass[5]=n;pass[6]=1;
    }
    constants->Unmap(0,nullptr);root=std::move(signature);return true;
}
static void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);
}
static void DispatchPasses(ID3D12GraphicsCommandList* list,UINT64 cb0,UINT64 cb1,
    D3D12_GPU_DESCRIPTOR_HANDLE gpu,UINT step,ID3D12Resource* const images[2],
    UINT w,UINT h,UINT passes,bool history,bool useHitDistance){
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    constexpr auto write=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    list->SetComputeRootSignature(root.Get());list->SetComputeRootConstantBufferView(0,cb0);
    list->SetComputeRootConstantBufferView(1,cb1);
    for(UINT i=0;i<=passes;++i){
        const UINT out=i&1;
        const UINT table=i?1+((i-1)&1):0;
        const UINT spatialBase=useHitDistance?2:4;
        list->SetPipelineState(pipelines[i?spatialBase+((i-1)&1):unsigned(history)].Get());
        if(i)list->SetComputeRootConstantBufferView(1,constants->GetGPUVirtualAddress()+256*((passes-1)*MaxPasses+i-1));
        list->SetComputeRootDescriptorTable(2,{gpu.ptr+UINT64(9*table)*step});
        Barrier(list,images[out],read,write);list->Dispatch((w+7)/8,(h+7)/8,1);
        Barrier(list,images[out],write,read);
    }
 }
}
