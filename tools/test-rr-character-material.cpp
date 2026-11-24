#include "gpu_test_textures.h"
#include <DirectXPackedVector.h>
#include "../src/rr_character_material_dispatch.h"

int main() try{
    using Half=DirectX::PackedVector::HALF;
    using H4=std::array<Half,4>;
    const auto half=DirectX::PackedVector::XMConvertFloatToHalf;
    constexpr UINT w=27,h=19;
    constexpr DXGI_FORMAT format=DXGI_FORMAT_R16G16B16A16_FLOAT;
    const H4 raw={half(.00473785f),half(.00504684f),half(.00476074f),half(.375f)};
    const H4 filtered={half(.0014038f),half(.0014915f),half(.0014038f),half(1.f)};
    std::vector<H4> dst(w*h,raw),both(w*h,filtered),mask(w*h);
    std::vector<float> depth(w*h,.5f);
    // PackSurfaceWord for asuint(.5f) == 0x3f000000 (low=0, high=0x7e00).
    auto pack=[](unsigned x){return Half((0x400+(x&0x3fff))|((x&0x4000)<<1));};
    for(UINT y=2;y<17;++y)for(UINT x=2;x<25;++x)mask[y*w+x]={pack(0),pack(0x7e00),0,half(1)};
    depth[6*w+6]=.75f;mask[6*w+18][3]=half(.5f);
    both[12*w+12][1]=0x7e00;
    Device gpu(false);
    auto m=Texture(gpu,w,h,format,mask.data()),z=Texture(gpu,w,h,DXGI_FORMAT_R32_FLOAT,depth.data());
    auto b=Texture(gpu,w,h,format,both.data()),d=Texture(gpu,w,h,format,dst.data(),true);
    TextureView(gpu,m.Get(),0);TextureView(gpu,z.Get(),1);TextureView(gpu,b.Get(),2);TextureView(gpu,d.Get(),3,true);
    Require(RRCharacterMaterial::Prepare(gpu.device.Get()),"material pipeline");
    ID3D12DescriptorHeap* heaps[]={gpu.views.Get()};gpu.list->SetDescriptorHeaps(1,heaps);
    GICharacterMeasurement sample;
    Require(RRCharacterMaterial::measured && sample.Prepare(gpu.device.Get(),123),"GPU coverage counter setup");
    RRCharacterMaterial::Dispatch(gpu.list.Get(),gpu.views->GetGPUDescriptorHandleForHeapStart(),d.Get(),w,h,&sample);
    auto download=CopyOutput(gpu,d.Get());gpu.Submit();unsigned char* bytes=nullptr;
    Check(download.buffer->Map(0,nullptr,reinterpret_cast<void**>(&bytes)));
    UINT changed=0;
    for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x){
        int tx=x&~1,ty=y&~1;
        bool inside=tx>=4&&tx<=22&&ty>=4&&ty<=14;
        auto overlaps=[&](int px,int py){return tx-1<=px&&px<=tx+2&&ty-1<=py&&py<=ty+2;};
        inside=inside&&!overlaps(6,6)&&!overlaps(18,6)&&!(x==12&&y==12);
        const H4& expected=inside?filtered:raw;changed+=inside;
        Require(!std::memcmp(&expected,bytes+y*download.rowPitch+x*sizeof(H4),sizeof(H4)),"incorrect selected GI, boundary, or alpha");
    }
    GICharacterMeasurement::Sample stats{};
    Require(sample.Read(stats)&&stats.counts[0]==changed&&stats.counts[1]==changed,"real GPU counts differ from selected footprint");
    Require(changed>0&&changed<w*h,"empty selection");D3D12_RANGE empty{};download.buffer->Unmap(0,&empty);
    std::cout<<"R507 material GI WARP: "<<changed<<" interior pixels changed, outside RGBA preserved, selected RGBA matches Both\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
