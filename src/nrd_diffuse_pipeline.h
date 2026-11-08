#pragma once
#include "nrd_diffuse_d3d12.h"
#include <memory>

namespace control_nrd {
// Complete conversion -> RELAX -> remodulation transaction. This class does
// not install hooks. The boundary owner must supply same-frame resources and
// restore the native compute bindings after Record, then fence-retire Work.
class DiffusePipeline {
 template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
 using External=D3D12Backend::External;
 Ptr<ID3D12Device> device;
 Ptr<ID3D12RootSignature> root;
 Ptr<ID3D12PipelineState> prepare,finish;
 std::array<Ptr<ID3D12Resource>,6> textures;
 std::array<D3D12_RESOURCE_STATES,6> states{};
 std::unique_ptr<D3D12Backend> backend;
 Session session;
 UINT width,height,increment;
 float historyScale=0;
 bool failed=false;
 static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("NRD conversion D3D12 failure");}
 static void Transition(ID3D12GraphicsCommandList* l,External& r,D3D12_RESOURCE_STATES next){
  D3D12_RESOURCE_BARRIER b{};
  if(r.state!=next){b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r.resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,r.state,next};l->ResourceBarrier(1,&b);r.state=next;}
  else if(next==D3D12_RESOURCE_STATE_UNORDERED_ACCESS){b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=r.resource;l->ResourceBarrier(1,&b);}
 }
