// Exercise production shader bindings with a real previous-frame UAV written by
// the preceding dispatch. Motion, jitter, and disocclusion are independent inputs.
#include "../src/rr_character_root.h"
#include "../src/rr_character_history.h"
static float MaskWord(UINT value) {
    UINT h=0x400+(value&0x3fff);
    float f=std::ldexp(1.f+(h&1023)/1024.f,int(h>>10)-15);
    return (value&0x4000)?-f:f;
}
static Pixel TestMask(float z,UINT tag,float coverage=1) {
    UINT bits;std::memcpy(&bits,&z,4);
    return {MaskWord(bits&32767),MaskWord(bits>>15),MaskWord(tag),coverage};
}
static void EmptyHistoryViews(Device& gpu,UINT first) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Texture2D.MipLevels=1;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    auto h=gpu.views->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(first)*gpu.descriptorSize;
    for(auto fmt:{DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32G32B32A32_UINT}){
        srv.Format=fmt;gpu.device->CreateShaderResourceView(nullptr,&srv,h);h.ptr+=gpu.descriptorSize;
    }
    D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;u.Format=srv.Format;
    gpu.device->CreateUnorderedAccessView(nullptr,nullptr,&u,h);
}

static void TestMotionHistory(bool sample,bool previousBuild=false) {
    Device gpu(false);
    ComPtr<ID3DBlob> blob,error;Check(RRCharacterSerializeRoot(&blob,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto code=Read(previousBuild?"build/gi-character-r501.dxil":sample?"build/gi-character-measured.dxil":"build/gi-character-blend.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
    ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    constexpr UINT w=20,h=84;
    const Pixel darkGI={.000468f,.000434f,.000351f,1}, darkScene={.000102f,.000041f,.000024f,.75f};
    const Pixel flashGI={.153931f,.120056f,.074219f,1},flashScene={.036713f,.015060f,.004642f,.75f};
    const auto format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    auto histA=Texture(gpu,w,h,DXGI_FORMAT_R32G32B32A32_UINT,nullptr,true);
    auto histB=Texture(gpu,w,h,DXGI_FORMAT_R32G32B32A32_UINT,nullptr,true);
    gpu.Transition(histB.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Readback read[3];GICharacterMeasurement stats[3];std::vector<Pixel> expected[3];
    // One-pixel-wide character strips have only two current matching neighbors.
    // The R500 spatial correction alone cannot modify their centers.
    for(UINT frame=0;frame<3;++frame){
        std::vector<Pixel> gi(w*h,darkGI),scene(w*h,darkScene),mask(w*h,{0,0,0,0}),motion(w*h,Pixel{}),
            depth(w*h,{.5f,0,0,0}),normal(w*h,{.5f,.5f,0,0}),mat(w*h,{0,0,0,0});
        UINT x=frame==0?5:9;
        for(UINT kind=0;kind<20;++kind){
            UINT y=kind*4+1,i=y*w+x;
            for(UINT j=i-1;j<=i+1;++j) {
                mask[j]=TestMask(.5f,kind+1);
                // Jitter delta -0.8 plus -3.2 pixels of motion maps 9 to 5.
                motion[j][0]=frame==1?-3.2f/w:0;
                if(frame==0){
                    if(kind==2)mask[j][3]=0; // newly revealed
                    if(kind==3){depth[j][0]=.42f;mask[j]=TestMask(.42f,kind+1);}
                    if(kind==4)normal[j]={0,0,0,0};
                    if(kind==5)mat[j][2]=1;
                    if(kind==6)mask[j]=TestMask(.5f,88); // different replay surface
                }
            }
            if(kind>=10 && kind<=15){
                mask[i+1][3]=0; // Only one current character neighbor.
                if(kind==11)mask[i-1]=TestMask(.5f,99); // wrong primitive
                if(kind==12)mask[i-1][3]=0; // no independent neighbor
            }
            if(kind==17 || kind==18) {
                for(int yy=-1;yy<=1;++yy)for(int xx=-1;xx<=2;++xx) {
                    UINT j=(y+yy)*w+x+xx;
                    mask[j]=TestMask(.5f,kind+1);
                    motion[j][0]=frame==1?-3.2f/w:0;
                    if(frame==1 && (kind==18 || (yy==0 && (xx==0 || xx==1)))) {
                        gi[j]=flashGI;scene[j]=flashScene;
                    }
                }
            }
            if(frame==1 || (frame==0 && kind==7)){gi[i]=flashGI;scene[i]=flashScene;}
            if(frame==1 && kind==8)motion[i][0]=2; // off screen
            if(frame==1 && kind==9)gi[i]=darkGI; // direct-light-only change
            if(frame==1){
                if(kind==13){gi[i-1]=flashGI;scene[i-1]=flashScene;}
                if(kind==14 || kind==19){gi[i]=darkGI;scene[i]=darkScene;gi[i][0]=.002f;scene[i][0]=.00045f;}
                if(kind==15)motion[i][0]=-3.f/w; // only 80% previous character support
                if(kind==16)gi[i][1]=darkGI[1]; // independent green scene spike
            }
            if(kind==1 && frame==1){gi[i]=darkGI;gi[i][1]=.15f;scene[i]=darkScene;scene[i][1]=.03f;}
        }
        expected[frame]=scene;
        if(frame==1){
            for(UINT kind:{0u,1u,10u,17u,19u}){
                if(previousBuild && kind==10)continue;
                UINT first=(kind*4+1)*w+x;
                for(UINT i=first;i<=first+(kind==17);++i)for(UINT k=0;k<3;++k){
                    if((kind==1 && k!=1) || (kind==19 && k!=0))continue;
                    if(previousBuild || kind==19) {
                        expected[frame][i][k]-=.70f*std::min(scene[i][k]-darkScene[k],scene[i][k]*(1-darkGI[k]/gi[i][k]));
                    } else {
                        expected[frame][i][k]=std::max(2*std::max(darkScene[k],.00005f),scene[i][k]*2*darkGI[k]/gi[i][k]);
                    }
                }
            }
        }
        const UINT offset=frame*10;
        std::vector<Pixel>* images[]={&gi,&mask,&depth,&normal,&mat,&scene,&scene,&motion};
        ComPtr<ID3D12Resource> target;
        for(UINT k=0;k<8;++k){auto t=Texture(gpu,w,h,format,images[k]->data(),k==6);TextureView(gpu,t.Get(),offset+k,k==6);if(k==6)target=t;}
        auto* prev=(frame==1?histA:histB).Get();auto* next=(frame==1?histB:histA).Get();
        if(frame)gpu.Transition(next,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TextureView(gpu,prev,offset+8);TextureView(gpu,next,offset+9,true);
        gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
        auto handle=gpu.views->GetGPUDescriptorHandleForHeapStart();handle.ptr+=UINT64(offset)*gpu.descriptorSize;
        gpu.list->SetComputeRootDescriptorTable(0,handle);
        RRCharacterHistoryParams params{float(w),float(h),frame==1?-.8f:0,0,frame?2u:1u};
        gpu.list->SetComputeRoot32BitConstants(2,5,&params,0);
        if(sample){Require(stats[frame].Prepare(gpu.device.Get(),2840+frame),"History counters");stats[frame].Begin(gpu.list.Get());}
        gpu.list->Dispatch((w+7)/8,(h+7)/8,1);
        if(sample)stats[frame].End(gpu.list.Get());
        gpu.Transition(next,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        read[frame]=CopyOutput(gpu,target.Get());
    }
    gpu.Submit();
    for(UINT frame=0;frame<3;++frame){
        unsigned char* p=nullptr;Check(read[frame].buffer->Map(0,nullptr,reinterpret_cast<void**>(&p)));
        for(UINT i=0;i<w*h;++i){
            Pixel actual{};std::memcpy(&actual,p+(i/w)*read[frame].rowPitch+(i%w)*16,16);
            for(UINT k=0;k<4;++k)if(std::abs(actual[k]-expected[frame][i][k])>=1e-6f){
                std::cerr<<"frame="<<frame<<" xy="<<i%w<<","<<i/w<<" channel="<<k<<" actual="<<actual[k]<<" expected="<<expected[frame][i][k]<<"\n";
                Require(false,"Temporal RGB, alpha or disoccluded background");
            }
            if(frame==1 && i==w+9)std::cout<<(previousBuild?"R501":"R502")<<" neck test RR input red="<<actual[0]<<"\n";
        }
        D3D12_RANGE empty{};read[frame].buffer->Unmap(0,&empty);
        if(sample){GICharacterMeasurement::Sample saved{};Require(stats[frame].Read(saved)&&saved.temporal==(frame==1?1u:0u) && saved.strongTemporal==(frame==1?5u:0u),"Count actual history corrections");}
    }
    RRCharacterHistory state;int view=1;state.view=&view;state.frame=7;state.scaleX=w;state.scaleY=h;
    auto mode=[&](UINT f,UINT committed,void* v,int reset){return RRCharacterHistoryForFrame(state,f,committed,v,reset,0,0,w,h).mode;};
    Require(mode(8,7,&view,0)==2 && mode(8,7,&view,1)==1 && mode(9,7,&view,0)==1 &&
            mode(8,6,&view,0)==1 && mode(8,7,nullptr,0)==1,"History cut, gap, failed evaluation or view change");
    std::cout<<"PASS three-frame moving character flash, motion plus jitter, no trailing color, disocclusion, surface/depth/material/normal mismatch, offscreen, direct light, reset and counts\n";
}
