#include "gpu_test_textures.h"
#include "../build/gi_motion_history.generated.h"
#include "../build/gi_spatial_history.generated.h"
#include "../src/rr_character_gi_dispatch.h"

int main() try {
    Device g(false);Require(RRCharacterSource::Prepare(g.device.Get()),"Both source pipelines");
    constexpr UINT w=32,h=16,n=w*h;constexpr auto format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    std::vector<Pixel> flat(n,{.00047f,.00043f,.00035f,.00047f}),flash=flat;
    flash[8*w+12]={.153931f,.120056f,.074219f,.153931f};
    std::vector<Pixel> geometry(n,{.5f,.5f,.5f,.5f}),zero(n,Pixel{}),distance(n,{1,1,1,1});
    auto surface=Texture(g,w,h,format,geometry.data()),velocity=Texture(g,w,h,format,zero.data());
    auto hit=Texture(g,w,h,format,distance.data());
    std::array<float,288> sys{};sys[0]=sys[4]=w;sys[1]=sys[5]=h;
    sys[2]=sys[6]=1.f/w;sys[3]=sys[7]=1.f/h;
    auto cb=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
    auto filter=g.Upload(std::vector<UINT>(64));
    ComPtr<ID3D12Resource> history;Readback samples[2];
    for(UINT frame=0;frame<2;++frame){
        auto raw=Texture(g,w,h,format,frame?flash.data():flat.data());
        ComPtr<ID3D12Resource> a=Texture(g,w,h,format,nullptr,true),b=Texture(g,w,h,format,nullptr,true);
        auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,write=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        g.Transition(a.Get(),write,read);g.Transition(b.Get(),write,read);
        ID3D12Resource* images[]={a.Get(),b.Get()};
        UINT offset=frame*27;
        ID3D12Resource* old=history?history.Get():raw.Get();
        ID3D12Resource* temporal[]={surface.Get(),surface.Get(),velocity.Get(),raw.Get(),old,raw.Get(),raw.Get(),raw.Get()};
        for(UINT j=0;j<8;++j)TextureView(g,temporal[j],offset+j);
        TextureView(g,a.Get(),offset+8,true);
        for(UINT i=0;i<2;++i){
            ID3D12Resource* spatial[]={surface.Get(),surface.Get(),surface.Get(),surface.Get(),velocity.Get(),images[i],old,hit.Get()};
            for(UINT j=0;j<8;++j)TextureView(g,spatial[j],offset+9*(i+1)+j);
            TextureView(g,images[1-i],offset+9*(i+1)+8,true);
        }
        auto gpu=g.views->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(offset)*g.descriptorSize;
        RRCharacterSource::DispatchPasses(g.list.Get(),cb->GetGPUVirtualAddress(),filter->GetGPUVirtualAddress(),
            gpu,g.descriptorSize,images,w,h,6,frame!=0);
        g.Transition(a.Get(),read,write);samples[frame]=CopyOutput(g,a.Get());
        g.Transition(a.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read);history=a;
    }
    g.Submit();
    for(UINT f=0;f<2;++f){
        unsigned char* data=nullptr;Check(samples[f].buffer->Map(0,nullptr,reinterpret_cast<void**>(&data)));
        Pixel value{};std::memcpy(&value,data+8*samples[f].rowPitch+12*sizeof(Pixel),sizeof(Pixel));
        for(UINT k=0;k<3;++k)Require(std::isfinite(value[k])&&value[k]>0&&
            (f?value[k]<flash[8*w+12][k]*.3f:std::abs(value[k]-flat[0][k])<1e-6f),"Uniform energy or captured flash");
        std::cout<<"source frame "<<f<<" raw red "<<(f?flash[8*w+12][0]:flat[0][0])<<" Both GI red "<<value[0]<<"\n";
        D3D12_RANGE empty{};samples[f].buffer->Unmap(0,&empty);
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
