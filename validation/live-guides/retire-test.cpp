#include <cassert>
#include <cstdio>
#include <cstdint>
#define __except(x) catch(...)
using DWORD=unsigned long;using UINT=unsigned;using HRESULT=long;
#define FAILED(x) ((x)<0)
#include "../../src/rr_frame_slots.h"
#include "../../src/rr_live_capture_policy.h"
struct RRLiveCapture {control_rr::LiveCapturePolicy policy;};
static unsigned exports=0;
static DWORD RRLiveCaptureExport(void*){return 0;}
constexpr unsigned WT_EXECUTEDEFAULT=0;
static bool QueueUserWorkItem(DWORD(*)(void*),void*,unsigned){++exports;return true;}
static DWORD lastError=9;
static DWORD GetLastError(){return lastError;}static void SetLastError(DWORD x){lastError=x;}
static bool rrLiveLock=false;
static bool TryAcquireSRWLockExclusive(bool* l){if(*l)return false;*l=true;return true;}
static void ReleaseSRWLockExclusive(bool* l){*l=false;}
template<class... A> static void Log(const char*,A...){lastError=99;}
struct Fence {std::uint64_t completed=0;std::uint64_t GetCompletedValue(){return completed;}};
struct Queue {bool fail=false;unsigned calls=0;HRESULT Signal(Fence*,std::uint64_t){++calls;return fail?-1:0;}};
struct RRLiveSlot {void* sources[4]{};unsigned long long present=0;};
struct RRLiveOwner {
 RRLiveCapture* capture=nullptr;
 Fence* fence;Queue* queue;bool prepared=true,stopped=false;
 control_rr::FrameSlots policy;RRLiveSlot slots[3]{};
 unsigned long long recorded=0,retired=0,skipped=0,signal=0,diffuseCopied=0;
 control_rr::FenceKey Key(){return {1,2,3};}
};
static RRLiveOwner* rrLive=nullptr;static unsigned releases=0;
static void RRLiveReleaseSources(RRLiveSlot& s){for(auto*& p:s.sources){assert(p);p=nullptr;++releases;}}
static void RRDiffuseDisableForLiveFailure() {}
#include "../../src/rr_live_retire.h"
static control_rr::Lease ready(RRLiveOwner& c,unsigned frame,unsigned present){control_rr::Lease l{};assert(c.policy.Begin({frame,1,2560,1440},l));assert(c.policy.CommandsStarted(l));assert(c.policy.GuidesReady(l));auto& s=c.slots[l.index];s.present=present;for(auto& p:s.sources)p=reinterpret_cast<void*>(1);return l;}
int main(){
 Fence f;Queue q;RRLiveOwner c{};c.fence=&f;c.queue=&q;rrLive=&c;assert(c.policy.Bind(c.Key()));
 RRLiveCapture cap;c.capture=&cap;
 auto l=ready(c,1,10);assert(cap.policy.Begin(l.index,l.serial));cap.policy.Recorded(true);RRLiveAfterPresent(10);assert(q.calls==0&&releases==0);
 RRLiveAfterPresent(11);assert(q.calls==1&&releases==0&&exports==0);assert(c.policy.Inspect()[l.index].state==control_rr::SlotState::Submitted);
 RRLiveAfterPresent(12);assert(releases==0&&q.calls==1);
 f.completed=1;RRLiveAfterPresent(13);assert(releases==4&&c.retired==1);assert(lastError==9&&exports==1);RRLiveAfterPresent(13);assert(exports==1);
 l=ready(c,2,13);q.fail=true;RRLiveAfterPresent(14);assert(c.stopped&&releases==4);
 f.completed=2;RRLiveAfterPresent(15);assert(releases==4); // uncertain signal never retires
 Fence lost;Queue q2;RRLiveOwner c2{};c2.fence=&lost;c2.queue=&q2;rrLive=&c2;assert(c2.policy.Bind(c2.Key()));
 ready(c2,1,1);lost.completed=UINT64_MAX;RRLiveAfterPresent(2);assert(c2.stopped&&q2.calls==0&&releases==4);
 puts("PASS: actual retirement path waits for later native Present and completed fence; releases all four sources once; failed signal/device loss retain resources");
}
