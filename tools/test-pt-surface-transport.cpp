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
    const auto ng=PTV(0,0,1),ns=PTV(.6f,0,.8f),view=PTV(0,0,1);
    require(PTValidPrimaryShadingNormal(ns,ng,view),"Tilted native shading normal accepted");
    require(!PTValidPrimaryShadingNormal(PTV(0,0,-1),ng,view),"Reversed raster normal rejected");
    require(!PTValidPrimaryShadingNormal(PTV(0,0,2),ng,view),"Non-unit shading normal rejected");
    require(!PTValidPrimaryShadingNormal(ns,ng,PTV(0,0,-1)),"Back-side camera rejected");
    // This ray is above the shading normal but below the triangle. It must
    // terminate as zero contribution rather than leaking through the surface.
    const auto leak=PTUnit(PTV(1,0,-.1f));require(PTDot(ns,leak)>0,"Leak fixture above shading normal");
    require(!PTPrimaryReflectionAboveGeometry(ng,leak),"Geometric back hemisphere clipped");
    require(PTPrimaryReflectionAboveGeometry(ng,view),"Front reflection retained");

    const auto primary=PTPrimaryShadingNormal(ns,ng,view);
    require(PTDot(primary,ns)>.99999f,"Primary detail preserved away from shading horizon");
    const auto grazingView=PTUnit(PTV(-1,0,.1f));
    const auto corrected=PTPrimaryShadingNormal(ns,ng,grazingView);
    require(PTValidPrimaryShadingNormal(corrected,ng,grazingView),"Grazing raster normal keeps valid PT frame");
    require(PTDot(PTPrimaryShadingNormal(PTV(0,0,2),ng,view),ng)==0,"Malformed primary normal still rejects");
    require(PTDot(PTPrimaryShadingNormal(ns,ng,PTV(0,0,-1)),ng)==0,"Geometric back side still rejects");
    auto previous=PTPrimaryShadingNormal(PTUnit(PTV(.1f,0,1)),ng,grazingView);
    for(int i=1;i<=1000;++i){
        const auto candidate=PTPrimaryShadingNormal(PTUnit(PTV(.1f+i*.001f,0,1)),ng,grazingView);
        require(PTValidPrimaryShadingNormal(candidate,ng,grazingView),"Continuous grazing sweep remains owned by PT");
        require(PTDot(previous,candidate)>.99999f,"No hard geometric-normal switch at shading horizon");
        previous=candidate;
    }
    const auto secondary=PTSecondaryShadingNormal(ns,ng,view);
    require(fabs(secondary.x-.6f)<1e-6f&&fabs(secondary.z-.8f)<1e-6f,"Secondary interpolation retained");
    const auto back=PTSecondaryShadingNormal(PTScale(ns,-1),ng,view);
    require(fabs(back.x-.6f)<1e-6f&&fabs(back.z-.8f)<1e-6f,"Back-face mesh normal oriented to geometry");
    for(auto bad: {PTV(0,0,0),PTV(0,0,2),PTV(NAN,0,1),PTV(INFINITY,0,1),PTV(1,0,0)}){
        const auto n=PTSecondaryShadingNormal(bad,ng,view);
        require(n.x==0&&n.z==1,"Invalid or tangent secondary frame uses geometry");
    }
    const auto grazing=PTSecondaryShadingNormal(ns,ng,PTUnit(PTV(-1,0,.1f)));
    require(grazing.x==0&&grazing.z==1,"Viewer outside shading hemisphere retains geometric reflection");

    // Retaining native specular fill must remove that lobe only from the first
    // continuation, while preserving its diffuse throughput and direct BSDF.
    PTSurfaceMaterial original={PTV(.4f,.2f,.1f),PTV(.9f,.9f,.9f),.2f,3u};
    const auto first=PTPrimaryDiffuseContinuation(original);
    require(original.lobes==3&&first.lobes==1,"Primary continuation leaves direct material intact");
    for(int i=0;i<10000;++i){auto sample=PTSampleSurface(first,PTV(0,0,1),random01(),random01(),random01());
        require(sample.valid&&fabs(sample.weight.x-.4f)<1e-5f&&fabs(sample.weight.y-.2f)<1e-5f,"Native fill adds no duplicated first specular bounce");}

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
    // Independent quadrature catches a changed proposal with the OLD PDF:
    // that error would brighten glossy energy instead of only reducing noise.
    for(float rough: {.3f,.6f,.9f})for(float nz:{1.f,.2f}){
        PTSurfaceMaterial m={PTV(.96f,.96f,.96f),PTV(.04f,.04f,.04f),rough,3u};
        PTVec3 wo=PTV(sqrt(1-nz*nz),0,nz);double sum=0;
        for(int i=0;i<350000;++i){auto sample=PTSamplePrimarySurface(m,wo,random01(),random01(),random01());
            if(sample.valid)sum+=sample.weight.x;}
        float reference=integrate(m,wo);
        require(fabs(float(sum/350000)-reference)<.004f+.025f*reference,"Balanced primary proposal preserves quadrature energy");
    }
    require(PTSampleSurfaceProposal(diffuse,view,.5f,.5f,.5f,.5f).fault!=0,"Unsupported lobe proposals rejected");
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
    // Early roulette preserves the expected four-hit sum while reducing work.
    // Include direct light before termination, just as the production integrator.
    double radiance=0;unsigned continuations=0;unsigned rng=1234567;
    for(unsigned i=0;i<200000;++i){auto q=PTBeginTransport();
        for(unsigned depth=0;depth<4;++depth){q=PTAccumulate(q,PTV(1,1,1));if(depth==3)break;
            PTSurfaceSample step{};step.valid=1;step.weight=PTV(.3f,.3f,.3f);
            rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
            q=PTContinue(q,step,float(rng>>8)*(1.0f/16777216.0f),1);
            if(!q.alive)break;++continuations;
        }radiance+=q.radiance.x;
    }
    require(std::abs(radiance/200000-(1+.3+.09+.027))<.01,"Early roulette preserves four-hit energy");
    require(continuations<200000,"Early roulette reduces secondary traversal work");
    std::puts("PASS: shared transport math, 12 integration cases, accumulation and roulette");
}
