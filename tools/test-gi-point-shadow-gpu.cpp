#include "gpu_test_device.h"
int main()try{
 Device d(false);constexpr UINT rows=65536,bytes=rows*3*16;
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0};
 D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable={1,&range};
 D3D12_ROOT_SIGNATURE_DESC rd{1,&param,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("build/gi-point-fixture.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;
 Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 auto output=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);auto view=d.View(output.Get(),0,16,rows*3,true);
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootDescriptorTable(0,view);d.list->Dispatch(rows/64,1,1);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);auto rb=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyResource(rb.Get(),output.Get());d.Submit();
 void* mapped=nullptr;Check(rb->Map(0,nullptr,&mapped));auto data=static_cast<UINT*>(mapped);unsigned callsOld=0,callsNew=0,mismatch=0,invalid=0;
 for(UINT row=0;row<rows;++row){const UINT* a=data+row*12;callsOld+=a[3];callsNew+=a[7];
  for(UINT c=0;c<3;++c){float x,y;std::memcpy(&x,a+c,4);std::memcpy(&y,a+4+c,4);invalid+=!std::isfinite(x)||!std::isfinite(y);mismatch+=a[c]!=a[4+c];}
 }
 rb->Unmap(0,nullptr);std::cout<<"Point-shadow fixture: cases="<<rows<<" mismatched_channels="<<mismatch<<" nonfinite="<<invalid<<" reference_lookups="<<callsOld<<" candidate_lookups="<<callsNew<<'\n';
 Require(!invalid,"Nonfinite fixture output");Require(!mismatch,"Point shadow rejection changed output bits");Require(callsNew<callsOld&&callsNew>0,"Gate did not selectively reduce shadow lookups");
 std::cout<<"PASS native point-light math and shadow gate produce identical accumulated RGB\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
