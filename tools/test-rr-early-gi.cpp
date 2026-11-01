#define CONTROL_RR_GI_SEQUENCE
#define CONTROL_GI_BOUNDARY_CAPTURE
#include "gpu_test_device.h"
#include "gi_history_gpu_fixture.h"
#include "../src/rr_capture_ui.h"
#include "../src/rr_input_capture_limits.h"
#include <cstdint>
#include <cassert>
struct RRGuideInputTexture {void* nativeTexture;ID3D12Resource* resource;D3D12_RESOURCE_DESC desc;void* stateTracker;unsigned trackedState,nativeFlags,manualStateDisabled;};
struct RRGuideInputSnapshot {ID3D12GraphicsCommandList* commandList;void* commandContext;ID3D12CommandQueue* queue;void* recordingLock;unsigned long long engineFrame,presentToken;};
struct RRInputCaptureJob {
 control_rr::InputCapturePolicy policy{};
 ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;
 ID3D12Resource* sources[kRRInputCaptureMaxResources]{},*readback[kRRInputCaptureMaxResources]{};
 D3D12_RESOURCE_DESC descriptions[kRRInputCaptureMaxResources]{};
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[kRRInputCaptureMaxResources]{};
 D3D12_RESOURCE_STATES states[kRRInputCaptureMaxResources]{};
 unsigned count=1,preGIIndex=0;
 unsigned long long frame=0,preGIFrame=0,preGIPresent=0,preGITick=0;
 bool preGICaptured=false;
};
static SRWLOCK rrInputCaptureLock=SRWLOCK_INIT;
static RRInputCaptureJob rrInputCaptures[2];
static unsigned long long rrInputCaptureSettledFrame=99;
static RRGuideInputSnapshot current{};
static RRGuideInputTexture source{};
static unsigned held=0;static bool busy=false;
static void Log(const char*,...){}
template<class T>static void RRGuideRelease(T*& p){if(p)p->Release();p=nullptr;}
static bool RRGuideReadRendererContext(RRGuideInputSnapshot* out,const char**){*out=current;return true;}
static bool RRGuideTryAcquireNativeLock(void*){if(busy)return false;++held;return true;}
static void RRGuideReleaseNativeLock(void*){assert(held);--held;}
#include "../src/rr_input_capture_native.h"
static bool RRInputCaptureReadShaderTexture(size_t offset,RRGuideInputTexture* out){assert(offset==0x128f3c8);return RRInputCaptureReadNative(source.nativeTexture,source.resource,out);}
#define RR_SEH_TRY __try
#define RR_SEH_EXCEPT(x) __except(x)
#include "../src/rr_input_capture_record.h"
#include "../src/rr_input_capture_guard.h"
#include "../src/rr_input_capture_early_gi.h"
int main(){try{
 Device gpu(false);auto image=HistoryInput(gpu,10.f,.01f,0,4,5);
 alignas(16) unsigned char wrapper[256]{};
 *reinterpret_cast<ID3D12Resource**>(wrapper+0x88)=image.resource.Get();
 *reinterpret_cast<unsigned*>(wrapper+0x60)=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 source.nativeTexture=wrapper;source.resource=image.resource.Get();
 unsigned long long context[2]{};int lock=0;
 current={gpu.list.Get(),context,gpu.queue.Get(),&lock,100,100};
 control_rr_gi::boundaryRequested=true;
 for(auto& c:rrInputCaptures){c.policy.phase=control_rr::InputCapturePolicy::Ready;c.policy.requests=1;c.queue=gpu.queue.Get();Check(image.resource->GetDevice(IID_PPV_ARGS(&c.device)));c.descriptions[0]=image.resource->GetDesc();
  UINT64 bytes=0;gpu.device->GetCopyableFootprints(&c.descriptions[0],0,1,0,&c.footprint[0],nullptr,nullptr,&bytes);
  auto rb=gpu.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);c.readback[0]=rb.Detach();
 }
 RRInputCaptureAtGI(99,gpu.list.Get());Require(!rrInputCaptures[0].preGICaptured,"stale frame admitted");
 busy=true;RRInputCaptureAtGI(100,gpu.list.Get());Require(!rrInputCaptures[0].preGICaptured&&!rrInputCaptures[0].sources[0]&&!held,"lock contention retained source");busy=false;
 current.engineFrame=101;RRInputCaptureAtGI(100,gpu.list.Get());Require(!rrInputCaptures[0].preGICaptured,"wrong context admitted");current.engineFrame=100;
 *reinterpret_cast<unsigned*>(wrapper+0x60)=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
 RRInputCaptureAtGI(100,gpu.list.Get());Require(!rrInputCaptures[0].preGICaptured,"unproven writable state admitted");
 *reinterpret_cast<unsigned*>(wrapper+0x60)=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 RRInputCaptureAtGI(100,gpu.list.Get());Require(rrInputCaptures[0].preGICaptured&&rrInputCaptures[0].preGIFrame==100&&!held,"early copy missing");
 const auto commands=context[1];RRInputCaptureAtGI(100,gpu.list.Get());Require(commands==context[1],"duplicate overwrote readback");
 rrInputCaptures[0].policy.phase=control_rr::InputCapturePolicy::Recorded;rrInputCaptures[0].frame=100;current.engineFrame=current.presentToken=101;rrInputCaptureSettledFrame=100;
 RRInputCaptureAtGI(101,gpu.list.Get());Require(rrInputCaptures[1].preGICaptured&&rrInputCaptures[1].preGIFrame==101,"second consecutive slot missing");
 gpu.Submit();
 for(auto& c:rrInputCaptures){void* ptr=nullptr;Check(c.readback[0]->Map(0,nullptr,&ptr));
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<16;++x){auto* p=reinterpret_cast<float*>(static_cast<unsigned char*>(ptr)+c.footprint[0].Offset+y*c.footprint[0].Footprint.RowPitch)+4*x;
   for(unsigned ch=0;ch<4;++ch)Require(p[ch]==PatternValue(10.f,5,x,y),"early GI copy altered mip0 pixels");}
  c.readback[0]->Unmap(0,nullptr);RRGuideRelease(c.sources[0]);RRGuideRelease(c.readback[0]);RRGuideRelease(c.device);
 }
 Require(*reinterpret_cast<unsigned*>(wrapper+0x60)==D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,"native tracker changed");
 std::cout<<"PASS R263 early GI: actual WARP copies, consecutive slots, exact mip0 pixels, frame/context/state/lock guards, no duplicate overwrite, restored native state\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
