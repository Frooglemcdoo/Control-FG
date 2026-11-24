#include "gpu_test_device.h"
#include "rr_paired_fixture.h"
#include "../src/rr_paired_capture.h"
#include <filesystem>

namespace fs=std::filesystem;
static std::shared_ptr<rr_pair::Frame> Record(Device& g,Fixture& test){
    using namespace rr_pair;
    auto f=std::make_shared<Frame>();
    static_cast<FrameView&>(*f)=test.f;
    std::array<ComPtr<ID3D12Resource>,PlaneCount> textures{};
    UINT64 total=0;
    for(unsigned i=0;i<PlaneCount;++i){
        auto& grid=f->g[i];
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width=grid.w;d.Height=grid.h;d.DepthOrArraySize=d.SampleDesc.Count=d.MipLevels=1;
        d.Format=i==Motion?DXGI_FORMAT_R16G16_FLOAT:i==Normal?DXGI_FORMAT_R8G8B8A8_UNORM:
                 i==Depth?DXGI_FORMAT_R32_FLOAT:i==Remap?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16G16B16A16_FLOAT;
        grid.format=unsigned(d.Format);
        UINT rows=0;UINT64 rowBytes=0;
        const UINT64 base=(total+511)&~UINT64(511);
        g.device->GetCopyableFootprints(&d,0,1,base,&f->layouts[i],&rows,&rowBytes,nullptr);
        auto& layout=f->layouts[i];
        total=base+UINT64(rows-1)*layout.Footprint.RowPitch+rowBytes;
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        Check(g.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,IID_PPV_ARGS(&textures[i])));
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};
        auto sourceLayout=layout;sourceLayout.Offset=0;
        std::vector<unsigned char> uploadData(total-base);
        for(UINT y=0;y<rows;++y)std::memcpy(uploadData.data()+size_t(y)*layout.Footprint.RowPitch,
            grid.base+size_t(y)*grid.pitch,size_t(rowBytes));
        auto upload=g.Upload(uploadData);src.pResource=upload.Get();
        src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=sourceLayout;
        dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=textures[i].Get();
        g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        g.Transition(textures[i].Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        grid.pitch=layout.Footprint.RowPitch;grid.base=nullptr;
    }
    f->buffer=g.Buffer(total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    f->capacity=f->bytes=total;UINT64 commands=0;
    for(unsigned i=0;i<PlaneCount;++i)rr_pair::Copy(g.list.Get(),commands,*f,i,textures[i].Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Require(commands==3*PlaneCount,"GPU copy accounting");f->outputRecorded=true;return f;
}
int main() try{
    fs::path root=fs::absolute("build/paired-packet");
    fs::create_directories(root);Require(SetEnvironmentVariableW(L"LOCALAPPDATA",root.c_str())!=0,"Test output directory");
    Device gpu(false);rr_pair::Toggle();
    std::vector<std::shared_ptr<rr_pair::Frame>> frames;
    for(unsigned i=0;i<6;++i){
        Fixture fixture(100+i,3+i);
        if(i==1)for(unsigned ch=0;ch<3;++ch)fixture.Light(ch,4,true);
        frames.push_back(Record(gpu,fixture));
    }
    gpu.Submit();
    for(auto& frame:frames)rr_pair::CaptureReader().Submit(std::move(frame));
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(rr_pair::Armed()&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Require(!rr_pair::Armed()&&rr_pair::status.load()==std::wstring(L"Saved in ControlFGProbe"),"Fenced six-frame event");
    std::cout<<"PASS original D3D12 copies, motion match, independent channel peaks, atomic event and three follow-through frames\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
