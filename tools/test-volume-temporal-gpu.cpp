#include "gpu_test_device.h"
#include "../src/visual_settings_policy.h"
#include <atomic>
#include <cstdarg>
namespace native_lighting {
inline constexpr GUID kKindGuid{0x72134610,0x3213,0x4321,{1,2,3,4,5,6,7,8}};
static void Trace(const char* format,...){va_list args;va_start(args,format);vprintf(format,args);va_end(args);puts("");}
using CreateCompute=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_COMPUTE_PIPELINE_STATE_DESC*,REFIID,void**);
#include "../src/character_surface_runtime.h"
#include "../src/volume_temporal_runtime.h"
}
using namespace native_lighting::volume_temporal;
static HRESULT STDMETHODCALLTYPE Create(ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* p,REFIID iid,void** out){return d->CreateComputePipelineState(p,iid,out);}
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
            float v=value+float(c)*.1f+(checker?(((x+y)%2)?1.f:-1.f):0.f);
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
int main(){try{
    Device gpu(false);
    for(unsigned p=0;p<12;++p)for(unsigned q=0;q<4;++q)
        Require(Enabled(p,q)==((p>=4 && p!=8) || (!p && q==3)),"History selection mismatch");
    D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,2,0,0,0}};
    D3D12_ROOT_PARAMETER params[4]{};
    for(UINT i=0;i<2;++i){
        params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;
        params[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i+2].DescriptorTable={1,&ranges[i]};
    }
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderRegister=6;sampler.RegisterSpace=1;sampler.MaxLOD=D3D12_FLOAT32_MAX;
    sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxAnisotropy=1;
    D3D12_ROOT_SIGNATURE_DESC rs{4,params,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&rs,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    ComPtr<ID3D12PipelineState> native,filtered;
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root.Get();
    pipeline.CS={kTemporalNative,sizeof(kTemporalNative)};
    Check(gpu.device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&native)));
    Observe(gpu.device.Get(),&pipeline,native.Get(),Create);
    Require(ready.load()==1,"Runtime history PSO not admitted");
    Observe(gpu.device.Get(),&pipeline,native.Get(),Create);
    Require(ready.load()==1,"Duplicate history PSO prepared");
    pipeline.CS={kTemporalNear,sizeof(kTemporalNear)};
    Check(gpu.device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&filtered)));
    struct Case {bool filter;float history,weight,threshold;bool outside,zoutside;};
    const Case cases[]={
        {false,2.1f,.9f,99,false,false}, // native near: no reuse
        {true,2.1f,.9f,99,false,false},  // near bounded reuse
        {true,2.1f,.5f,-1,false,false}, // far preserves native
        {true,2.1f,0,99,false,false},   // camera cut
        {true,2.1f,.9f,99,true,false},  // previous XY outside
        {true,4.f,.9f,99,false,false},  // large light change
        {true,0.f,.9f,99,false,false},  // extinguished history
        {true,2.1f,.9f,99,false,true},  // previous Z outside
        {true,2.f,.9f,99,false,false},  // constant field
        {true,2.1f,.1f,99,false,false}  // retain low engine weight
    };
    struct Result {ComPtr<ID3D12Resource> buffer;D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;UINT test,coefficient;};
    std::vector<Result> results;
    for(UINT i=0;i<10;++i){
        const auto c=cases[i];const UINT first=6*i;
        Texture(gpu,first,false,false,2.f);
        Texture(gpu,first+1,false,false,2.f);
        Texture(gpu,first+2,false,false,c.history);
        Texture(gpu,first+3,false,false,c.history);
        std::array<float,128> cb0{};
        const UINT dimensions[]={width,height,depth};
        std::memcpy(cb0.data()+18*4,dimensions,sizeof(dimensions));
        cb0[21*4]=1.f/width;cb0[21*4+1]=1.f/height;cb0[21*4+2]=1.f/depth;
        cb0[23*4]=1;cb0[23*4+1]=c.zoutside?100.f:1.f;cb0[23*4+2]=.1f;
        cb0[24*4]=.1f;cb0[24*4+1]=.1f;cb0[24*4+2]=2;
        std::array<float,64> cb1{};
        cb1[4*4]=c.outside?2.f:.5f;cb1[4*4+1]=.5f;cb1[4*4+3]=1;
        cb1[5*4]=c.weight;cb1[5*4+1]=c.threshold;
        auto a=gpu.Buffer(sizeof(cb0),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,cb0.data());
        auto b=gpu.Buffer(sizeof(cb1),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,cb1.data());
        Volume outputs[]={Texture(gpu,first+4,true,false,0),Texture(gpu,first+5,true,false,0)};
        auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();srv.ptr+=UINT64(first)*gpu.descriptorSize;
        auto uav=srv;uav.ptr+=4*gpu.descriptorSize;
        gpu.list->SetComputeRootSignature(root.Get());
        gpu.list->SetComputeRootConstantBufferView(0,a->GetGPUVirtualAddress());
        gpu.list->SetComputeRootConstantBufferView(1,b->GetGPUVirtualAddress());
        gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,uav);
        gpu.list->SetPipelineState(c.filter?filtered.Get():native.Get());gpu.list->Dispatch(2,2,1);
        for(UINT j=0;j<2;++j){
            const auto& out=outputs[j];
            gpu.Transition(out.resource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
            auto read=gpu.Buffer(out.bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
            D3D12_TEXTURE_COPY_LOCATION from{},to{};
            from.pResource=out.resource.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            to.pResource=read.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=out.layout;
            gpu.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
            results.push_back({read,out.layout,i,j});
        }
    }
    gpu.Submit();
    for(const auto& result:results){
        void* data=nullptr;Check(result.buffer->Map(0,nullptr,&data));
        for(UINT y=1;y<height-1;++y)for(UINT x=1;x<width-1;++x)for(UINT c=0;c<4;++c){
            const auto offset=result.layout.Offset+(height+y)*result.layout.Footprint.RowPitch+(4*x+c)*sizeof(float);
            float actual;std::memcpy(&actual,static_cast<unsigned char*>(data)+offset,sizeof(actual));
            float expected=2.f+.1f*c;
            if(result.test==1)expected+=.1f*.75f*(1.f-4.f*(.1f/2.1f));
            if(result.test==2)expected+=.05f;
            if(result.test==9)expected+=.1f*.1f*(1.f-4.f*(.1f/2.1f));
            if(result.coefficient==0 && c==3)expected=0;
            if(!std::isfinite(actual)||std::abs(actual-expected)>=.0001f){
                std::cerr<<"case="<<result.test<<" coefficient="<<result.coefficient<<" x="<<x<<" y="<<y<<" channel="<<c<<" actual="<<actual<<" expected="<<expected<<"\n";
                throw std::runtime_error("volume filter pixel mismatch");
            }
        }
        result.buffer->Unmap(0,nullptr);
    }
    std::cout<<"PASS ten real shader dispatch cases: near reuse, far preservation, reset, XY/Z rejection, light changes, constant energy, engine weight\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
