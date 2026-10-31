// Actual native and modified spatial bytecode, including multi-frame history feedback.
#include "gpu_test_device.h"
static void Log(const char*,...) {}
#include "../src/rr_gi_pipeline.h"
#include "gi_history_gpu_fixture.h"
static void Run(unsigned variant,bool patched,bool lastPass,float initialHistory,float velocity,bool cut,bool bounded=false,bool varying=false,bool rejectDepth=false,float currentValue=.2f,bool steady=false,unsigned pattern=0,bool materialEdge=false,bool depthEdge=false){
 Device gpu(false);
 D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,8,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0}};
 D3D12_ROOT_PARAMETER params[4]{};
 for(unsigned i=0;i<2;++i){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=i;}
 for(unsigned i=0;i<2;++i){params[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[2+i].DescriptorTable={1,&ranges[i]};}
 D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
 sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
 sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxAnisotropy=1;sampler.ShaderRegister=6;sampler.RegisterSpace=1;
 D3D12_ROOT_SIGNATURE_DESC desc{4,params,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(gpu.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC p{};p.pRootSignature=root.Get();
 p.CS=variant==0?D3D12_SHADER_BYTECODE{kGINativeSpatial0,sizeof(kGINativeSpatial0)}:D3D12_SHADER_BYTECODE{kGINativeSpatial1,sizeof(kGINativeSpatial1)};
 ComPtr<ID3D12PipelineState> native;Check(gpu.device->CreateComputePipelineState(&p,IID_PPV_ARGS(&native)));
 auto create=+[](ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* x,REFIID iid,void** out)->HRESULT{return d->CreateComputePipelineState(x,iid,out);};
 // Separate fixture devices: retire each fixture's cache after its Submit below.
 control_rr_gi::Capture(gpu.device.Get(),&p,native.Get(),create);
 auto* bundle=control_rr_gi::Find(native.Get());Require(bundle&&bundle->spatial&&bundle->adaptive&&bundle->bounded&&bundle->steady,"spatial capture failed");
 HistoryInput(gpu,.5f,.5f,0);HistoryInput(gpu,materialEdge?.6f:.5f,.5f,1,1,materialEdge?4:0);
 HistoryInput(gpu,depthEdge?.8f:.5f,.5f,2,1,depthEdge?4:0);HistoryInput(gpu,rejectDepth?.8f:.5f,.5f,3);
 HistoryInput(gpu,velocity,velocity,4);auto current=HistoryInput(gpu,currentValue,currentValue,5,1,pattern);
 auto history=HistoryInput(gpu,initialHistory,initialHistory,6);HistoryInput(gpu,1.f,1.f,7);
 std::array<float,288> sys{};sys[0]=sys[4]=16;sys[1]=sys[5]=8;sys[2]=sys[6]=1.f/16;sys[3]=sys[7]=1.f/8;
 auto cb0=gpu.Buffer(sizeof(sys),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,sys.data());
 std::array<UINT,64> filter{};filter[4]=lastPass?1:0;filter[5]=2;filter[6]=cut?1:0;
 auto cb1=gpu.Buffer(sizeof(filter),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,false,filter.data());
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
 auto outputDesc=history.resource->GetDesc();outputDesc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 ComPtr<ID3D12Resource> output;Check(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&outputDesc,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&output)));
 D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=outputDesc.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
 auto cpu=gpu.views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=8*gpu.descriptorSize;gpu.device->CreateUnorderedAccessView(output.Get(),nullptr,&uav,cpu);
 auto srv=gpu.views->GetGPUDescriptorHandleForHeapStart();auto target=srv;target.ptr+=8*gpu.descriptorSize;
 gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetComputeRootConstantBufferView(0,cb0->GetGPUVirtualAddress());gpu.list->SetComputeRootConstantBufferView(1,cb1->GetGPUVirtualAddress());
 gpu.list->SetComputeRootDescriptorTable(2,srv);gpu.list->SetComputeRootDescriptorTable(3,target);
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;gpu.device->GetCopyableFootprints(&outputDesc,0,1,0,&footprint,nullptr,nullptr,&bytes);
 // Missing bounded PSO must report native fallback, never silently select R259.
 if(bounded||steady){
  auto*& optional=steady?bundle->steady:bundle->bounded;auto* saved=optional;optional=nullptr;bool used=false;unsigned forwarded=0;
  control_rr_gi::spatialDispatch=true;control_rr_gi::spatialHistoryBounded=bounded;control_rr_gi::spatialSteady=steady;
  control_rr_gi::Dispatch(native.Get(),1,used,gpu.list.Get(),2,1,1,
   [](ID3D12GraphicsCommandList*,ID3D12PipelineState*){throw std::runtime_error("unexpected fallback PSO substitution");},
   [&](ID3D12GraphicsCommandList*,UINT,UINT,UINT){++forwarded;});
  Require(!used&&forwarded==1,"missing bounded PSO did not fall back");optional=saved;
  control_rr_gi::spatialDispatch=false;control_rr_gi::spatialHistoryBounded=false;control_rr_gi::spatialSteady=false;
 }
 std::vector<ComPtr<ID3D12Resource>> results;
 for(unsigned frame=0;frame<8;++frame){
  if(varying){
   const float value=frame%2?.22f:.18f;
   auto next=HistoryInput(gpu,value,value,9,1,pattern);
   gpu.Transition(next.resource.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
   gpu.Transition(current.resource.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
   gpu.list->CopyResource(current.resource.Get(),next.resource.Get());
   gpu.Transition(current.resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }
  gpu.list->SetPipelineState(native.Get());bool used=false;control_rr_gi::spatialDispatch=patched;
  control_rr_gi::spatialHistoryBounded=bounded;control_rr_gi::spatialSteady=steady;
  control_rr_gi::Dispatch(native.Get(),1,used,gpu.list.Get(),2,1,1,
   [](ID3D12GraphicsCommandList* l,ID3D12PipelineState* ps){l->SetPipelineState(ps);},
   [](ID3D12GraphicsCommandList* l,UINT x,UINT y,UINT z){l->Dispatch(x,y,z);});
  Require(used==patched,"scope selected wrong spatial pipeline");control_rr_gi::spatialDispatch=false;control_rr_gi::spatialHistoryBounded=false;control_rr_gi::spatialSteady=false;
  gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
  auto readback=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;
  D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=output.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  gpu.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);results.push_back(readback);
  gpu.Transition(history.resource.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
  dst={};dst.pResource=history.resource.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  gpu.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  gpu.Transition(history.resource.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  gpu.Transition(output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 }
 gpu.Submit();float expected=initialHistory;unsigned frame=0;float firstAlternating=0;
 for(auto& result:results){
  const float weight=((!patched||bounded)&&lastPass&&!cut&&!rejectDepth)?.7f*std::max(0.f,1.f-40.f*std::sqrt(2.f)*std::abs(velocity)):0.f;
  const float value=varying?(frame%2?.22f:.18f):currentValue;
  if(bounded&&lastPass)expected=std::max(std::max(0.f,.75f*value-.001f),std::min(expected,1.25f*value+.001f));
  expected=value+weight*(expected-value);
  if(varying&&frame==6)firstAlternating=expected;
  if(varying&&bounded&&frame==7)Require(std::abs(expected-firstAlternating)<.04f,"bounded history did not reduce alternating variation");
  ++frame;void* mapped=nullptr;Check(result->Map(0,nullptr,&mapped));
  for(unsigned y=3;y<5;++y)for(unsigned x=3;x<13;++x){
   const auto* pixel=reinterpret_cast<const float*>(static_cast<const unsigned char*>(mapped)+footprint.Offset+y*footprint.Footprint.RowPitch)+4*x;
   float pixelExpected=expected;
   if(pattern){
    const auto val=[&](int sx,int sy){return PatternValue(value,pattern,sx,sy);};
    const float center=val(x,y);float weights[4]={.0625f,.25f,.25f,.0625f},neighbors[4]{};
    const int offsets[4]={-2,-1,1,2};float mean=center;
    for(unsigned k=0;k<4;++k){const int nx=int(x)+(variant==0?offsets[k]:0),ny=int(y)+(variant==1?offsets[k]:0);
     neighbors[k]=val(nx,ny);mean+=neighbors[k];
     if(materialEdge&&((nx>=8)!=(x>=8)))weights[k]=0;
     if(depthEdge&&((nx>=8)!=(x>=8)))weights[k]*=.001f;
    }
    mean*=.2f;float avg=0,squares=0;
    for(unsigned k=0;k<4;++k){const float d=mean>0?std::abs(center-neighbors[k])/mean*weights[k]:0;avg+=d*.25f;squares+=d*d*.25f;}
    const float gate=steady?1.f:std::min(1.f,10.f*std::sqrt(std::abs(squares-avg*avg)));
    float sum=center*.375f,total=.375f;
    for(unsigned k=0;k<4;++k){sum+=neighbors[k]*weights[k]*gate;total+=weights[k]*gate;}
    pixelExpected=sum/total;
    if(steady&&pattern==5&&!materialEdge&&!depthEdge)Require(std::abs(pixelExpected-(value+.2f)*.5f)<.0001f,"checker smoothing reference");
   }
   for(unsigned c=0;c<4;++c)Require(std::isfinite(pixel[c])&&std::abs(pixel[c]-pixelExpected)<.0003f,"spatial history recurrence mismatch");
  }
  result->Unmap(0,nullptr);
 }
 for(auto& slot:control_rr_gi::bundles){auto* b=slot.exchange(nullptr);if(b){b->native->Release();b->adaptive->Release();if(b->bounded)b->bounded->Release();if(b->steady)b->steady->Release();delete b;}}
 control_rr_gi::reserved=0;control_rr_gi::ready=0;
}
int main(){try{
 for(unsigned variant=0;variant<2;++variant)for(bool patched:{false,true}){
  Run(variant,patched,true,2.f,0.f,false);Run(variant,patched,true,.02f,.001f,false);
  Run(variant,patched,false,2.f,0.f,false);Run(variant,patched,true,2.f,0.f,true);
 }
 for(unsigned variant=0;variant<2;++variant){
  Run(variant,true,true,2.f,0.f,false,true);
  Run(variant,true,true,.02f,.001f,false,true);
  Run(variant,true,true,.22f,0.f,false,true);
  Run(variant,true,false,2.f,0.f,false,true);
  Run(variant,true,true,2.f,0.f,true,true);
  Run(variant,true,true,2.f,0.f,false,true,false,true);
  Run(variant,true,true,.2f,0.f,false,true,true);
  Run(variant,true,true,2.f,0.f,false,true,false,false,0.f);
 }
 for(unsigned variant=0;variant<2;++variant)for(bool steady:{false,true}){
  Run(variant,true,true,2.f,0.f,false,false,false,false,.22f,steady,5);
  Run(variant,true,true,.02f,.001f,false,false,true,false,.22f,steady,5);
  Run(variant,true,true,2.f,0.f,false,false,false,false,.4f,steady,4,true);
  Run(variant,true,true,2.f,0.f,false,false,false,false,.4f,steady,4,false,true);
  Run(variant,true,true,2.f,0.f,false,false,false,false,0.f,steady);
 }
 std::cout<<"PASS R261 actual steady spatial shaders: low-contrast checker noise, changing current input, history independence, material boundaries, native depth weights, black input and optional-PSO fallback\n";
 std::cout<<"PASS R260 bounded spatial history: bright/dark stale history, stable history, alternating input stabilization, depth rejection, camera cut, non-final pass and black input\n";
 std::cout<<"PASS R259 actual spatial shaders: eight-frame bright/dark feedback, nonzero motion, non-final pass, camera cut, RGBA ABI and scoped production selection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
