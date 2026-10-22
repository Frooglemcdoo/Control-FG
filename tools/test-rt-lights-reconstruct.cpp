#include "gpu_test_device.h"
#include "../src/rt_lights/layout.h"
#include "../src/rt_lights/tile_layout.h"
using namespace rt_lights;
static void Schedule() {
    for(auto extent: {std::pair<UINT,UINT>{1,1},{127,129},{2560,1440},{67,3}}) {
        const auto w=extent.first,h=extent.second;
        std::vector<UINT> coverage(w*h,0);
        for(UINT index=0;index<TileLayout::Count(w,h);++index) {
            auto tile=TileLayout::At(index,w,h,19);
            Require(tile.receiverCount<=32768 && tile.originX+tile.width<=w && tile.originY+tile.height<=h,"Tile exceeds bounds");
            for(UINT y=tile.coreY;y<tile.coreY+tile.coreHeight;++y)for(UINT x=tile.coreX;x<tile.coreX+tile.coreWidth;++x) {
                ++coverage[y*w+x];
                const UINT left=x>8?x-8:0,top=y>8?y-8:0;
                Require(left>=tile.originX && top>=tile.originY,"Missing reconstruction border");
            }
        }
        for(auto count:coverage)Require(count==1,"Tile interiors overlap or miss pixels");
    }
}
static void Reconstruction() {
    Device gpu(false);
    constexpr UINT width=20,height=20,count=width*height;
    std::vector<ShadowTask> tasks(count*2);
    std::vector<Visibility> visibility(count*2);
    std::vector<Contribution> contributions(count*2);
    std::vector<ReceiverRange> receivers(count);
    for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x) {
        UINT pixel=y*width+x;receivers[pixel]={pixel*2,2,19,0};
        for(UINT light=0;light<2;++light) {
            UINT index=pixel*2+light;auto& task=tasks[index];
            task.receiver={float(x)*.01f,float(y)*.01f,0};task.normal={0,0,1};
            task.lightCenter={0,0,10};task.receiverId=pixel;task.lightId=light;task.frameTag=19;
            auto& c=contributions[index];c.visibilityIndex=index;c.receiverId=pixel;c.lightId=light;c.frameTag=19;
            c.diffuse=light?float3{0,0,2}:float3{2,0,0};c.specular=c.diffuse;
            auto& v=visibility[index];v.receiverId=pixel;v.lightId=light;v.frameTag=19;
            v.valid=(x%2==0 && y%2==0)?1u:2u;v.value=light?1.0f:float(x>=10);v.nearestBlocker=5;
        }
    }
    // The upper row is a disconnected parallel surface. Its red visibility must
    // not leak onto the plane, and the two light identities must stay separate.
    for(UINT x=0;x<width;++x) {tasks[x*2].receiver.z=5;visibility[x*2].value=1;}
    // One stale receiver must retain native lighting instead of importing neighbours.
    contributions[(10*width+10)*2].frameTag=18;
    auto vs=gpu.Upload(visibility);gpu.View(vs.Get(),0,sizeof(Visibility),count*2,false);
    auto cs=gpu.Upload(contributions);gpu.View(cs.Get(),1,sizeof(Contribution),count*2,false);
    auto rs=gpu.Upload(receivers);gpu.View(rs.Get(),2,sizeof(ReceiverRange),count,false);
    auto ts=gpu.Upload(tasks);gpu.View(ts.Get(),3,sizeof(ShadowTask),count*2,false);
    auto output=gpu.Buffer(count*sizeof(Lighting),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
    gpu.View(output.Get(),4,sizeof(Lighting),count,true);
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,4}};
    D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={2,ranges};
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[1].Constants={0,0,16};
    D3D12_ROOT_SIGNATURE_DESC desc{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
    ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto shader=Read("rt_lights_reconstruct.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={shader.data(),shader.size()};
    ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    auto tile=TileLayout::At(0,width,height,19);
    gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
    gpu.list->SetComputeRootDescriptorTable(0,gpu.views->GetGPUDescriptorHandleForHeapStart());
    gpu.list->SetComputeRoot32BitConstants(1,16,&tile,0);gpu.list->Dispatch((count+63)/64,1,1);
    gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto copy=gpu.Buffer(count*sizeof(Lighting),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->CopyResource(copy.Get(),output.Get());gpu.Submit();
    void* mapped=nullptr;Check(copy->Map(0,nullptr,&mapped));auto* results=static_cast<Lighting*>(mapped);
    auto center=results[11*width+9];Require(center.valid==1 && center.diffuse.x>0 && center.diffuse.x<2,"Sparse shadow edge was not reconstructed");
    Require(std::abs(center.diffuse.z-2)<.0001f,"Different lights contaminated each other's visibility");
    Require(results[10*width+10].valid==0,"Stale input was accepted");
    Require(results[2*width+0].valid==1 && results[2*width+0].diffuse.x==0,"Disconnected surface leaked through reconstruction");
    copy->Unmap(0,nullptr);
}
int main() try {Schedule();Reconstruction();std::cout<<"PASS: tiled coverage, sparse shadow reconstruction, light separation, surface rejection and stale-input fallback\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
