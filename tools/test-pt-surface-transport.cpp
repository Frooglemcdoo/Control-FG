#include "../src/shaders/pt_surface_transport.hlsli"
#include <cstdio>
#include <cstdlib>
using namespace control_pt;
static void require(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static uint seed=0x92185a3u;
static float random01(){seed=1664525u*seed+1013904223u;return float(seed>>8)*(1.0f/16777216.0f);}
static float integrate(PTSurfaceMaterial m,PTVec3 wo) {
    // Independent numerical hemisphere integral of f*cos, not sampling-PDF math.
    const int nz=512,np=512;double sum=0;
    for(int z=0;z<nz;++z) for(int j=0;j<np;++j) {
        float c=(z+0.5f)/nz,phi=2*PTTransportPi*(j+0.5f)/np;
        PTVec3 wi=PTV(sqrt(1-c*c)*cos(phi),sqrt(1-c*c)*sin(phi),c);
        sum+=PTEvaluateSurface(m,wo,wi).f.x*c;
    }
    return float(sum*(2*PTTransportPi)/(nz*np));
}
int main(){
    PTSurfaceMaterial diffuse={PTV(.7f,.3f,.1f),PTV(0,0,0),.5f,1u};
    for(int i=0;i<10000;++i){
        auto s=PTSampleSurface(diffuse,PTV(0,0,1),random01(),random01(),random01());
        require(s.valid&&fabs(s.weight.x-.7f)<1e-5f&&fabs(s.weight.y-.3f)<1e-5f,"Lambertian throughput equals reflectance");
    }
    for(uint lobes: {2u,3u}) for(float rough: {.3f,.6f,.9f}) for(float nz: {1.0f,.2f}) {
        PTSurfaceMaterial m={PTV(.4f,.4f,.4f),PTV(.12f,.12f,.12f),rough,lobes};
        PTVec3 wo=PTV(sqrt(1-nz*nz),0,nz);double sum=0;int valid=0;
        const int count=350000;
        for(int i=0;i<count;++i){
            auto s=PTSampleSurface(m,wo,random01(),random01(),random01());
            if(s.valid){require(PTValid(s.weight)&&s.pdf>0,"Finite sample");sum+=s.weight.x;++valid;}
        }
        float estimate=float(sum/count),reference=integrate(m,wo);
        std::printf("lobes=%u rough=%.1f nz=%.1f estimate=%.6f integral=%.6f accepted=%.3f\n",lobes,rough,nz,estimate,reference,float(valid)/count);
        require(fabs(estimate-reference)<.004f+.025f*reference,"Sample estimate agrees with independent quadrature");
    }
    auto p=PTBeginTransport();p=PTAccumulate(p,PTV(2,3,4));
    auto s=PTSampleSurface(diffuse,PTV(0,0,1),.5f,.2f,.4f);
    p=PTContinue(p,s,.5f,99);p=PTAccumulate(p,PTV(1,1,1));
    require(fabs(p.radiance.x-2.7f)<1e-5f&&fabs(p.radiance.y-3.3f)<1e-5f,"Accumulation uses path throughput once");
    s.weight=PTV(.25f,.25f,.25f);s.valid=1;double mean=0;
    for(int i=0;i<100000;++i){auto q=PTContinue(PTBeginTransport(),s,(i+.5f)/100000,1);if(q.alive)mean+=q.throughput.x;}
    require(fabs(mean/100000-.25)<1e-6,"Roulette preserves expected throughput");
    s.valid=0;require(PTContinue(PTBeginTransport(),s,.1f,3).alive==0,"Invalid sample terminates");
    require(!PTSampleSurface(diffuse,PTV(0,0,-1),.5f,.5f,.5f).valid,"Back-facing path rejected");
    require(!PTAccumulate(PTBeginTransport(),PTV(-1,0,0)).alive,"Negative contribution rejected");
    require(!PTAccumulate(PTBeginTransport(),PTV(-1,0,0)).valid,"Bad data invalidates composition admission");
    auto bad=PTSampleSurface(diffuse,PTV(0,0,1),1.0f,.5f,.5f);
    require(bad.fault&&!PTContinue(PTBeginTransport(),bad,.5f,3).valid,"Invalid random input is not physical absorption");
    auto absorbed=PTSampleSurface(diffuse,PTV(0,0,-1),.5f,.5f,.5f);
    auto ended=PTContinue(PTBeginTransport(),absorbed,.5f,3);
    require(ended.valid&&!ended.alive,"Physical termination preserves valid accumulated radiance");
    std::puts("PASS: shared transport math, 12 integration cases, accumulation and roulette");
}
