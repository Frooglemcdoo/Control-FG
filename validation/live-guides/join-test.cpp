#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using UINT=unsigned;using DWORD=unsigned long;
struct RRAlbedoLastError {~RRAlbedoLastError(){}};
struct {long long QuadPart=1000;} frequency;
struct Resource {};struct Queue {};
using ID3D12Resource=Resource;
constexpr unsigned D3D12_RESOURCE_STATE_RENDER_TARGET=4;
static bool skinResponsivity=false;
namespace control_rr {static bool RRUserSkinResponsivity(){return skinResponsivity;}}
static bool RRAlbedoNativeTextureState(void*,unsigned state){return state==D3D12_RESOURCE_STATE_RENDER_TARGET;}
static Resource resource;static Queue queue;
static void* primary=reinterpret_cast<void*>(3);
static unsigned long long engineFrame=42;
static bool wrongView=false,frameReadable=true;
static const char* stopReason=nullptr;
static bool ReadEngineFrameSafe(unsigned long long* f,DWORD*){*f=engineFrame;return frameReadable;}
static bool RRAlbedoReadCallback(void*,std::uintptr_t* r,std::uintptr_t* m){*r=1;*m=2;return true;}
namespace control_rr_albedo {static bool ReadJoinView(void*,void** p,DWORD*){*p=wrongView?reinterpret_cast<void*>(4):primary;return true;}}
template<class... A> static void Log(const char*,A...){}
enum class RRDiffuseStage {Idle,Recording,Joined,Stopped};
struct RRDiffuseReplay {
 std::uintptr_t renderer=1,manager=2;void* primaryView=primary;Queue* queue=nullptr;
 unsigned long long frame=42,joined=0;UINT accepted=10,rejected=2,width=2560,height=1440;
 std::atomic<UINT> frameSkips{0},rangeSkips{0},viewSkips{0},drawCalls{0};
 std::atomic<UINT> batches{10};std::atomic<bool> fault{false};std::atomic<long long> drawTicks{1};
 std::atomic<long long> characterDrawTicks{0};
 std::atomic<UINT> characterMaskBatches{0},characterBatches{0};
 struct {Resource* resource=nullptr;} targets[2];
 struct {void* nativeTexture=nullptr;Resource* resource=nullptr;} characterTarget;
};
static RRDiffuseReplay* rrDiffuse=nullptr;static std::atomic<RRDiffuseStage> rrDiffuseStage{RRDiffuseStage::Recording};
static thread_local void* rrDiffuseJoinView=nullptr;
static void RRDiffuseStop(const char* reason){stopReason=reason;rrDiffuseStage.store(RRDiffuseStage::Stopped);}
struct RRGuideInputSnapshot {
 unsigned long long engineFrame=42;Queue* queue=nullptr;struct{void* renderer=reinterpret_cast<void*>(1);} camera;
 struct{struct{unsigned long long Width=2560;UINT Height=1440;}desc;}gbuffer1;
};
static void RRPerfReplayJoined(unsigned long long,double,unsigned) {}
#include "../../src/rr_diffuse_join.h"
int main(){
 RRDiffuseReplay c;c.queue=&queue;c.targets[0].resource=&resource;rrDiffuse=&c;RRGuideInputSnapshot in;in.queue=&queue;
 assert(!RRDiffuseCurrent(&in));wrongView=true;RRDiffuseJoinComplete(nullptr);assert(rrDiffuseStage==RRDiffuseStage::Recording);
 wrongView=false;auto* old=RRDiffuseBeginJoin(nullptr);assert(!old&&rrDiffuseJoinView==primary);RRDiffuseEndJoin(nullptr,old);
 assert(!rrDiffuseJoinView&&rrDiffuseStage==RRDiffuseStage::Joined&&c.joined==1);assert(RRDiffuseCurrent(&in)==&resource);
 skinResponsivity=true;c.characterBatches=1;c.characterMaskBatches=1;c.characterTarget.nativeTexture=reinterpret_cast<void*>(7);c.characterTarget.resource=&resource;
 assert(RRDiffuseCharacterCurrent(42,2560,1440)==&resource);c.characterMaskBatches=0;assert(!RRDiffuseCharacterCurrent(42,2560,1440));skinResponsivity=false;c.characterBatches=0;c.characterMaskBatches=0;
 in.engineFrame=43;assert(!RRDiffuseCurrent(&in));in.engineFrame=42;in.gbuffer1.desc.Width=1280;assert(!RRDiffuseCurrent(&in));in.gbuffer1.desc.Width=2560;
 in.queue=nullptr;assert(!RRDiffuseCurrent(&in));in.queue=&queue;
 rrDiffuseStage=RRDiffuseStage::Recording;c.batches=9;RRDiffuseJoinComplete(nullptr);assert(rrDiffuseStage==RRDiffuseStage::Stopped);assert(!RRDiffuseCurrent(&in));assert(!std::strcmp(stopReason,"join_batch_count_mismatch"));
 rrDiffuseStage=RRDiffuseStage::Recording;c.batches=10;c.fault=true;RRDiffuseJoinComplete(nullptr);assert(rrDiffuseStage==RRDiffuseStage::Stopped);
 assert(!std::strcmp(stopReason,"join_draw_fault"));
 rrDiffuseStage=RRDiffuseStage::Recording;c.fault=false;engineFrame=43;RRDiffuseJoinComplete(nullptr);assert(rrDiffuseStage==RRDiffuseStage::Stopped);
 assert(!std::strcmp(stopReason,"join_frame_mismatch"));
 rrDiffuseStage=RRDiffuseStage::Recording;frameReadable=false;RRDiffuseJoinComplete(nullptr);
 assert(rrDiffuseStage==RRDiffuseStage::Stopped&&!std::strcmp(stopReason,"join_frame_read_failed"));frameReadable=true;
 rrDiffuseStage=RRDiffuseStage::Recording;engineFrame=42;c.batches=11;RRDiffuseJoinComplete(nullptr);
 assert(rrDiffuseStage==RRDiffuseStage::Stopped&&!std::strcmp(stopReason,"join_batch_count_mismatch"));
 rrDiffuseStage=RRDiffuseStage::Joined;RRDiffuseDisableForLiveFailure();assert(rrDiffuseStage==RRDiffuseStage::Stopped);
 puts("PASS: actual recurring join/readiness gates; wrong view, incomplete batches, draw faults, stale frame, extent and queue mismatches reject; nested join view restored");
}
