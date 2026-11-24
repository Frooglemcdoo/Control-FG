#include "gpu_test_textures.h"
#include <DirectXPackedVector.h>
#include "../build/gi_motion_history.generated.h"
#include "../build/gi_spatial_history.generated.h"
#include "../src/rr_character_gi_dispatch.h"

// Separate processes exercise both native provider outcomes with the shipped GPU recorder.
int main(int argc,char** argv) try {
    const bool missing=argc==2 && std::strcmp(argv[1],"--missing-hit")==0;
    const bool originalBoth=argc==2&&std::strcmp(argv[1],"--original-both")==0;
    Require(argc==1 || missing || originalBoth,"Expected --missing-hit, --original-both or no argument");
    Device g(false);Require(RRCharacterSource::Prepare(g.device.Get()),"Both source pipelines");
    constexpr UINT w=32,h=16,n=w*h;constexpr auto format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    std::vector<Pixel> flat(n,{.00047f,.00043f,.00035f,.00047f}),flash=flat;
    flash[8*w+12]={.153931f,.120056f,.074219f,.153931f};
    std::vector<Pixel> geometry(n,{.5f,.5f,.5f,.5f}),zero(n,Pixel{}),distance(n,{1,1,1,1});
    auto surface=Texture(g,w,h,format,geometry.data()),velocity=Texture(g,w,h,format,zero.data());
    struct Hit {ID3D12Resource* resource=nullptr;D3D12_RESOURCE_DESC desc{};
                D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;};
    ComPtr<ID3D12Resource> hit;
    if(!missing)hit=Texture(g,w,h,format,distance.data());
    Hit selected;
    const bool useHit=RRCharacterSource::HitDistanceInput(w,h,selected,[&](Hit& t){
        if(!hit)return false;t.resource=hit.Get();t.desc=hit->GetDesc();return true;
    });
    Require(useHit==!missing,"Hit-distance mode did not match the native input");
    if(useHit) {
        auto admits=[&](Hit t){return RRCharacterSource::HitDistanceInput(w,h,t,[](Hit&){return true;});};
        Hit invalid=selected;invalid.state=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        Require(!admits(invalid),"Unready hit texture selected");
        invalid=selected;invalid.desc.Width+=1;Require(!admits(invalid),"Mismatched hit extent selected");
        invalid=selected;invalid.desc.Format=DXGI_FORMAT_R32_UINT;Require(!admits(invalid),"Integer hit view selected");
    }
    std::array<float,288> sys{};sys[0]=sys[4]=w;sys[1]=sys[5]=h;
    sys[2]=sys[6]=1.f/w;sys[3]=sys[7]=1.f/h;
    auto cb=g.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
    auto filter=g.Upload(std::vector<UINT>(64));
    ComPtr<ID3D12Resource> history;Readback samples[2],expectedTemporal[2],savedTemporal[2];
    for(UINT frame=0;frame<2;++frame){
        auto raw=Texture(g,w,h,format,frame?flash.data():flat.data());
        constexpr auto privateFormat=DXGI_FORMAT_R16G16B16A16_FLOAT;
        auto a=Texture(g,w,h,privateFormat,nullptr,true),b=Texture(g,w,h,privateFormat,nullptr,true);
        auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,write=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        g.Transition(a.Get(),write,read);g.Transition(b.Get(),write,read);
        ID3D12Resource* images[]={a.Get(),b.Get()};
        UINT offset=frame*27;
        ID3D12Resource* old=history?history.Get():raw.Get();
        ID3D12Resource* temporal[]={surface.Get(),surface.Get(),velocity.Get(),raw.Get(),old,raw.Get(),raw.Get(),raw.Get()};
        for(UINT j=0;j<8;++j)TextureView(g,temporal[j],offset+j);
        TextureView(g,a.Get(),offset+8,true);
        for(UINT i=0;i<2;++i){
            ID3D12Resource* spatial[]={surface.Get(),surface.Get(),surface.Get(),surface.Get(),velocity.Get(),images[i],old};
            for(UINT j=0;j<7;++j)TextureView(g,spatial[j],offset+9*(i+1)+j);
            auto hitHandle=g.views->GetCPUDescriptorHandleForHeapStart();
            hitHandle.ptr+=SIZE_T(offset+9*(i+1)+7)*g.descriptorSize;
            RRCharacterSource::HitView(g.device.Get(),hitHandle,useHit?selected.resource:nullptr,format);
            TextureView(g,images[1-i],offset+9*(i+1)+8,true);
        }
        auto gpu=g.views->GetGPUDescriptorHandleForHeapStart();gpu.ptr+=UINT64(offset)*g.descriptorSize;
        ComPtr<ID3D12Resource> snapshot;
        if(originalBoth){
            snapshot=Texture(g,w,h,privateFormat,nullptr,true);g.Transition(snapshot.Get(),write,read);
            RRCharacterSource::DispatchPasses(g.list.Get(),cb->GetGPUVirtualAddress(),filter->GetGPUVirtualAddress(),
                gpu,g.descriptorSize,images,w,h,0,frame!=0,true);
            g.Transition(a.Get(),read,write);expectedTemporal[frame]=CopyOutput(g,a.Get());
            g.Transition(a.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read);
        }
        RRCharacterSource::DispatchPasses(g.list.Get(),cb->GetGPUVirtualAddress(),filter->GetGPUVirtualAddress(),
            gpu,g.descriptorSize,images,w,h,6,frame!=0,useHit,snapshot.Get(),!originalBoth||frame==0?1u:0u);
        if(originalBoth){
            g.Transition(snapshot.Get(),read,write);savedTemporal[frame]=CopyOutput(g,snapshot.Get());
        }
        g.Transition(a.Get(),read,write);samples[frame]=CopyOutput(g,a.Get());
        g.Transition(a.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,read);history=a;
    }
    g.Submit();
    for(UINT f=0;f<2;++f){
        if(originalBoth){
            unsigned char *actual=nullptr,*expected=nullptr;
            Check(savedTemporal[f].buffer->Map(0,nullptr,reinterpret_cast<void**>(&actual)));
            Check(expectedTemporal[f].buffer->Map(0,nullptr,reinterpret_cast<void**>(&expected)));
            for(unsigned y=0;y<h;++y)Require(std::memcmp(actual+y*savedTemporal[f].rowPitch,
                expected+y*expectedTemporal[f].rowPitch,w*8)==0,"Temporal snapshot differs from independently dispatched temporal shader");
            D3D12_RANGE empty{};savedTemporal[f].buffer->Unmap(0,&empty);expectedTemporal[f].buffer->Unmap(0,&empty);
        }
        unsigned char* data=nullptr;Check(samples[f].buffer->Map(0,nullptr,reinterpret_cast<void**>(&data)));
        std::array<DirectX::PackedVector::HALF,4> channels{};
        std::memcpy(channels.data(),data+8*samples[f].rowPitch+12*sizeof(channels),sizeof(channels));
        Pixel value{};
        for(UINT k=0;k<4;++k)value[k]=DirectX::PackedVector::XMConvertHalfToFloat(channels[k]);
        for(UINT k=0;k<3;++k)Require(std::isfinite(value[k])&&value[k]>0&&
            (f?value[k]<flash[8*w+12][k]*.3f:std::abs(value[k]-flat[0][k])<1e-6f),"Uniform energy or captured flash");
        std::cout<<(originalBoth?"original Both":useHit?"hit-aware":"no-hit geometry")<<" source frame "<<f<<" raw red "<<(f?flash[8*w+12][0]:flat[0][0])<<" Both GI red "<<value[0]<<"\n";
        if(missing && f==1){
            // The separate RR composition fixture consumes exactly this GPU result.
            std::ofstream output("build/character-no-hit-gpu-sample.bin",std::ios::binary);
            output.write(reinterpret_cast<const char*>(value.data()),sizeof(value));
            output.close();Require(bool(output),"Saving private GI readback");
        }
        D3D12_RANGE empty{};samples[f].buffer->Unmap(0,&empty);
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
