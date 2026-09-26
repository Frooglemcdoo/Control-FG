#pragma once
#include "rr_responsivity_policy.h"

// GI25: full-frame RR temporal-responsivity mask. The resource is owned by the
// mod and retained for the process lifetime so no in-flight GPU reference can
// outlive a resize/replacement. The mask is R16_FLOAT, matching NVIDIA's
// documented DLSSD responsivity-mask contract.
struct RRResponsivityOwner {
 ID3D12Device* device=nullptr;
 ID3D12DescriptorHeap* heap=nullptr;
 ID3D12Resource* mask=nullptr;
 UINT width=0,height=0;
 bool stopped=false;
};
static SRWLOCK rrResponsivityLock=SRWLOCK_INIT;
static std::vector<RRResponsivityOwner*> rrResponsivityOwners;
static std::atomic<unsigned long long> rrResponsivityUpdates{0};
static std::atomic<unsigned long long> rrResponsivityFailures{0};

static bool RRResponsivityCreateOwner(ID3D12Device* device,UINT width,UINT height,RRResponsivityOwner** out) noexcept {
 if(!device||!out||width<64||height<64||width>8192||height>8192)return false;
 *out=nullptr;auto* c=new(std::nothrow) RRResponsivityOwner;if(!c)return false;
 c->device=device;c->device->AddRef();c->width=width;c->height=height;
 HRESULT hr=S_OK;
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 hr=device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&c->heap));
 if(SUCCEEDED(hr)){
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=1;d.MipLevels=1;
  d.Format=DXGI_FORMAT_R16_FLOAT;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&c->mask));
 }
 if(SUCCEEDED(hr)&&c->heap&&c->mask){
  D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=DXGI_FORMAT_R16_FLOAT;ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
  device->CreateUnorderedAccessView(c->mask,nullptr,&ud,c->heap->GetCPUDescriptorHandleForHeapStart());
 }
 if(FAILED(hr)||!c->heap||!c->mask){
  c->stopped=true;rrResponsivityOwners.push_back(c);
  Log("RR_GI25_RESPONSIVITY_CREATE_FAILED width=%u height=%u hr=0x%08lX retained_partial=1",width,height,static_cast<unsigned long>(hr));
  return false;
 }
 rrResponsivityOwners.push_back(c);*out=c;
 Log("RR_GI25_RESPONSIVITY_READY width=%u height=%u format=R16_FLOAT owner=%p retained_until_exit=1",width,height,c);
 return true;
}

static RRResponsivityOwner* RRResponsivityFindOrCreate(ID3D12Device* device,UINT width,UINT height) noexcept {
 for(auto* c:rrResponsivityOwners)if(c&&!c->stopped&&c->device==device&&c->width==width&&c->height==height)return c;
 RRResponsivityOwner* created=nullptr;RRResponsivityCreateOwner(device,width,height,&created);return created;
}

static ID3D12Resource* RRResponsivityBeforeEvaluation(ID3D12GraphicsCommandList* list,UINT width,UINT height,
 unsigned long long frame,int bias) noexcept {
 bias=control_rr_responsivity::Normalize(bias);
 if(!list||bias==0)return nullptr;
 ID3D12Device* device=nullptr;HRESULT hr=list->GetDevice(IID_PPV_ARGS(&device));if(FAILED(hr)||!device)return nullptr;
 AcquireSRWLockExclusive(&rrResponsivityLock);
 auto* c=RRResponsivityFindOrCreate(device,width,height);
 ReleaseSRWLockExclusive(&rrResponsivityLock);device->Release();
 if(!c)return nullptr;
 const float value=control_rr_responsivity::MaskValue(bias);
 __try {
  D3D12_RESOURCE_BARRIER toUav{};toUav.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;toUav.Transition.pResource=c->mask;
  toUav.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;toUav.Transition.StateBefore=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;toUav.Transition.StateAfter=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  list->ResourceBarrier(1,&toUav);
  ID3D12DescriptorHeap* heaps[]{c->heap};list->SetDescriptorHeaps(1,heaps);
  const float clear[4]{value,0.0f,0.0f,0.0f};
  list->ClearUnorderedAccessViewFloat(c->heap->GetGPUDescriptorHandleForHeapStart(),c->heap->GetCPUDescriptorHandleForHeapStart(),c->mask,clear,0,nullptr);
  D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=c->mask;list->ResourceBarrier(1,&u);
  std::swap(toUav.Transition.StateBefore,toUav.Transition.StateAfter);list->ResourceBarrier(1,&toUav);
  const auto n=rrResponsivityUpdates.fetch_add(1,std::memory_order_relaxed)+1;
  if(n<=4||(n&(n-1))==0||frame%240ull==0)
   Log("RR_GI25_RESPONSIVITY frame=%llu count=%llu bias=%d value=%.3f mask=%p width=%u height=%u preset_scope=F full_frame=1",
    frame,n,bias,double(value),c->mask,width,height);
  return c->mask;
 } __except(EXCEPTION_EXECUTE_HANDLER){
  const auto n=rrResponsivityFailures.fetch_add(1,std::memory_order_relaxed)+1;
  Log("RR_GI25_RESPONSIVITY_FAILED frame=%llu count=%llu exception=0x%08lX action=mask_off_for_frame",frame,n,GetExceptionCode());
  return nullptr;
 }
}
