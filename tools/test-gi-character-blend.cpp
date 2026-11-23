#include "gpu_test_textures.h"
#include "../src/gi_character_measurement.h"

// Seed output exactly as the native raw-bypass shader does. Uncovered pixels
// must remain bit-identical even when they are brighter than the foreground.
static void Run(bool sample) {
    Device gpu(false);
    constexpr UINT width=37, height=11;
    constexpr auto format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    std::vector<Pixel> raw(width*height, {0.0005f,0.0003f,0.0002f,0.9f});
    std::vector<Pixel> mask(width*height, {-1,-1,-1,0});
    std::vector<Pixel> depth(width*height, {0.5f,0,0,0});
    std::vector<Pixel> normals(width*height, {0.5f,0.5f,0,0});
    std::vector<Pixel> materials(width*height, {0,0,0,0});
    for(UINT y=1;y<height-1;++y)
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
    std::vector<Pixel> baseline=raw;
    for(auto& p:baseline)p[3]=p[0];
    const std::vector<Pixel>* images[]={&raw,&mask,&depth,&normals,&materials,&baseline};
    ComPtr<ID3D12Resource> target;
    for(UINT i=0;i<6;++i){
        auto tex=Texture(gpu,width,height,format,images[i]->data(),i==5);
        TextureView(gpu,tex.Get(),i,i==5);
        if(i==5)target=tex;
    }
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,5}};
    D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={2,ranges};
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor.ShaderRegister=1;
    D3D12_ROOT_SIGNATURE_DESC desc{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> rootBytes,error;
    Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&rootBytes,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,rootBytes->GetBufferPointer(),rootBytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto code=Read(sample?"build/gi-character-measured.dxil":"build/gi-character-blend.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
    ComPtr<ID3D12PipelineState> pso;
    Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    GICharacterMeasurement stats;
    if(sample){Require(stats.Prepare(gpu.device.Get(),2841),"counter allocation");stats.Begin(gpu.list.Get());}
    gpu.list->Dispatch((width+7)/8,(height+7)/8,1);
    if(sample)stats.End(gpu.list.Get());
    auto output=CopyOutput(gpu,target.Get());gpu.Submit();
    if(sample){
        std::array<UINT,4> counts{};Require(stats.Read(counts),"counter readback");
        const UINT drawn=(height-2)*(width-7);
        Require(counts[0]==1 && counts[1]+counts[2]+counts[3]==drawn-1 && counts[3]==4,"GPU coverage and correction counts");
    }
    unsigned char* mapped=nullptr;Check(output.buffer->Map(0,nullptr,reinterpret_cast<void**>(&mapped)));
    for(UINT i=0;i<width*height;++i){
        Pixel p{};std::memcpy(&p,mapped+(i/width)*output.rowPitch+(i%width)*sizeof(p),sizeof(p));
        if(std::find(std::begin(centers),std::end(centers),i)==std::end(centers))
            Require(p==baseline[i],"Unexpected write on background, edge, or supported highlight");
        else {
            const float peak=(std::max)({raw[i][0],raw[i][1],raw[i][2]});
            const float coverage=mask[i][3];
            for(UINT k=0;k<3;++k)
                Require(std::abs(p[k]-raw[i][k]*(1-coverage+coverage*0.0005f/peak))<1e-6f,"Impulse outside neighborhood scale");
            Require(p[3]==p[0],"Native red/alpha contract");
        }
    }
    D3D12_RANGE empty{};output.buffer->Unmap(0,&empty);
}
int main() try {
    Run(false);Run(true);
    std::cout<<"PASS character GI RGB impulses, supported highlight, depth/material edges and uncovered background\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
