#define CONTROL_NRD_GI
#include "../src/relax_quality.h"
#include "../src/rt_surface_source_capture.h"
#include <cassert>
int main(){using namespace control_relax;
 assert(requested==Profile::Balanced);
 using namespace rt_surface_source;
 assert(manualGI==4&&EffectiveRequest()==0);
 unsigned manual=Cycle(manualGI.load());manualGI=manual;
 assert(manual==0&&requested==Profile::Balanced&&EffectiveRequest()==7);
 manual=Cycle(manual);assert(manual==0&&requested==Profile::Original);
 manual=Cycle(manual);assert(manual==4&&requested==Profile::Original);
 manual=Cycle(manual);assert(manual==0&&requested==Profile::Balanced);
 manual=Cycle(1);assert(manual==0&&requested==Profile::Balanced);
 assert(!Valid(static_cast<Profile>(99)));
 manualGI=0;manualContact=2;ResetManual();
 assert(manualGI==4&&EffectiveRequest()==0&&manualContact==0);
 request=7;assert(EffectiveRequest()==7);request=8;assert(EffectiveRequest()==8);
 request=0;assert(EffectiveRequest()==0);
}
