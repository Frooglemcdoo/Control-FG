// Execute the actual scheduler's extent/camera admission block under host mocks.
#include "rr_dimensions.h"
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
using UINT64=std::uint64_t;
enum class RRGuideStage {Waiting,Prepared,Finished};
static RRGuideStage rrGuideStage=RRGuideStage::Waiting;
static std::atomic<bool> rrGuideInactive{false};
static std::atomic<std::uint64_t> rrGuideHighResolutionExtent{0};
static constexpr UINT64 kRRGuideMaxPixels=8388608;
static unsigned logs=0;template<class... T>static void Log(const char*,T...){++logs;}
static bool RRGuideCameraValid(bool camera){return camera;}
struct Input {struct {struct {std::uint64_t Width;unsigned Height;} desc;} gbuffer1;bool camera;};
static bool continued=false,accepted=false;
static void Run(Input input,bool ready=true){
 const char* reason=nullptr;
#include "high-resolution.inc"
 continued=true;accepted=ready;(void)reason;
}
static void Reset(){rrGuideStage=RRGuideStage::Waiting;rrGuideInactive=false;rrGuideHighResolutionExtent=0;continued=accepted=false;logs=0;}
int main(){
 for(unsigned width:{7680u,8192u}){
  Reset();Run({{{width,4320}},true});
  assert(rrGuideStage==RRGuideStage::Finished&&rrGuideInactive&&!continued&&logs==1);
  assert(control_rr::IndependentDiffuseBootstrap(rrGuideHighResolutionExtent,true,false));
 }
 Reset();Run({{{3840,2160}},true});assert(continued&&accepted&&!rrGuideInactive&&!rrGuideHighResolutionExtent);
 Reset();Run({{{7680,4320}},false});assert(continued&&!accepted&&!rrGuideHighResolutionExtent);
 Reset();Run({{{7680,4320}},true},false);assert(continued&&!accepted&&!rrGuideHighResolutionExtent);
 Reset();Run({{{8193,4320}},true});assert(continued&&!accepted&&!rrGuideHighResolutionExtent);
 Reset();rrGuideStage=RRGuideStage::Prepared;Run({{{7680,4320}},true});assert(continued&&!accepted&&!rrGuideHighResolutionExtent);
 puts("PASS actual high-resolution scheduler: 8K independent startup, 4K diagnostic retained, invalid camera/input/extent and existing owner rejected");
}
