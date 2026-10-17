#include "../src/shaders/pt_emitter_sampling.hlsli"
#include <cassert>
#include <cstdio>
#include <cmath>
using namespace control_pt;
int main(){
    const auto a=PTV(0,0,2),b=PTV(2,0,2),c=PTV(0,2,2);
    double x=0,y=0;const unsigned n=256;
    for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){
        auto s=PTSampleEmitterTriangle(a,b,c,4,(i+.5f)/n,(j+.5f)/n);
        assert(s.valid&&std::abs(s.area-2)<1e-6f&&std::abs(s.pdfArea-.125f)<1e-6f);
        assert(s.position.x>=0&&s.position.y>=0&&s.position.x+s.position.y<=2.000001f);
        assert(std::abs(s.position.z-2)<1e-6f&&s.normal.z==1);x+=s.position.x;y+=s.position.y;
    }
    assert(std::abs(x/(n*n)-2.0/3)<.0001&&std::abs(y/(n*n)-2.0/3)<.0001);
    assert(PTEmitterSolidAnglePdf(2,4,9,.5f)==2.25f);
    assert(PTEmitterSolidAnglePdf(2,8,9,.5f)==1.125f);
    assert(PTEmitterSolidAnglePdf(2,4,9,0)==0);
    assert(!PTSampleEmitterTriangle(a,a,a,4,.5f,.5f).valid);
    assert(!PTSampleEmitterTriangle(a,b,c,0,.5f,.5f).valid);
    assert(!PTSampleEmitterTriangle(a,b,c,4,1,.5f).valid);
    assert(std::abs(PTPowerWeight(2.25f,.3f)+PTEmitterHitWeight(.3f,2.25f)-1)<1e-6f);
    PTSurfaceMaterial m{PTV(.7f,.3f,.1f),PTV(.04f,.04f,.04f),.4f,3};
    auto wo=PTV(0,0,1),wi=PTUnit(PTV(.2f,.3f,1));auto le=PTV(3,2,1);
    auto d=PTEvaluateEmitterDirect(m,wo,wi,le,2.25f,.5f,1,1);
    auto e=PTEvaluateSurfaceProposal(m,wo,wi,.5f);
    float w=PTPowerWeight(2.25f,e.pdf),k=wi.z*w/2.25f;
    assert(d.valid&&std::abs(d.radiance.x-e.f.x*le.x*k)<1e-6f);
    assert(std::abs(d.diffuse.x-m.kd.x/PTTransportPi*le.x*k)<1e-6f);
    auto blocked=PTEvaluateEmitterDirect(m,wo,wi,le,2.25f,.5f,0,1);
    assert(blocked.valid&&PTMax(blocked.radiance)==0);
    auto last=PTEvaluateEmitterDirect(m,wo,wi,le,2.25f,.5f,1,0);
    assert(last.valid&&std::abs(last.radiance.x-e.f.x*le.x*wi.z/2.25f)<1e-6f);
    assert(!PTEvaluateEmitterDirect(m,wo,wi,le,0,.5f,1,1).valid);
    assert(!PTEvaluateEmitterDirect(m,wo,wi,le,2.25f,.5f,2,1).valid);
    // Monte Carlo integral: MIS light+BSDF estimators preserve a constant
    // Lambertian emitter over the hemisphere, including unequal proposal PDFs.
    double integral=0;
    for(unsigned i=0;i<100000;++i){
        double z=(i+.5)/100000,pl=1/(2*PTTransportPi),pb=z/PTTransportPi;
        double wl=pl*pl/(pl*pl+pb*pb),wb=pb*pb/(pl*pl+pb*pb);
        integral+=2*z*(wl+wb)/100000;
    }
    assert(std::abs(integral-1)<1e-8);
    puts("PASS: triangle sampling, area/solid-angle PDFs, light-selection probability, MIS complement, primary proposal, diffuse split, shadow and last-bounce energy");
}
