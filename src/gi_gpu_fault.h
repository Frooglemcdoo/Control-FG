#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
// Diagnostic-only, process-lifetime observer. Never waits on a GPU fence, takes
// the GI router lock, changes TDR settings or bypasses native Present.
namespace gi_gpu_fault {
inline std::atomic<bool> requested{false};
inline HRESULT settingsResult=E_PENDING;
inline ID3D12Device* device=nullptr;
inline std::atomic<ID3D12Fence*> fence{nullptr};
inline std::atomic<UINT64> issued{0},presentStart{0},presentFrame{0},recordedFrame{0},retiredFrame{0};
inline std::atomic<HRESULT> signalResult{S_OK};
inline void BeforeDevice()noexcept {
 char value[4]{};
 if(GetEnvironmentVariableA("CONTROLTOH_GPU_FAULT",value,4)!=1||value[0]!='1')return;
 requested=true;
 Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> settings;
 settingsResult=D3D12GetDebugInterface(IID_PPV_ARGS(&settings));
 if(SUCCEEDED(settingsResult)){
  settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
 }
}
inline void DumpRemoved(ID3D12Device* d)noexcept {
 Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> dred;
 const auto query=d->QueryInterface(IID_PPV_ARGS(&dred));
 Log("GI_FAULT_R467 dred_interface=0x%08lX",static_cast<unsigned long>(query));
 if(FAILED(query))return;
 D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT crumbs{};
 const auto hr=dred->GetAutoBreadcrumbsOutput(&crumbs);
 Log("GI_FAULT_R467 breadcrumbs_hr=0x%08lX",static_cast<unsigned long>(hr));
 if(SUCCEEDED(hr)){
  unsigned nodes=0;
  for(auto* n=crumbs.pHeadAutoBreadcrumbNode;n&&nodes<1024;n=n->pNext,++nodes){
   const UINT done=n->pLastBreadcrumbValue?*n->pLastBreadcrumbValue:0;
   if(done>=n->BreadcrumbCount)continue;
   Log("GI_FAULT_R467 breadcrumb node=%u list=%p queue=%p completed=%u total=%u",nodes,n->pCommandList,n->pCommandQueue,done,n->BreadcrumbCount);
   const UINT first=done>8?done-8:0;
   for(UINT i=first;n->pCommandHistory&&i<n->BreadcrumbCount&&i-first<16;++i)
    Log("GI_FAULT_R467 operation node=%u index=%u opcode=%u",nodes,i,unsigned(n->pCommandHistory[i]));
  }
  Log("GI_FAULT_R467 breadcrumb_nodes=%u truncated=%u",nodes,unsigned(nodes==1024));
 }
 D3D12_DRED_PAGE_FAULT_OUTPUT page{};
 const auto ph=dred->GetPageFaultAllocationOutput(&page);
 Log("GI_FAULT_R467 page_fault_hr=0x%08lX address=0x%llX",static_cast<unsigned long>(ph),page.PageFaultVA);
 if(SUCCEEDED(ph))for(unsigned kind=0;kind<2;++kind){
  unsigned count=0;
  for(auto* a=kind?page.pHeadRecentFreedAllocationNode:page.pHeadExistingAllocationNode;a&&count<64;a=a->pNext,++count)
   Log("GI_FAULT_R467 allocation freed=%u type=%u name=%s",kind,unsigned(a->AllocationType),a->ObjectNameA?a->ObjectNameA:"unnamed");
 }
}
inline DWORD WINAPI Watch(void*)noexcept {
 UINT64 reported=0;unsigned reports=0;bool removedReported=false;
 for(;;){
  Sleep(500);
  const auto reason=device->GetDeviceRemovedReason();
  const auto start=presentStart.load();const auto frame=presentFrame.load();
  const auto now=GetTickCount64();
  const bool stalled=start&&now>=start&&now-start>=3000;
  const bool removed=FAILED(reason);
  if((removed&&!removedReported)||(stalled&&reported!=start&&reports<4)){
   auto* f=fence.load();const auto completed=f?f->GetCompletedValue():0;
   Log("GI_FAULT_R467 stall present=%llu elapsed_ms=%llu device_reason=0x%08lX issued=%llu completed=%llu signal_hr=0x%08lX recorded=%llu retired=%llu",frame,start?now-start:0,static_cast<unsigned long>(reason),issued.load(),completed,static_cast<unsigned long>(signalResult.load()),recordedFrame.load(),retiredFrame.load());
   reported=start;++reports;
   if(removed){removedReported=true;DumpRemoved(device);}
  }
 }
}
inline void Attach(ID3D12Device* d)noexcept {
 if(!requested.load()||device||!d)return;
 // Called once under the existing GI initialization lock. Retained intentionally
 // until process exit; the observer must remain valid while rendering is hung.
 d->AddRef();device=d;
 HMODULE pinned=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&Watch),&pinned)){
  device=nullptr;d->Release();Log("GI_FAULT_R467 watcher=0 reason=module_pin");return;
 }
 HANDLE thread=CreateThread(nullptr,0,&Watch,nullptr,0,nullptr);
 Log("GI_FAULT_R467 watcher=%u dred_settings_hr=0x%08lX requested_before_factory=1",unsigned(thread!=nullptr),static_cast<unsigned long>(settingsResult));
 if(thread)CloseHandle(thread);else{device=nullptr;d->Release();}
}
inline HRESULT Signal(ID3D12CommandQueue* q,ID3D12Fence* f,UINT64 value){
 const auto hr=q->Signal(f,value);
 if(requested.load()){
  ID3D12Fence* expected=nullptr;f->AddRef();if(!fence.compare_exchange_strong(expected,f))f->Release();
  signalResult=hr;if(SUCCEEDED(hr))issued=value;
 }
 return hr;
}
inline void BeginPresent(UINT64 frame)noexcept{if(requested.load()){presentFrame=frame;presentStart=GetTickCount64();}}
inline void EndPresent()noexcept{if(requested.load())presentStart=0;}
}
