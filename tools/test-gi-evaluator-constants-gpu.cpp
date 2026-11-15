#include "gpu_test_device.h"
#include <array>
int main()try{
 Device d(false);constexpr UINT rows=17,bytes=rows*16;
 std::vector<UINT> constants(64),expected(rows*4);
 for(UINT i=0;i<64;++i)constants[i]=0x9e3779b9u*(i+1)^0x85ebca6bu;
 for(UINT row=0;row<rows;++row)for(UINT lane=0;lane<4;++lane){
  UINT v=constants[lane];for(UINT i=0;i<row;++i){v=(v*1664525u+constants[4+lane])^constants[8+lane];
   if((i+row)&1)v+=constants[12+lane];for(UINT j=0;j<row;++j)v^=constants[16+lane]+j;
   v+=constants[((i&7)+8)*4+lane];}expected[row*4+lane]=v+constants[lane];
 }
 auto input=d.Upload(constants);
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0};
 D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[0].Descriptor={0,0};
 params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1].DescriptorTable={1,&range};
 D3D12_ROOT_SIGNATURE_DESC rd{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 std::array<ComPtr<ID3D12Resource>,2> readback;std::array<ComPtr<ID3D12PipelineState>,2> psos;
 for(UINT pass=0;pass<2;++pass){auto code=Read(pass?"build/gi-constant-fixture-fast.dxil":"build/gi-constant-fixture.dxil");
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&psos[pass])));
  auto output=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);auto view=d.View(output.Get(),pass,16,rows,true);
  d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(psos[pass].Get());d.list->SetComputeRootConstantBufferView(0,input->GetGPUVirtualAddress());
  d.list->SetComputeRootDescriptorTable(1,view);d.list->Dispatch(3,1,1);
  d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  readback[pass]=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyResource(readback[pass].Get(),output.Get());
 }
 d.Submit();for(auto& buffer:readback){void* data=nullptr;Check(buffer->Map(0,nullptr,&data));Require(!std::memcmp(data,expected.data(),bytes),"GI constant-loop mismatch");buffer->Unmap(0,nullptr);}
 std::cout<<"PASS original/hoisted GPU CB reads match CPU bits: 0..16 rays, nested loops, conditional and dynamic-index reads\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
