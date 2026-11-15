#include "gpu_test_device.h"
#include <array>
int main()try{
 Device d(false);constexpr UINT rows=17,groups=57,words=groups*4,bytes=rows*words*4;
 const UINT slots[]={5,7,11,13,15},strides[]={336,160,320,80,16};
 std::vector<UINT> expected(rows*words);UINT group=0;
 for(UINT b=0;b<5;++b){const UINT n=strides[b]/4;std::vector<UINT> data(rows*n);
  for(UINT row=0;row<rows;++row)for(UINT f=0;f<n;++f)data[row*n+f]=(0x9e3779b9u*(1+row*n+f))^(slots[b]*0x85ebca6bu);
  auto input=d.Upload(data);d.View(input.Get(),slots[b],strides[b],rows,false);
  for(UINT first=0;first<n;first+=4,++group)for(UINT row=0;row<rows;++row)for(UINT lane=0;lane<4;++lane)
   expected[row*words+group*4+lane]=data[row*n+first+(lane+group)%4];
 }
 Require(group==groups,"fixture size mismatch");
 D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,16,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[2]{};for(UINT i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[i].DescriptorTable={1,ranges+i};}
 D3D12_ROOT_SIGNATURE_DESC rd{2,params,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 std::array<ComPtr<ID3D12Resource>,2> readback;std::array<ComPtr<ID3D12PipelineState>,2> psos;
 for(UINT pass=0;pass<2;++pass){auto code=Read(pass?"build/gi-load-fixture-fast.dxil":"build/gi-load-fixture.dxil");
  D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&psos[pass])));
  auto output=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
  auto view=d.View(output.Get(),16+pass,16,rows*groups,true);
  d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(psos[pass].Get());
  d.list->SetComputeRootDescriptorTable(0,d.views->GetGPUDescriptorHandleForHeapStart());d.list->SetComputeRootDescriptorTable(1,view);d.list->Dispatch(3,1,1);
  d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  readback[pass]=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyResource(readback[pass].Get(),output.Get());
 }
 d.Submit();for(auto& buffer:readback){void* data=nullptr;Check(buffer->Map(0,nullptr,&data));Require(!std::memcmp(data,expected.data(),bytes),"GI structured field mismatch");buffer->Unmap(0,nullptr);}
 std::cout<<"PASS original/combined GPU loads match CPU bits for all five record layouts, all fields, 17 rows including final record\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
