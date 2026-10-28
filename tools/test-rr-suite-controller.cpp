#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <new>
#define CONTROL_RR_F9_SUITE
#include "../src/rr_suite_pipelines.h"
#include "../src/rr_capture_ui.h"
#include "../src/rr_user_control.h"
#include "../src/native_lighting_live_state.h"
static HMODULE selfModule=nullptr;
static HWND foreground=reinterpret_cast<HWND>(1);
static std::atomic<bool> slFgEnabledByApi{false};
namespace native_lighting {
inline LiveSelection selection;
inline bool SelectMode(Mode m){selection.Select(m);return true;}
template<class... A> void Trace(const char*,A...){}
inline bool DrainDiagnosticLog(bool){return true;}
namespace capture_suite {inline bool active=false;}
namespace performance_capture {inline bool Active(){return false;}}
}
static DWORD NoPackage(HMODULE,wchar_t*,DWORD){return 0;}
static HWND Foreground(){return foreground;}
static BOOL NotIconic(HWND){return FALSE;}
#define GetModuleFileNameW NoPackage
#define GetForegroundWindow Foreground
#define IsIconic NotIconic
#include "../src/ui/rr_reflection_suite.h"
int main(){
 namespace s=native_lighting::reflection_suite;namespace ui=control_rr_capture_ui;using native_lighting::Mode;
 rr_suite::ready=1;native_lighting::specularSamplesReady=true;control_rr::RRUserRequest(true);
 // A previously selected RR-bypass lighting mode must not prevent starting.
 control_rr::RRSetDiagnosticBypass(true);native_lighting::SelectMode(Mode::RoomNativeNoRR);
 unsigned long long now=1;
 for(unsigned location=1;location<=2;++location){
  assert(s::Start(now));assert(!s::Start(now));assert(native_lighting::LiveSelection::GetMode(native_lighting::selection.Read())==native_lighting::kNativeBlendDefaultMode);
  for(unsigned step=1;step<=8;++step){
   rr_suite::Latch(step);rr_suite::dispatchStep=step;
   native_lighting::selection.Observe(native_lighting::selection.Read(),1); // observation bits aren't a user mode change
   now+=1001;s::Tick(foreground,now);assert(s::active&&s::waiting&&ui::requested);
   ui::requested=false;ui::Publish(control_rr::InputCapturePolicy::Idle,control_rr::InputCapturePolicy::Idle,(location-1)*8+step,true);
   s::Tick(foreground,++now);
  }
  assert(!s::active&&rr_suite::requested==0);assert(native_lighting::LiveSelection::GetMode(native_lighting::selection.Read())==Mode::RoomNativeNoRR);
 }
 assert(!s::Start(now));
 auto reset=[&](){rr_suite::session=0;rr_suite::cancelled=false;ui::completedPairs=0;ui::requested=false;ui::state=ui::State::Idle;foreground=reinterpret_cast<HWND>(1);};
 reset();assert(s::Start(now));s::Cancel("cancelled");s::Tick(foreground,++now);assert(!s::active&&rr_suite::cancelled&&rr_suite::requested==0);
 reset();assert(s::Start(now));s::Tick(nullptr,++now);assert(!s::active&&rr_suite::cancelled&&rr_suite::requested==0);
 reset();assert(s::Start(now));s::Tick(foreground,now+180001);assert(!s::active&&rr_suite::cancelled&&rr_suite::requested==0);
 reset();slFgEnabledByApi=true;assert(!s::Start(now));slFgEnabledByApi=false;control_rr::RRUserRequest(false);assert(!s::Start(now));
 puts("PASS: both automated locations, observation changes, mode restoration, cancellation, focus loss, timeout, RR/FG admission");
}
