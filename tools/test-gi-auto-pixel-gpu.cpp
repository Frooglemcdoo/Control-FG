#include "gpu_test_device.h"
#include "../src/gi_reuse/auto_pixel_audit.h"
int main()try {using namespace gi_reuse;Device d(false);
 constexpr UINT capacity=9,width=19,height=17;
 constexpr size_t prefix=capacity*sizeof(GIAutoTraining),payload=width*height*sizeof(GIAutoPixelAudit);
 std::vector<unsigned char> sentinel(prefix+payload+64,0xcd);
 auto initial=d.Upload(sentinel);auto data=d.Buffer(sentinel.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,true);
 auto read=d.Buffer(sentinel.size()*2,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->CopyResource(data.Get(),initial.Get());d.Transition(data.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;p[0].Descriptor={0,19};p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={1,19,4};
 D3D12_ROOT_SIGNATURE_DESC rd{2,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-auto-pixel.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootUnorderedAccessView(0,data->GetGPUVirtualAddress());
 for(UINT phase=0;phase<2;++phase){const UINT controls[]={width,height,8,capacity|(phase?65536u:0u)};d.list->SetComputeRoot32BitConstants(1,4,controls,0);d.list->Dispatch(3,3,1);
 d.Transition(data.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(read.Get(),phase*sentinel.size(),data.Get(),0,sentinel.size());d.Transition(data.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
 d.Submit();unsigned char* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));
 Require(std::memcmp(values,sentinel.data(),sentinel.size())==0,"disabled audit wrote memory");
 const auto* second=values+sentinel.size();Require(std::memcmp(second,sentinel.data(),prefix)==0,"audit overwrote learner");Require(std::memcmp(second+prefix+payload,sentinel.data()+prefix+payload,64)==0,"audit overwrote suffix guard");
 for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x){GIAutoPixelAudit v{};std::memcpy(&v,second+prefix+(y*width+x)*sizeof(v),sizeof(v));
 if((x+y)%3==0){Require(v.flags==0&&v.weighted[0]==0&&v.rawPeak==0&&v.material==65534,"invalid pixel not cleared");}
 else Require(v.raw[0]==float(x)&&v.raw[1]==float(y)&&v.raw[2]==3&&v.rawPeak==4&&v.weighted[0]==5&&v.weighted[1]==6&&v.weighted[2]==7&&v.weightedPeak==8&&v.position[0]==9&&v.position[1]==10&&v.position[2]==11&&v.weight==.25f&&v.material==1913&&v.flags==7&&v.ray==4&&v.covered==6,"pixel ABI/coverage mismatch");}
 D3D12_RANGE empty{};read->Unmap(0,&empty);std::cout<<"PASS all-pixel audit ABI, disabled path, irregular ROI, learning-prefix and suffix guards, invalid clearing.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
