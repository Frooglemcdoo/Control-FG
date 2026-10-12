#include "../src/shaders/pt_direct_light.hlsli"
#include <cstdio>
#include <cstdlib>
using namespace control_pt;
void check(bool x){if(!x){std::puts("FAIL direct lighting");std::exit(1);}}
int main(){
    PTSurfaceMaterial m={PTV(.5f,.25f,.125f),PTV(0,0,0),.5f,1};
    PTDirectSample l={PTV(0,0,1),PTV(2,2,2),.25f,1};
    auto a=PTEvaluateDirect(m,PTV(0,0,1),l,1);
    check(a.valid&&fabs(a.radiance.x-4/PTTransportPi)<1e-6f);
    check(fabs(a.radiance.y-2/PTTransportPi)<1e-6f);
    auto blocked=PTEvaluateDirect(m,PTV(0,0,1),l,0);
    check(blocked.valid&&PTMax(blocked.radiance)==0);
    check(!PTEvaluateDirect(m,PTV(0,0,1),l,2).valid);
    l.delta=0;auto area=PTEvaluateDirect(m,PTV(0,0,1),l,1);
    float expected=(4/PTTransportPi)*(.25f*.25f)/(.25f*.25f+1/(PTTransportPi*PTTransportPi));
    check(area.valid&&fabs(area.radiance.x-expected)<1e-6f);
    check(fabs(PTPowerWeight(1e30f,1e30f)-.5f)<1e-6f);
    check(fabs(PTPowerWeight(.1f,.9f)+PTPowerWeight(.9f,.1f)-1)<1e-6f);
    l.pdf=0;check(!PTEvaluateDirect(m,PTV(0,0,1),l,1).valid);
    std::puts("PASS: light selection probability, occlusion, invalid visibility and MIS");
}
