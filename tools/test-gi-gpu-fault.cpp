#include "gpu_test_device.h"
#include <atomic>
#include <cstdarg>
#include <cstdio>
static std::atomic<unsigned> stalls{0};
static void Log(const char* fmt,...)noexcept {
 char text[2048];va_list args;va_start(args,fmt);vsnprintf(text,sizeof(text),fmt,args);va_end(args);
 if(strstr(text,"GI_FAULT_R467 stall "))++stalls;
 std::puts(text);
}
#include "../src/gi_gpu_fault.h"
int main()try {
 using namespace gi_reuse;
 SetEnvironmentVariableA("CONTROLTOH_GPU_FAULT","1");gi_gpu_fault::BeforeDevice();
 Require(gi_gpu_fault::requested.load(),"fault diagnostics not requested");
 Device d(false);gi_gpu_fault::Attach(d.device.Get());Require(gi_gpu_fault::device!=nullptr,"observer attachment failed");
 Microsoft::WRL::ComPtr<ID3D12Fence> fence;Check(d.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
 Check(gi_gpu_fault::Signal(d.queue.Get(),fence.Get(),1));
 gi_gpu_fault::BeginPresent(77);Sleep(4200);
 Require(stalls.load()==1,"stalled present not reported exactly once");
 gi_gpu_fault::EndPresent();Sleep(600);
 Require(stalls.load()==1,"completed present reported again");
 Require(SUCCEEDED(d.device->GetDeviceRemovedReason()),"observer damaged healthy device");
 Require(gi_gpu_fault::issued.load()==1&&gi_gpu_fault::fence.load()->GetCompletedValue()==1,"observer fence tracking mismatch");
 std::puts("PASS live WARP fault observer: bounded stall report, fence progress, healthy device preserved. No actual GPU fault injected.");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
