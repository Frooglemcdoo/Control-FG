#pragma once
#include "../build/rr_skin_mask_compiled.h"

// Experimental r20s skin-protection pass. Character-family draws are replayed
// into an engine-owned RGBA16F target; this tiny compute pass converts alpha
// coverage to the one-channel FP16 RR responsivity mask. No submission, wait or
// fence is introduced: commands stay on Control's current RR command list.
struct RRSkinMaskOwner {
 ID3D12Device* device=nullptr;
 ID3D12RootSignature* root=nullptr;
 ID3D12PipelineState* pipeline=nullptr;
 ID3D12DescriptorHeap* heap=nullptr;
 ID3D12Resource* mask=nullptr;
 UINT descriptorSize=0,width=0,height=0;
 bool stopped=false;
};
static SRWLOCK rrSkinMaskLock=SRWLOCK_INIT;
static std::vector<RRSkinMaskOwner*> rrSkinMaskOwners; // retained to process exit; avoids unsafe resize release
static std::atomic<unsigned long long> rrSkinMaskDispatches{0};
static std::atomic<unsigned long long> rrSkinMaskFailures{0};

static bool RRSkinMaskCreateOwner(ID3D12Device* device,UINT width,UINT height,RRSkinMaskOwner** out) noexcept {
 if(!device||!out||width<64||height<64||width>8192||height>8192)return false;
 *out=nullptr;auto* c=new(std::nothrow) RRSkinMaskOwner;if(!c)return false;
 c->device=device;c->device->AddRef();c->width=width;c->height=height;
 HRESULT hr=S_OK;
 D3D12_DESCRIPTOR_RANGE ranges[2]{};
 ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=1;ranges[0].BaseShaderRegister=0;ranges[0].RegisterSpace=0;ranges[0].OffsetInDescriptorsFromTableStart=0;
 ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=1;ranges[1].BaseShaderRegister=0;ranges[1].RegisterSpace=0;ranges[1].OffsetInDescriptorsFromTableStart=1;
 D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable.NumDescriptorRanges=2;parameter.DescriptorTable.pDescriptorRanges=ranges;
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&parameter;
 auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
 ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
 if(!serialize)hr=E_NOINTERFACE;else hr=serialize(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);
 if(SUCCEEDED(hr)&&blob)hr=device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&c->root));
 else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
 RRGuideRelease(blob);RRGuideRelease(errors);
 if(SUCCEEDED(hr)){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=c->root;pd.CS={kRRSkinMaskShader,sizeof(kRRSkinMaskShader)};hr=device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&c->pipeline));}
 if(SUCCEEDED(hr)){D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&c->heap));}
 if(SUCCEEDED(hr)){
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=1;d.MipLevels=1;d.Format=DXGI_FORMAT_R16_FLOAT;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&c->mask));
 }
 if(FAILED(hr)||!c->root||!c->pipeline||!c->heap||!c->mask){
  Log("RR_SKIN_MASK_CREATE_FAILED width=%u height=%u hr=0x%08lX retained_partial=1",width,height,static_cast<unsigned long>(hr));
  c->stopped=true;rrSkinMaskOwners.push_back(c);return false;
 }
 c->descriptorSize=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 rrSkinMaskOwners.push_back(c);*out=c;
 Log("RR_SKIN_MASK_READY width=%u height=%u format=R16_FLOAT shader=alpha_character_coverage owner=%p retained_until_exit=1",width,height,c);
 return true;
}

static RRSkinMaskOwner* RRSkinMaskFindOrCreate(ID3D12Device* device,UINT width,UINT height) noexcept {
 for(auto* c:rrSkinMaskOwners)if(c&&!c->stopped&&c->device==device&&c->width==width&&c->height==height)return c;
 RRSkinMaskOwner* created=nullptr;RRSkinMaskCreateOwner(device,width,height,&created);return created;
}

static ID3D12Resource* RRSkinMaskBeforeEvaluation(ID3D12GraphicsCommandList* list,ID3D12Resource* character,UINT width,UINT height,unsigned long long frame) noexcept {
 if(!control_rr::RRUserSkinResponsivity()||!list||!character)return nullptr;
 const auto desc=character->GetDesc();
 if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.Width!=width||desc.Height!=height||desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||desc.MipLevels!=1||desc.SampleDesc.Count!=1)return nullptr;
 ID3D12Device* device=nullptr;HRESULT hr=list->GetDevice(IID_PPV_ARGS(&device));if(FAILED(hr)||!device)return nullptr;
 AcquireSRWLockExclusive(&rrSkinMaskLock);
 RRSkinMaskOwner* c=RRSkinMaskFindOrCreate(device,width,height);
 ReleaseSRWLockExclusive(&rrSkinMaskLock);device->Release();
 if(!c)return nullptr;
 const auto cpuStart=RRPerfClock();
 __try {
  auto cpu=c->heap->GetCPUDescriptorHandleForHeapStart();
  D3D12_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;sd.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sd.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sd.Texture2D.MipLevels=1;
  c->device->CreateShaderResourceView(character,&sd,cpu);
  cpu.ptr+=c->descriptorSize;
  D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R16_FLOAT;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
  c->device->CreateUnorderedAccessView(c->mask,nullptr,&ud,cpu);
  D3D12_RESOURCE_BARRIER b[2]{};
  b[0].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b[0].Transition.pResource=character;b[0].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b[0].Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;b[0].Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  b[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b[1].Transition.pResource=c->mask;b[1].Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b[1].Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;b[1].Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(2,b);
  ID3D12DescriptorHeap* heaps[]{c->heap};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(c->root);list->SetPipelineState(c->pipeline);list->SetComputeRootDescriptorTable(0,c->heap->GetGPUDescriptorHandleForHeapStart());list->Dispatch((width+7)/8,(height+7)/8,1);
  D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=c->mask;list->ResourceBarrier(1,&u);
  std::swap(b[0].Transition.StateBefore,b[0].Transition.StateAfter);std::swap(b[1].Transition.StateBefore,b[1].Transition.StateAfter);list->ResourceBarrier(2,b);
  const auto n=rrSkinMaskDispatches.fetch_add(1,std::memory_order_relaxed)+1;
  if(n<=4||(n&(n-1))==0||frame%240ull==0)Log("RR_SKIN_MASK_DISPATCH frame=%llu count=%llu width=%u height=%u character=%p mask=%p cpu_record_ms=%.3f gpu_wait=0 submission=existing_rr_list responsivity=1",frame,n,width,height,character,c->mask,RRPerfElapsed(cpuStart));
  return c->mask;
 } __except(EXCEPTION_EXECUTE_HANDLER){
  const auto n=rrSkinMaskFailures.fetch_add(1,std::memory_order_relaxed)+1;Log("RR_SKIN_MASK_FAILED frame=%llu count=%llu exception=0x%08lX action=disable_skin_mask_for_frame rr_continues=1",frame,n,GetExceptionCode());return nullptr;
 }
}
