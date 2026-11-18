#include "gpu_test_device.h"
#include "../src/gi_reuse/regional_target_lookup.h"
int main()try{
 using namespace gi_reuse;Device d(false);constexpr UINT capacity=16;
 struct Query{GIWorldGuideKey receiver;UINT frame,version,age;};
 static_assert(sizeof(Query)==32,"query ABI");
 const auto key=[](float x){return GIMakeWorldGuideKey({x,.1f,.1f},{0,0,1});};
 const auto a=key(.1f);auto b=key(2.1f);
 for(float x=2.1f;GIRegionalSlot(a,capacity,0)!=GIRegionalSlot(b,capacity,0);x+=.25f)b=key(x);
 std::vector<GIRegionalTarget> table(capacity);
 const UINT first=GIRegionalSlot(a,capacity,0),second=GIRegionalSlot(b,capacity,1);
 const auto target=[](){GIAutoTargets t{};t.schema=0x41544731;t.frame=20;t.version=7;t.validMask=1;t.bounds[0]={{1,0,3},.3f};t.counts[0]=16;t.scores[0]=100;return t;};
 table[first]={a,target()};table[second]={b,target()};
 auto back=a;back.normal^=4;auto invalid=a;invalid.valid=0;
 std::vector<Query> queries={{a,22,7,4},{b,22,7,4},{a,25,7,4},{a,19,7,4},{a,22,8,4},{back,22,7,4},{invalid,22,7,4}};
 const UINT count=UINT(queries.size());auto input=d.Upload(table),query=d.Upload(queries);
 auto output=d.Buffer(count*sizeof(UINT),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto read=d.Buffer(count*sizeof(UINT),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_ROOT_PARAMETER params[4]{};
 params[0].ParameterType=params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[0].Descriptor={0,0};params[1].Descriptor={1,0};
 params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[2].Descriptor={0,0};
 params[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[3].Constants={0,0,1};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=4;rd.pParameters=params;
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-regional-target-validation.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
 ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());
 d.list->SetComputeRootShaderResourceView(0,input->GetGPUVirtualAddress());d.list->SetComputeRootShaderResourceView(1,query->GetGPUVirtualAddress());
 d.list->SetComputeRootUnorderedAccessView(2,output->GetGPUVirtualAddress());d.list->SetComputeRoot32BitConstant(3,capacity,0);d.list->Dispatch(count,1,1);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyResource(read.Get(),output.Get());d.Submit();
 UINT* values=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&values)));
 Require(values[0]==first&&values[1]==second,"GPU crossed receiver regions under collision");
 for(UINT i=2;i<count;++i)Require(values[i]==0xffffffffu,"GPU admitted invalid guide identity");
 D3D12_RANGE none{};read->Unmap(0,&none);
 std::cout<<"PASS GPU receiver separation under hash collision and stale/future/version/normal/invalid-key rejection. No game integration or GPU timing claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
