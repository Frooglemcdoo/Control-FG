#include "gpu_test_textures.h"
#include "../src/gi_character_measurement.h"
#include "../src/rr_character_color_binding.h"

#include "test-gi-character-history.h"
#include "test-gi-character-source-regression.h"

// The fixture supplies independent GI and composed RR color, including the
// measured pair at frame 2841. The output starts as a byte-for-byte scene copy.
static void Run(bool sample,bool reference=false,bool remapped=false) {
    std::cout<<"GI composite: measured="<<sample<<" reference="<<reference<<" remap="<<remapped<<"\n";
    Device gpu(false);
    constexpr UINT width=37, height=29;
    constexpr auto format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    std::vector<Pixel> raw(width*height, {0.0005f,0.0003f,0.0002f,0.9f});
    std::vector<Pixel> mask(width*height, {-1,-1,-1,0});
    std::vector<Pixel> depth(width*height, {0.5f,0,0,0});
    std::vector<Pixel> normals(width*height, {0.5f,0.5f,0,0});
    std::vector<Pixel> materials(width*height, {0,0,0,0});
    for(UINT y=1;y<10;++y)
        for(UINT x=2;x<width-5;++x) mask[y*width+x]={0,0,252,1}; // IEEE 0.5 = 0x3f000000
    const UINT centers[]={width*5+5,width*5+10,width*5+15,width*8+6};
    raw[centers[0]]={0.153931f,0.120056f,0.074219f,1};
    raw[centers[1]]={0.02f,0.19f,0.01f,1};
    raw[centers[2]]={0.2f,0.2f,0.2f,1};
    raw[centers[3]]=raw[centers[0]];mask[centers[3]][3]=0.75f;
    raw[width*5+20]=raw[width*5+21]=raw[width*5+36]=raw[centers[2]];
    raw[width*5+25]=raw[width*5+29]=raw[centers[0]];
    depth[width*5+25][0]=0.25f; // An occluder won depth after the character replay.
    materials[width*5+29][2]=1;
    const Pixel background={0.00020f,0.00010f,0.00005f,0.75f};
    std::vector<Pixel> baseline(width*height,background);
    for(UINT i=0;i<width*height;++i)
        if(raw[i][0]>0.001f)baseline[i]={0.036713f,0.015060f,0.004642f,0.875f};
    const UINT direct=width*8+22,giOnly=width*8+26;
    baseline[direct]={0.2f,0.3f,0.4f,0.625f}; // isolated direct light, ordinary GI
    raw[giOnly]=raw[centers[0]]; // GI impulse with no corresponding scene impulse
    raw[centers[1]][0]=0.0005f; // protect the independent red channel at this pixel
    // Each strip has exactly two inner neighbors. A wide tap is valid only
    // along a direction whose intermediate pixel also belongs to this surface.
    const Pixel covered={0,0,252,1};
    const Pixel clear={-1,-1,-1,0};
    auto strip=[&](int x,int y,int dy) {
        for(int i=-2;i<=2;++i) mask[(y+i*dy)*width+x+i]=covered;
        return y*width+UINT(x);
    };
    auto spike=[&](UINT i,UINT channel) {
        for(UINT k=0;k<3;++k) if(k==channel || channel==3) {
            raw[i][k]=0.19f;baseline[i][k]=0.04f;
        }
    };
    const UINT extra[]={strip(4,13,0),strip(12,13,0),strip(20,13,0),strip(4,24,1)};
    spike(extra[0],0);spike(extra[1],1);spike(extra[2],3);spike(extra[3],1);
    const UINT noScene=strip(29,13,0);raw[noScene]=raw[extra[2]];
    const UINT seam=strip(4,18,0);spike(seam,0);materials[seam+2][2]=1;
    const UINT occluded=strip(12,18,0);spike(occluded,1);depth[occluded+2][0]=0.25f;
    const UINT gap=strip(20,18,0);spike(gap,3);mask[gap+1]=clear;
    const UINT highlight=strip(29,18,0);spike(highlight,1);spike(highlight+2,1);
    const UINT oneSide=strip(12,24,0);spike(oneSide,0);
    mask[oneSide-1]=clear;mask[oneSide-width+1]=mask[oneSide-2*width+2]=covered;
    const UINT crease=strip(20,24,0);spike(crease,0);normals[crease+2]={0,0,0,0};
    const UINT directEdge=strip(29,24,0);baseline[directEdge]=baseline[direct];

    // Dense, isolated single-channel impulses, plus a white input whose green
    // channel has no GI impulse. R499 makes the latter bright green.
    const UINT red=width*2+4,green=width*2+9,white=width*2+14,mixedGreen=width*2+20;
    spike(red,0);spike(green,1);spike(white,3);spike(mixedGreen,3);raw[mixedGreen][1]=0.0003f;

    if(!reference)for(auto& pixel:mask)if(pixel[3]>0)pixel=TestMask(.5f,1,pixel[3]);
    const UINT mappedCenter=centers[0], source=mappedCenter-1;
    // Pixel (5,5) selects (4,5). Both destinations see the same lighting ray.
    // Make all other pixels identical and put GI exclusively at its actual source.
    if(remapped){raw[source]=raw[mappedCenter];raw[mappedCenter]=raw[0];}
    const std::vector<Pixel>* images[]={&raw,&mask,&depth,&normals,&materials,&baseline,&baseline};
    ComPtr<ID3D12Resource> target;
    for(UINT i=0;i<7;++i){
        auto tex=Texture(gpu,width,height,format,images[i]->data(),i==6);
        TextureView(gpu,tex.Get(),i,i==6);
        if(i==6)target=tex;
    }
    EmptyHistoryViews(gpu,7);
    const UINT mw=(width+1)/2,mh=(height+1)/2;
    std::vector<UINT> selectors(mw*mh,0x03020100u);
    if(remapped)selectors[2*mw+2]=0x02020100u;
    auto map=Texture(gpu,mw,mh,DXGI_FORMAT_R32_UINT,selectors.data());
    TextureView(gpu,map.Get(),10);
    if(remapped){
        const auto gpuSample=Read("build/character-no-hit-gpu-sample.bin");
        Require(gpuSample.size()==sizeof(Pixel),"Missing no-hit GI readback");
        std::vector<Pixel> filtered=raw;
        std::memcpy(filtered[source].data(),gpuSample.data(),sizeof(Pixel));
        // The environment spike has the same filtered lighting available. Its RR color must survive.
        filtered[width*5+36]=filtered[source];
        auto earlier=Texture(gpu,width,height,format,filtered.data());TextureView(gpu,earlier.Get(),11);
    }else EmptySourceView(gpu,11);
    ComPtr<ID3DBlob> rootBytes,error;
    Check(RRCharacterSerializeRoot(&rootBytes,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto code=Read(reference?"build/gi-character-reference.dxil":sample?"build/gi-character-measured.dxil":"build/gi-character-blend.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
    ComPtr<ID3D12PipelineState> pso;
    std::cout<<"Create character pipeline\n";
    Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    RRCharacterHistoryParams noHistory{};noHistory.useSource=remapped;
    gpu.list->SetComputeRoot32BitConstants(2,6,&noHistory,0);
    GICharacterMeasurement stats;
    if(sample){Require(stats.Prepare(gpu.device.Get(),2841),"counter allocation");stats.Begin(gpu.list.Get());}
    gpu.list->Dispatch((width+7)/8,(height+7)/8,1);
    if(sample)stats.End(gpu.list.Get());
    auto output=CopyOutput(gpu,target.Get());
    std::cout<<"Submit character dispatch\n";gpu.Submit();std::cout<<"Character dispatch completed\n";
    if(remapped){
        unsigned char* bytes=nullptr;Check(output.buffer->Map(0,nullptr,reinterpret_cast<void**>(&bytes)));
        for(UINT i=0;i<width*height;++i){
            Pixel actual{};std::memcpy(&actual,bytes+(i/width)*output.rowPitch+(i%width)*sizeof(Pixel),sizeof(Pixel));
            if(i==mappedCenter){
                for(UINT k=0;k<3;++k)Require(actual[k]>=0 && actual[k]<baseline[i][k]*0.025f,"Both GI remapped flash suppression");
                Require(actual[3]==baseline[i][3],"Source correction preserves alpha");
                std::cout<<"R505 remapped flash RR red "<<baseline[i][0]<<" -> "<<actual[0]<<"\n";
            }else Require(actual==baseline[i],"Character correction altered environment, stable light, or independent RGB");
        }
        D3D12_RANGE empty{};output.buffer->Unmap(0,&empty);
        GICharacterMeasurement::Sample record{};Require(stats.Read(record)&&record.sourceCorrected==1,"one source correction");
        return;
    }
    if(sample){
        GICharacterMeasurement::Sample record{};Require(stats.Read(record),"counter readback");
        const auto& counts=record.counts;
        UINT drawn=0;
        for(const auto& m:mask) if(m[3]>0) ++drawn;
        Require(counts[0]==2 && counts[1]+counts[2]+counts[3]+counts[4]==drawn-2,"Visible character coverage");
        Require(counts[3]==6 && counts[4]==2 && record.location!=0,"Corrected and preserved totals");
        UINT xy=record.location-1, idx=(xy>>16)*width+(xy&65535);
        Require(idx==centers[1] || idx==mixedGreen,"Recorded pixel matches an actual mixed-source sample");
        UINT independent=idx==centers[1]?0:1;
        for(UINT k=0;k<3;++k){
            Require(record.gi[k]==raw[idx][k] && record.scene[k]==baseline[idx][k],"Recorded linear inputs");
            float expected=k==independent?baseline[idx][k]:background[k];
            Require(std::abs(record.proposed[k]-expected)<1e-6f,"Recorded rejected color");
        }
    }
    unsigned char* mapped=nullptr;Check(output.buffer->Map(0,nullptr,reinterpret_cast<void**>(&mapped)));
    for(UINT i=0;i<width*height;++i){
        Pixel p{};std::memcpy(&p,mapped+(i/width)*output.rowPitch+(i%width)*sizeof(p),sizeof(p));
        const bool mixed=i==centers[1] || i==mixedGreen;
        const bool sparse=std::find(std::begin(extra),std::end(extra),i)!=std::end(extra);
        const bool dense=i==centers[0] || i==centers[2] || i==centers[3] || i==red || i==green || i==white;
        if(!dense && !(reference && (sparse || mixed))){
            Require(p==baseline[i],"Unexpected change at sparse, mixed-light, edge or background pixel");
        } else {
            for(UINT k=0;k<3;++k){
                const bool keep=reference && ((i==centers[1] && k==0) || (i==mixedGreen && k==1));
                const float expected=keep?baseline[i][k]:baseline[i][k]+mask[i][3]*(background[k]-baseline[i][k]);
                Require(std::abs(p[k]-expected)<1e-6f,"Incorrect composed color or independent channel");
            }
            Require(p[3]==baseline[i][3],"Original scene alpha");
        }
        if(i==centers[1] || i==mixedGreen)
            std::cout<<(reference?"R499":"R502")<<" input "<<i%width<<","<<i/width
                     <<" scene "<<baseline[i][0]<<","<<baseline[i][1]<<","<<baseline[i][2]
                     <<" result "<<p[0]<<","<<p[1]<<","<<p[2]<<"\n";
    }
    D3D12_RANGE empty{};output.buffer->Unmap(0,&empty);
}
static void TestBinding(){
    struct Api {
        int* value;int* reject;
        void Resource(const char*,int* p){if(p!=reject)value=p;}
        bool ResourceEquals(const char*,int* p){return value==p;}
    };
    int source=1,corrected=2;
    for(unsigned mode=0;mode<3;++mode){
        Api api{&source,mode==2?&corrected:nullptr};unsigned calls=0;bool bound=false;
        auto eval=[&](){
            ++calls;Require(api.value==(mode!=2?&corrected:&source),"Wrong color during evaluation");
            return mode==1?0xBAD00002u:1u;
        };
        auto result=RRWithCharacterColor(api,&source,&corrected,eval,bound);
        Require(result==(mode==1?0xBAD00002u:1u) && calls==1 && bound==(mode!=2) && api.value==&source,"Color scope or fallback");
    }
}
int main(int argc,char** argv) try {
    std::cout<<std::unitbuf;std::cerr<<std::unitbuf;
    std::cout<<"Checking character color binding\n";TestBinding();
    if(argc==2 && std::strcmp(argv[1],"--compare-r499")==0){Run(false,true);TestMotionHistory(false,true);TestBoundSource(true); }
    TestBoundSource(false);Run(true,false,true);Run(false);Run(true);TestMotionHistory(false);TestMotionHistory(true);
    std::cout<<"PASS character impulse and mixed-light RGB, sparse boundaries, recorded veto evidence, and RR binding\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
