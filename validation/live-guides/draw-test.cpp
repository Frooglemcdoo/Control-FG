#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
using UINT=unsigned;using DWORD=unsigned long;
struct RRAlbedoLastError {~RRAlbedoLastError(){}};
enum class RRDiffuseStage {Idle,Recording,Joined,Stopped};
static std::atomic<RRDiffuseStage> rrDiffuseStage{RRDiffuseStage::Recording};
static bool skinResponsivity=false;
namespace control_rr {static bool RRUserSkinResponsivity(){return skinResponsivity;}}
struct RRAlbedoNativeTarget {void* nativeTexture=nullptr;};
namespace control_rr_albedo {
struct PreparedReplay {std::vector<int>batches;};
}
struct RRDiffuseReplay {
 std::uintptr_t manager=2;void* primaryView=reinterpret_cast<void*>(3);
 unsigned long long frame=42;control_rr_albedo::PreparedReplay replay{},characterReplay{};UINT width=2560,height=1440,targetCount=1;
 RRAlbedoNativeTarget targets[2],characterTarget{};std::atomic<bool> fault{false};
 std::atomic<UINT> drawCalls{0},frameSkips{0},rangeSkips{0},viewSkips{0},batches{0},characterMaskBatches{0};
 std::atomic<long long> drawTicks{0},characterDrawTicks{0};
};
static RRDiffuseReplay* rrDiffuse;
struct RRDiffuseViewSnapshot {void* tls=nullptr;void* context=nullptr;void* state=nullptr;void* view=nullptr;bool worker=false;};
struct RRAlbedoNativeBindings {void* staticTlsBlock=nullptr;void* commandContext=nullptr;void* deviceState=nullptr;bool workerContext=false;UINT width=2560,height=1440;};
static int rrAlbedoAPI=0;
static bool otherView=false,viewOkay=true,bindingsOkay=true,contextChanged=false,bindOkay=true,drawOkay=true,restoreOkay=true,extentOkay=true;
static int bindings=0,binds=0,draws=0,restores=0,evidence=0;
static const char* lastFault=nullptr;
static bool ReadEngineFrameSafe(unsigned long long* frame,DWORD*){*frame=42;return true;}
namespace control_rr_albedo {
struct DrawRange {struct {UINT replayBatches=5;}coverage;void* managerView(){return this;}};
static bool prepareDrawRange(const PreparedReplay& replay,int,int,DrawRange& out){out.coverage.replayBatches=replay.batches.size()==1?2u:5u;return true;}
}
static bool RRDiffuseReadView(RRDiffuseViewSnapshot* out,DWORD*){out->view=reinterpret_cast<void*>(otherView?4:3);return viewOkay;}
static bool RRAlbedoNativeReadBindings(int*,RRAlbedoNativeBindings* out,const char** reason,DWORD*){
 ++bindings;*reason="original_depth_not_depth_write";if(contextChanged)out->deviceState=reinterpret_cast<void*>(7);if(!extentOkay)out->width=1;return bindingsOkay;
}
static void RRDiffuseDepthEvidence(unsigned long long,int,int,const RRAlbedoNativeBindings*){++evidence;}
static void RRDiffuseDrawFault(RRDiffuseReplay* c,const char* reason,int,int,DWORD,DWORD=0){c->fault=true;lastFault=reason;}
template<class... A>static void Log(const char*,A...){}
static long long RRGuideQpc(){return 1;}
static bool RRAlbedoNativeBind(int*,const RRAlbedoNativeBindings*,RRAlbedoNativeTarget* const*,UINT,bool* changed,DWORD*){++binds;*changed=true;return bindOkay;}
static bool RRAlbedoDrawSafe(void*,DWORD*){++draws;return drawOkay;}
static bool RRAlbedoNativeRestore(int*,const RRAlbedoNativeBindings*,DWORD*){++restores;return restoreOkay;}
#include "../../src/rr_diffuse_draw.h"
static void reset(RRDiffuseReplay& c){c.fault=false;c.batches=0;c.characterMaskBatches=0;c.viewSkips=0;c.replay.batches={1,2};c.characterReplay.batches.clear();c.characterTarget.nativeTexture=nullptr;skinResponsivity=false;bindings=binds=draws=restores=evidence=0;lastFault=nullptr;otherView=false;viewOkay=bindingsOkay=bindOkay=drawOkay=restoreOkay=extentOkay=true;contextChanged=false;}
static void run(){RRDiffuseDraw(reinterpret_cast<void*>(2),0,5);}
int main(){RRDiffuseReplay c;rrDiffuse=&c;
 reset(c);otherView=true;bindingsOkay=false;run();assert(c.viewSkips==1&&!c.fault&&bindings==0&&binds==0&&draws==0&&restores==0&&c.batches==0);
 reset(c);bindingsOkay=false;run();assert(c.fault&&bindings==1&&evidence==1&&binds==0&&!std::strcmp(lastFault,"original_depth_not_depth_write"));
 reset(c);viewOkay=false;run();assert(c.fault&&bindings==0&&!std::strcmp(lastFault,"draw_view_read_failed"));
 reset(c);contextChanged=true;run();assert(c.fault&&binds==0&&!std::strcmp(lastFault,"view_binding_context_changed"));
 reset(c);extentOkay=false;run();assert(c.fault&&binds==0);
 reset(c);run();assert(!c.fault&&c.batches==5&&bindings==1&&binds==1&&draws==1&&restores==1);
 reset(c);skinResponsivity=true;c.characterReplay.batches={1};c.characterTarget.nativeTexture=reinterpret_cast<void*>(9);run();assert(!c.fault&&c.batches==5&&c.characterMaskBatches==2&&bindings==1&&binds==2&&draws==2&&restores==2);
 reset(c);bindOkay=false;run();assert(c.fault&&c.batches==0&&draws==0&&restores==1&&!std::strcmp(lastFault,"bind_failed"));
 reset(c);drawOkay=false;run();assert(c.fault&&c.batches==0&&restores==1&&!std::strcmp(lastFault,"draw_failed"));
 reset(c);restoreOkay=false;run();assert(c.fault&&c.batches==0&&restores==1&&!std::strcmp(lastFault,"restore_failed"));
 puts("PASS: actual draw orchestration rejects other views before bindings, retains primary depth rejection, checks context, restores after bind/draw failure, counts only successful replay");
}
