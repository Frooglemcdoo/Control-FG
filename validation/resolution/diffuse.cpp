#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>
#include <array>
#include <set>
#include "rr_dimensions.h"
using UINT=unsigned;using DWORD=unsigned long;using HRESULT=int;
#define __try try
#define __except(x) catch(...)
#define EXCEPTION_EXECUTE_HANDLER 1
#define IID_PPV_ARGS(p) p
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)
constexpr unsigned D3D12_FENCE_FLAG_NONE=0;
static DWORD GetLastError(){return 0;}static void SetLastError(DWORD){}
static DWORD thread=7;static DWORD GetCurrentThreadId(){return thread;}static DWORD GetExceptionCode(){return 9;}
struct ID3D12Fence {std::uint64_t completed=0;auto GetCompletedValue(){return completed;}};
static ID3D12Fence fence;
struct Device {int CreateFence(int,unsigned,ID3D12Fence** out){*out=&fence;return 0;}void Release(){}};
using ID3D12Device=Device;static Device device;
struct ID3D12CommandQueue {
 unsigned signals=0;std::uint64_t scheduled=0;bool fail=false;
 void AddRef(){}int GetDevice(Device** p){*p=&device;return 0;}
 int Signal(ID3D12Fence* f,std::uint64_t n){assert(f==&fence);++signals;scheduled=n;return fail?-1:0;}
};
static ID3D12CommandQueue queue;
template<class T>void RRGuideRelease(T*& p){if(p)p->Release();p=nullptr;}
struct Resource {};
struct Native {Resource resource;bool destroyed=false;};
static unsigned creates=0,destroys=0,deletes=0;static std::set<Native*> allocations;
static void destroy(void* p){auto* n=static_cast<Native*>(p);assert(allocations.count(n)&&!n->destroyed);n->destroyed=true;++destroys;}
static void deleteMemory(void* p){auto* n=static_cast<Native*>(p);assert(n->destroyed);allocations.erase(n);delete n;++deletes;}
struct RRAlbedoNativeTarget {void* nativeTexture=nullptr;Resource* resource=nullptr;unsigned width=0,height=0;DWORD creationThread=0;bool everRecorded=false;};
struct {void (*destroy)(void*)=&::destroy;void (*deleteMemory)(void*)=&::deleteMemory;}rrAlbedoAPI;
static bool RRAlbedoNativeCreate(decltype(rrAlbedoAPI)*,UINT w,UINT h,RRAlbedoNativeTarget* out,const char**,DWORD*){
 auto* n=new Native;allocations.insert(n);++creates;*out={n,&n->resource,w,h,thread,false};return true;
}
static bool RRAlbedoNativePrepareClear(decltype(rrAlbedoAPI)*,RRAlbedoNativeTarget* target,DWORD*){target->everRecorded=true;return true;}
struct RRAlbedoLastError {~RRAlbedoLastError(){}};
static std::atomic<std::uint64_t> rrGuideHighResolutionExtent{0};
enum class RRAlbedoStage {Waiting,Finished};
static std::atomic<RRAlbedoStage> rrAlbedoStage{RRAlbedoStage::Finished};
// The diagnostic intentionally retains the original 1440p dimensions.
struct Diagnostic {unsigned width=2560,height=1440;};[[maybe_unused]] static Diagnostic diagnostic;[[maybe_unused]] static Diagnostic* rrAlbedoCapture=&diagnostic;
static unsigned long long frame=100,present=100;
static void* verifiedRenderer=reinterpret_cast<void*>(1);
struct CameraSnapshot {unsigned long long engineFrame;void* renderer;};
struct RRGuideInputSnapshot {unsigned long long engineFrame=0,presentToken=0;ID3D12CommandQueue* queue=nullptr;struct{Resource* resource=nullptr;}gbuffer1,gbuffer2;};
static bool RRGuideResourcesShareDevice(RRGuideInputSnapshot*,void*,void*){return true;}
static bool ReadEngineFrameSafe(unsigned long long* out,DWORD*){*out=frame;return true;}
static bool ReadCamera(CameraSnapshot* out,DWORD*,const char**){*out={frame,verifiedRenderer};return true;}
static bool RRGuideReadRendererContext(RRGuideInputSnapshot* out,const char**){out->engineFrame=frame;out->presentToken=present;out->queue=&queue;return true;}
static long long RRGuideQpc(){return 1;}static double RRGuideElapsedMs(long long){return 0;}
template<class... A>void Log(const char*,A...){}
namespace control_rr { static bool RRUserSkinResponsivity(){return false;} static bool RRUserRuntimeWorkRequested(){return true;} }
namespace RRAlbedoShader {struct Result {unsigned targetCount=1;};struct Session {
 bool ValidatePair(std::uintptr_t,std::uintptr_t,Result&,const char**){return true;}
};}
static bool RRAlbedoRead(std::uintptr_t,void* out,std::size_t bytes){std::memset(out,0,bytes);return true;}
static void RRAlbedoGetShader(){}static void RRAlbedoValidateEyeCandidate(){}
namespace control_rr_albedo {
struct OpaqueBatch {
 std::uintptr_t shader=0,mesh=0,material=0;
 std::int32_t instanceByteOffset=0,instanceCount=1;
 std::uint8_t flags=0,tessellated=0;
 std::uint16_t opaqueMetadata=0;
 std::uint32_t instanceBufferIndex=0;
};
enum class MaterialAdmission : std::uint8_t { Unsupported=0, Standard, Character, Cloth, Foliage, Hair, Eye };
struct PreparedReplay {std::array<unsigned char,64> manager{};std::vector<OpaqueBatch>batches;std::vector<unsigned>sourceIndices;std::vector<std::uint8_t>familyKinds;
 unsigned sourceFirst=0,sourceEnd=0;
 struct{unsigned sourceBatches=1,replayBatches=0,rejectedBatches=0;bool complete=false;}coverage;
 struct{} rejectionAudit;
};
struct PreparationState {std::uintptr_t renderer=1,manager=2;void* primaryView=nullptr;};
struct Access {decltype(&RRAlbedoRead)read;decltype(&RRAlbedoGetShader)get;void* null;RRAlbedoShader::Session* shaders;decltype(&RRAlbedoValidateEyeCandidate)eye;};
static bool ReadPreparationState(std::uintptr_t,void*,void*,PreparationState*,const char**,DWORD*){return true;}
static bool prepare(const Access&,std::uintptr_t,PreparedReplay& out,bool){out.batches.resize(1);out.sourceIndices.resize(1);out.familyKinds.resize(1);return true;}
}
static void RRPerfReplayPrepared(unsigned long long,double) {}
#include "diffuse-prepare.inc"
static void prepare(unsigned w,unsigned h){RRDiffuseRequestExtent(frame,w,h);RRDiffusePrepare(nullptr,nullptr);}
static void joined(){assert(rrDiffuseStage==RRDiffuseStage::Recording);rrDiffuseStage=RRDiffuseStage::Joined;++frame;++present;}
int main(){
 assert(!rrDiffuseJoinView);(void)&RRDiffuseDrawFault;
 prepare(2560,1440);assert(rrDiffuse&&rrDiffuse->width==2560&&creates==1);joined();
 // Reproduce the user's 1440p -> 4K change while the diagnostic is still 1440p.
 prepare(3840,2160);assert(rrDiffuseStage==RRDiffuseStage::Idle&&rrDiffuse->resizePending&&destroys==0);
 RRDiffuseAfterPresent(rrDiffuse->lastUsePresent);assert(queue.signals==0);
 thread=8;RRDiffuseAfterPresent(present);assert(queue.signals==0);thread=7;
 RRDiffuseAfterPresent(present);assert(queue.signals==1);prepare(3840,2160);assert(destroys==0&&creates==1);
 fence.completed=queue.scheduled;prepare(3840,2160);assert(rrDiffuse->width==3840&&destroys==1&&deletes==1&&creates==2);joined();
 // Rapid changes coalesce. Never allocate the intermediate 1440p request.
 prepare(2560,1440);RRDiffuseAfterPresent(present);fence.completed=queue.scheduled;
 prepare(7680,4320);assert(rrDiffuse->width==7680&&rrDiffuse->height==4320&&creates==3);joined();
 for(unsigned n=0;n<1000;++n){
  const unsigned w=n%2?7680:2560,h=n%2?4320:1440;
  prepare(w,h);assert(rrDiffuse->resizePending);RRDiffuseAfterPresent(present);
  prepare(w,h);assert(rrDiffuse->resizePending&&allocations.size()==1);
  fence.completed=queue.scheduled;prepare(w,h);assert(!rrDiffuse->resizePending&&allocations.size()==1);joined();
 }
 // CPU jobs must join before the next size can replace targets.
 prepare(1920,1080);RRDiffuseAfterPresent(present);fence.completed=queue.scheduled;prepare(1920,1080);
 DWORD fault=0;const auto before=deletes;assert(!RRDiffuseResize(rrDiffuse,2560,1440,&fault));assert(deletes==before);
 joined();prepare(2560,1440);RRDiffuseAfterPresent(present);fence.completed=UINT64_MAX;prepare(2560,1440);
 assert(rrDiffuseStage==RRDiffuseStage::Stopped&&deletes==before);
 // No guessed deletion after a device-removal sentinel. Fixture cleanup only.
 for(auto* n:allocations)delete n;
 allocations.clear();delete rrDiffuse;rrDiffuse=nullptr;
 puts("PASS actual diffuse preparation/resize: stale diagnostic size, CPU join, primary thread, post-Present fence, 1000 bounded cycles, coalesced 8K, device loss");
}
