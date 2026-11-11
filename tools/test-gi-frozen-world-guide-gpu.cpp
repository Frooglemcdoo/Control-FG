#include "gpu_test_device.h"
#include "../src/gi_reuse/frozen_world_guide.h"
int main()try{
 using namespace gi_reuse;
 Device d(false);constexpr UINT count=4096,bytes=count*sizeof(GIFrozenRay);
 auto output=d.Buffer(bytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto back=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;param.Descriptor={0,0};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&param;
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-frozen-world.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
 ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());d.list->SetComputeRootUnorderedAccessView(0,output->GetGPUVirtualAddress());d.list->Dispatch(count/64,1,1);
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(back.Get(),0,output.Get(),0,bytes);d.Submit();
 GIFrozenRay* data=nullptr;D3D12_RANGE range{0,bytes};Check(back->Map(0,&range,reinterpret_cast<void**>(&data)));
 for(UINT i=0;i<count;++i){
  UINT scenario=i%4;float energy[64]{};energy[63]=1e30f;energy[2]=3e28f;if(scenario==3)energy[0]=INFINITY;
  auto guide=GIFreezeWorldGuide(energy,scenario==2?255:512,100,7);GIPoint n{.6f,0,(i&4)?-.8f:.8f};
  GIPoint position{-.001f,.25f,-.25f};auto key=GIMakeWorldGuideKey(position,n),stored=key;if(i&8)stored.x+=1;
  auto ref=GISampleKeyedFrozenWorld(guide,stored,key,n,scenario==1?101:100,7,.125f,(float(i)+.5f)/4096,(float((i*73)%4096)+.5f)/4096);
  auto actual=data[i];Require(actual.valid==ref.valid&&actual.trace==ref.trace&&actual.frame==ref.frame&&actual.epoch==ref.epoch,"Frozen ray metadata mismatch");
  float a[]={actual.direction.x,actual.direction.y,actual.direction.z,actual.weight,actual.pdf};
  float b[]={ref.direction.x,ref.direction.y,ref.direction.z,ref.weight,ref.pdf};
  for(UINT j=0;j<5;++j)Require(std::isfinite(a[j])&&std::abs(a[j]-b[j])<.0003f*(1+std::abs(b[j])),"Frozen ray CPU/HLSL numerical mismatch");
 }
 D3D12_RANGE none{0,0};back->Unmap(0,&none);
 std::cout<<"PASS 4096 CPU/HLSL cases on WARP: frozen CDF, flipped normals, stale frame, insufficient/invalid training. Not a hardware performance test.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