public:
 // Order matches native conversion t0..t5. Output must not alias an input.
 enum Input:unsigned {RawGI,NormalRoughness,ViewZ,Motion,HitDistance,Throughput};
 struct Bindings {std::array<External,6> input;External output;};
 struct Constants {
  UINT extent[2]{};float motionScale[2]{1,1};float motionBias[2]{};
  float radianceScale=1,denoisingRange=500000;UINT valid=1;
 };
 static_assert(sizeof(Constants)==36);
 struct Work {
  D3D12Backend::Recording relax;
  bool commandsRecorded=false;
  Ptr<ID3D12DescriptorHeap> heap;Ptr<ID3D12Resource> constants;
  Ptr<ID3D12RootSignature> root;
  Ptr<ID3D12PipelineState> prepare,finish;
  std::vector<Ptr<ID3D12Resource>> retained;
  void* mapped=nullptr;
  ~Work(){if(mapped)constants->Unmap(0,nullptr);}
  void RetireForReuse(){relax.RetireForReuse();commandsRecorded=false;heap.Reset();root.Reset();prepare.Reset();finish.Reset();retained.clear();}
  Work()=default;Work(const Work&)=delete;Work&operator=(const Work&)=delete;
 };
 DiffusePipeline(ID3D12Device* d,UINT w,UINT h,D3D12_SHADER_BYTECODE prep,D3D12_SHADER_BYTECODE final):device(d),width(w),height(h){
  if(!d||!w||!h||w>8192||h>8192||!prep.pShaderBytecode||!final.pShaderBytecode)throw std::runtime_error("Invalid diffuse pipeline creation");
  increment=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,6,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,5,0,0,0}};
  D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;
  for(unsigned i=0;i<2;++i){p[i+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i+1].DescriptorTable={1,&ranges[i]};}
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=p;Ptr<ID3DBlob> b,error;
  Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&error));
  Check(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&root)));
  D3D12_COMPUTE_PIPELINE_STATE_DESC ps{};ps.pRootSignature=root.Get();ps.CS=prep;Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&prepare)));
  ps.CS=final;Check(d->CreateComputePipelineState(&ps,IID_PPV_ARGS(&finish)));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  for(unsigned i=0;i<6;++i){D3D12_RESOURCE_DESC t{};t.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;t.Width=w;t.Height=h;t.DepthOrArraySize=t.MipLevels=1;t.SampleDesc.Count=1;
   t.Format=i==2?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R16G16B16A16_FLOAT;t.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   Check(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&t,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&textures[i])));
  }
  backend=std::make_unique<D3D12Backend>(d,session.Description(),std::uint16_t(w),std::uint16_t(h));
 }
 // Live insertion uses a fresh range in the ALREADY BOUND native heap. Never
 // switch heaps there: doing so also invalidates the game's graphics tables.
 size_t DispatchCount()const noexcept{return session.Plan().size();}
 UINT MaxDescriptors()const noexcept{return 22+backend->MaxDescriptors();}
 bool Record(ID3D12GraphicsCommandList* list,const Frame& frame,std::uint64_t queue,Bindings& bindings,Constants conversion,Work& work,D3D12Backend::Arena arena={nullptr,0,0}){
  if(failed||!list||work.heap||frame.width!=width||frame.height!=height||!std::isfinite(conversion.radianceScale)||conversion.radianceScale<=0||conversion.valid!=1)return false;
  if(arena.heap&&(arena.count<22||arena.first>UINT32_MAX-22))return false;
  for(float v:conversion.motionScale)if(!std::isfinite(v))return false;
  for(float v:conversion.motionBias)if(!std::isfinite(v))return false;
  // Motion has already been converted to normalized unjittered screen units.
  // Reject double scaling or world-space interpretation in NRD itself.
  if(frame.settings.isMotionVectorInWorldSpace||frame.settings.motionVectorScale[0]!=1||frame.settings.motionVectorScale[1]!=1||frame.settings.motionVectorScale[2]!=0)return false;
  for(float v:frame.settings.motionVectorBias)if(v!=0)return false;
  std::array<External*,7> all{};for(unsigned i=0;i<6;++i)all[i]=&bindings.input[i];all[6]=&bindings.output;
  for(unsigned i=0;i<7;++i){if(!all[i]->resource)return false;auto d=all[i]->resource->GetDesc();
   if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=width||d.Height!=height||d.DepthOrArraySize!=1||!d.MipLevels||d.SampleDesc.Count!=1)return false;
   const bool rgba=d.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||d.Format==DXGI_FORMAT_R32G32B32A32_FLOAT;
   const bool rg=d.Format==DXGI_FORMAT_R16G16_FLOAT||d.Format==DXGI_FORMAT_R32G32_FLOAT;
   const bool scalar=d.Format==DXGI_FORMAT_R16_FLOAT||d.Format==DXGI_FORMAT_R32_FLOAT;
   if(!rgba&&!(i==Motion&&rg)&&!((i==ViewZ||i==HitDistance)&&(rg||scalar)))return false;
   Ptr<ID3D12Device> resourceDevice;Ptr<IUnknown> ownerIdentity,deviceIdentity;
   if(FAILED(all[i]->resource->GetDevice(IID_PPV_ARGS(&resourceDevice)))||
      FAILED(resourceDevice.As(&ownerIdentity))||FAILED(device.As(&deviceIdentity))||ownerIdentity.Get()!=deviceIdentity.Get())return false;
   for(unsigned j=0;j<i;++j)if(all[i]->resource==all[j]->resource)return false;
  }
  if(!(bindings.output.resource->GetDesc().Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))return false;
  Frame actual=frame;
  if(historyScale&&historyScale!=conversion.radianceScale)actual.settings.accumulationMode=nrd::AccumulationMode::CLEAR_AND_RESTART;
  if(!session.Prepare(actual,queue))return false;
  conversion.extent[0]=width;conversion.extent[1]=height;conversion.denoisingRange=actual.settings.denoisingRange;
  bool commands=false;
  std::array<D3D12_RESOURCE_STATES,7> original{};for(unsigned i=0;i<7;++i)original[i]=all[i]->state;
  try {
   D3D12Backend::Inputs inputs;
   const nrd::ResourceType kinds[]={nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,nrd::ResourceType::IN_NORMAL_ROUGHNESS,nrd::ResourceType::IN_VIEWZ,nrd::ResourceType::IN_MV,nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST};
   const unsigned slots[]={0,1,2,3,5};
   for(unsigned i=0;i<5;++i)inputs[kinds[i]]={textures[slots[i]].Get(),states[slots[i]]};
   // Every allocation precedes any command. Allocation failure can return to
   // native without partially recording a conversion or changing tracked states.
   D3D12Backend::Arena relaxArena=arena;if(arena.heap){relaxArena.first+=22;relaxArena.count-=22;}
   backend->AllocateRecording(session.Plan(),inputs,work.relax,relaxArena);
   D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=22;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   if(arena.heap)work.heap=arena.heap;else Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&work.heap)));
   D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;hp.CreationNodeMask=hp.VisibleNodeMask=1;
   D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=256;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   if(!work.constants)Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&work.constants)));
   if(!work.mapped){D3D12_RANGE empty{};Check(work.constants->Map(0,&empty,&work.mapped));}
   std::memcpy(work.mapped,&conversion,sizeof(conversion));
   for(auto* r:all)work.retained.emplace_back(r->resource);for(auto& t:textures)work.retained.push_back(t);
   work.root=root;work.prepare=prepare;work.finish=finish;
   auto cpu=work.heap->GetCPUDescriptorHandleForHeapStart();auto gpu=work.heap->GetGPUDescriptorHandleForHeapStart();
   if(arena.heap){cpu.ptr+=SIZE_T(arena.first)*increment;gpu.ptr+=UINT64(arena.first)*increment;}
   auto ch=[&](UINT i){return D3D12_CPU_DESCRIPTOR_HANDLE{cpu.ptr+SIZE_T(i)*increment};};
   auto gh=[&](UINT i){return D3D12_GPU_DESCRIPTOR_HANDLE{gpu.ptr+UINT64(i)*increment};};
   auto srv=[&](ID3D12Resource* r,UINT i){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=r?r->GetDesc().Format:DXGI_FORMAT_R16G16B16A16_FLOAT;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;device->CreateShaderResourceView(r,&v,ch(i));};
   auto uav=[&](ID3D12Resource* r,UINT i){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=r?r->GetDesc().Format:DXGI_FORMAT_R16G16B16A16_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;device->CreateUnorderedAccessView(r,nullptr,&v,ch(i));};
   for(UINT i=0;i<6;++i)srv(bindings.input[i].resource,i);
   for(UINT i=0;i<5;++i)uav(textures[i].Get(),6+i);
   srv(textures[5].Get(),11);srv(textures[4].Get(),12);srv(bindings.input[RawGI].resource,13);
   for(UINT i=14;i<17;++i)srv(nullptr,i);uav(bindings.output.resource,17);for(UINT i=18;i<22;++i)uav(nullptr,i);
   auto bind=[&](ID3D12PipelineState* ps,UINT base){auto* heap=work.heap.Get();if(!arena.heap)list->SetDescriptorHeaps(1,&heap);list->SetComputeRootSignature(root.Get());list->SetPipelineState(ps);list->SetComputeRootConstantBufferView(0,work.constants->GetGPUVirtualAddress());list->SetComputeRootDescriptorTable(1,gh(base));list->SetComputeRootDescriptorTable(2,gh(base+6));};
   commands=true;work.commandsRecorded=true;
   for(auto& input:bindings.input)Transition(list,input,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
   for(unsigned i=0;i<5;++i){External t{textures[i].Get(),states[i]};Transition(list,t,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);states[i]=t.state;}
   bind(prepare.Get(),0);list->Dispatch((width+7)/8,(height+7)/8,1);
   for(unsigned i=0;i<5;++i)inputs[kinds[i]].state=states[slots[i]];
   backend->RecordPrepared(list,session.Plan(),inputs,work.relax);
   for(unsigned i=0;i<5;++i)states[slots[i]]=inputs[kinds[i]].state;
   for(unsigned i:{4u,5u}){External t{textures[i].Get(),states[i]};Transition(list,t,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);states[i]=t.state;}
   Transition(list,bindings.output,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
   bind(finish.Get(),11);list->Dispatch((width+7)/8,(height+7)/8,1);
   // Restore states as well as synchronizing writes when the native output
   // was already UAV. Engine state bookkeeping therefore remains accurate.
   for(unsigned i=0;i<7;++i)Transition(list,*all[i],original[i]);
   historyScale=conversion.radianceScale;return true;
  }catch(...){
   session.CancelAfterReset();
   if(commands){
    // The caller retains Work through submission/reset even on failure. Restore
    // engine-owned states so the verified RAW fallback may overwrite output.
    for(unsigned i=0;i<7;++i)Transition(list,*all[i],original[i]);
    failed=true;backend->AbandonAfterReset();
   }
   return false;
  }
 }
 bool Submitted(std::uint64_t queue,std::uint64_t fence){return !failed&&session.Submitted(queue,fence);}
 void AbandonAfterReset(){failed=true;backend->AbandonAfterReset();session.CancelAfterReset();}
};
}
