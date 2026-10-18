#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
struct RRGuideInputTexture {void* nativeTexture;ID3D12Resource* resource;D3D12_RESOURCE_DESC desc;void* stateTracker;unsigned trackedState,nativeFlags,manualStateDisabled;};
#include "../src/rr_input_capture_native.h"
struct RRInputCaptureJob{};
struct RRGuideInputSnapshot {void* commandList;void* commandContext;void* queue;void* recordingLock;unsigned long long engineFrame,presentToken;};
static RRGuideInputSnapshot current{};static unsigned acquired=0,released=0,records=0;static void* busy=nullptr;
static bool RRGuideReadRendererContext(RRGuideInputSnapshot* out,const char**){*out=current;return true;}
static bool RRGuideTryAcquireNativeLock(void* p){if(p==busy)return false;++acquired;return true;}
static void RRGuideReleaseNativeLock(void*){++released;}
static bool RRInputCaptureRecord(RRInputCaptureJob*,const RRGuideInputSnapshot*,DWORD*,bool* started) {assert(acquired-released>=2&&acquired-released<=6);++records;*started=true;return true;}
#include "../src/rr_input_capture_guard.h"
int main(){
 IDXGIFactory4* factory=nullptr;IDXGIAdapter* adapter=nullptr;ID3D12Device* device=nullptr;
 assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));assert(SUCCEEDED(D3D12CreateDevice(adapter,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=64;desc.Height=64;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;
 ID3D12Resource* resources[3]{};alignas(16) unsigned char wrappers[3][256]{};void* pointers[3]{wrappers[0],wrappers[1],wrappers[2]};
 const DXGI_FORMAT formats[]{DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R32_TYPELESS,DXGI_FORMAT_R16G16_FLOAT};
 for(unsigned i=0;i<3;++i){desc.Format=formats[i];assert(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&resources[i]))));
  *reinterpret_cast<ID3D12Resource**>(wrappers[i]+0x88)=resources[i];*reinterpret_cast<unsigned*>(wrappers[i]+0x60)=0xc0;
 }
 RRGuideInputTexture inputs[3]{};RRInputCaptureSetNativeTextures(pointers,3,42);
 assert(RRInputCaptureFindNative(42,resources,inputs));assert(!RRInputCaptureFindNative(41,resources,inputs));
 RRInputCaptureJob job{};int marker=0;current={&marker,&marker,&marker,&marker,42,42};DWORD fault=0;bool started=false;
 assert(RRInputCaptureRecordGuarded(&job,&current,inputs,&fault,&started));assert(started&&records==1&&acquired==released);
 started=false;busy=inputs[1].stateTracker;assert(!RRInputCaptureRecordGuarded(&job,&current,inputs,&fault,&started));assert(!started&&records==1&&acquired==released);busy=nullptr;
 *reinterpret_cast<unsigned*>(wrappers[0]+0x60)=D3D12_RESOURCE_STATE_RENDER_TARGET;
 assert(!RRInputCaptureRecordGuarded(&job,&current,inputs,&fault,&started));assert(!started&&records==1&&acquired==released);
 assert(!RRInputCaptureFindNative(42,resources,inputs));
 *reinterpret_cast<unsigned*>(wrappers[0]+0x60)=0xc0;wrappers[0][0x68]=1;assert(!RRInputCaptureFindNative(42,resources,inputs));wrappers[0][0x68]=0;
 assert(RRInputCaptureFindNative(42,resources,inputs));auto stale=current;stale.engineFrame=41;
 assert(!RRInputCaptureRecordGuarded(&job,&stale,inputs,&fault,&started));assert(!started&&records==1&&acquired==released);
 *reinterpret_cast<unsigned*>(wrappers[1]+0x60)=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
 assert(RRInputCaptureFindNative(42,resources,inputs));
 RRGuideInputTexture five[]{inputs[0],inputs[1],inputs[2],inputs[1],inputs[2]};
 started=false;assert(RRInputCaptureRecordGuarded(&job,&current,five,&fault,&started,5));assert(started&&acquired==released);
 started=false;busy=inputs[2].stateTracker;assert(!RRInputCaptureRecordGuarded(&job,&current,five,&fault,&started,5));assert(!started&&acquired==released);busy=nullptr;
 // Output copies accept only explicitly tracked UAV, never read-only or RT.
 RRGuideInputTexture output{};
 assert(!RRInputCaptureFindOutput(42,resources[0],&output));
 *reinterpret_cast<unsigned*>(wrappers[0]+0x60)=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
 assert(RRInputCaptureFindOutput(42,resources[0],&output));
 assert(!RRInputCaptureReadNative(wrappers[0],resources[0],&output));
 assert(RRInputCaptureFindOutput(42,resources[0],&output));
 started=false;assert(RRInputCaptureRecordGuarded(&job,&current,&output,&fault,&started,1,true));assert(started&&acquired==released);
 RRInputCaptureSetNativeTextures(nullptr,0,0);assert(!RRInputCaptureFindNative(42,resources,inputs));
 for(auto* r:resources)r->Release();device->Release();adapter->Release();factory->Release();
 puts("PASS: real WARP resource formats; wrapper scope/frame matching; native state rejection, lock release and pre-record revalidation");
}
