#include "gpu_test_device.h"
static UINT Hash(UINT x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
static UINT Coordinate(UINT frame,UINT pixel,UINT ray,unsigned dimension){
 UINT n=frame*4u+ray,g=n^(n>>1),value=0,direction=0x80000000u;
 for(unsigned bit=0;bit<32;++bit){if((g>>bit)&1)value^=dimension?direction:(1u<<(31-bit));direction^=direction>>1;}
 const UINT seed=Hash(pixel^1013904226u^(dimension?0x02e5be93u:0x68bc21ebu));UINT result=0;
 for(int bit=31;bit>=8;--bit){const UINT prefix=bit==31?0:value&(~0u<<(bit+1));const UINT flip=Hash(seed^prefix^(UINT(bit)*0x9e3779b9u))&1u;result|=(((value>>bit)&1u)^flip)<<bit;}
 const float valueFloat=std::ldexp(float(result>>8),-24);UINT bits;std::memcpy(&bits,&valueFloat,4);return bits;
}
int main()try{
 Device d(false);constexpr UINT rows=4096,bytes=rows*8;
 D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0};
 D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable={1,&range};
 D3D12_ROOT_SIGNATURE_DESC rd{1,&param,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
 Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> root;
 Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("build/gi-progressive-fixture.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;
 Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 auto output=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);auto view=d.View(output.Get(),0,8,rows,true);
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootDescriptorTable(0,view);d.list->Dispatch(rows/64,1,1);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);auto rb=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyResource(rb.Get(),output.Get());d.Submit();
 void* mapped=nullptr;Check(rb->Map(0,nullptr,&mapped));auto data=static_cast<UINT*>(mapped);unsigned mismatch=0;
 const UINT boundaries[]={0,1,2,255,256,65535,0x3fffffff,0x40000000,0xffffffff};
 for(UINT row=0;row<rows;++row){const UINT frame=row<9?boundaries[row]:Hash(row*31u);
  for(unsigned dimension=0;dimension<2;++dimension)mismatch+=data[row*2+dimension]!=Coordinate(frame,Hash(row>>2),row&3,dimension);
 }
 rb->Unmap(0,nullptr);Require(!mismatch,"Progressive GPU/CPU coordinate mismatch");
 std::cout<<"PASS progressive GPU/CPU coordinate bits: 4096 cases, both axes, frame-wrap boundaries\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
