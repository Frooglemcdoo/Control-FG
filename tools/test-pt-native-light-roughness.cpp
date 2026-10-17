#define PT_DIRECT_LIGHT_ROUGHNESS 1
#include "../src/shaders/pt_native_light_roughness.hlsli"
#include "../src/shaders/pt_light_reservoir.hlsli"
#include <cstdio>
#include <cstdlib>
using namespace control_pt;
static void check(bool b,const char* m){if(!b){std::fprintf(stderr,"FAIL: %s\n",m);std::exit(1);}}
// Independent double-precision circle segment and simplified variance formula.
static double reference(double r,double radius,double distance,double cosine){
    if(radius>0&&distance>0){
        double h=std::max(-1.0,std::min(1.0,-distance*cosine/radius));
        double fraction=(std::acos(h)-h*std::sqrt(std::max(0.0,1-h*h)))/3.14159265358979323846;
        double area=std::min(1.0,fraction*radius*radius/(2*distance*distance));
        if(area>.000333333359){double b=1/(1+3*area);r=std::sqrt(r*r+(1-b*b)/(6*b-2*b*b*b));}
    }
    return std::min(1.0,std::max(.05,r));
}
int main(){
    for(float r:{0.f,.05f,.2f,.8f,1.f})for(float radius:{0.f,.01f,.1f,1.f,8.f})
    for(float distance:{.01f,1.f,2.f,50.f})for(float c:{-1.f,-.1f,0.f,.1f,1.f}){
        float got=PTNativeLightRoughness(r,radius,distance,c);
        check(std::isfinite(got)&&std::abs(double(got)-reference(r,radius,distance,c))<.0002,"native radius formula vs independent geometric reference");
    }
    check(PTNativeLightRoughness(.2f,0,0,1)==.2f,"zero-radius directional material retained");
    check(PTNativeLightRoughness(.2f,.01f,50,1)==.2f,"subthreshold source retains roughness");
    check(PTNativeLightRoughness(.1f,1,2,1)>.35f,"finite source broadens glossy lobe");
    check(PTNativeLightRoughness(.1f,1,2,-1)==.1f,"fully hidden source does not broaden");
    check(PTNativeLightRoughness(.1f,-1,2,1)<0&&PTNativeLightRoughness(.1f,NAN,2,1)<0,"invalid source rejected");
    PTSurfaceMaterial m={PTV(.4f,.3f,.2f),PTV(.04f,.04f,.04f),.05f,3};
    PTDirectSample light;light.wi=PTUnit(PTV(.4f,0,1));light.incidentRadiance=PTV(1,2,3);light.pdf=1;light.delta=1;
    light.roughness=m.roughness;auto narrow=PTEvaluateDirect(m,PTV(0,0,1),light,1);
    light.roughness=PTNativeLightRoughness(m.roughness,1,2,light.wi.z);
    auto broad=PTEvaluateDirect(m,PTV(0,0,1),light,1);
    check(broad.valid&&broad.radiance.x>narrow.radiance.x,"off-axis floor highlight recovered");
    auto diffuse=m;diffuse.lobes=1;auto db=PTEvaluateDirect(diffuse,PTV(0,0,1),light,1);
    light.roughness=m.roughness;auto dn=PTEvaluateDirect(diffuse,PTV(0,0,1),light,1);
    check(std::abs(db.radiance.x-dn.radiance.x)<1e-7,"diffuse irradiance unchanged");
    light.roughness=PTNativeLightRoughness(m.roughness,1,2,light.wi.z);auto targetMaterial=m;targetMaterial.roughness=light.roughness;
    check(std::abs(PTLightTarget(targetMaterial,PTV(0,0,1),light.wi,light.incidentRadiance)-PTDot(broad.radiance,PTV(.2126f,.7152f,.0722f)))<1e-6,"RIS and final lighting use same lobe");
    check(m.roughness==.05f,"path material roughness unchanged");
    auto blocked=PTEvaluateDirect(m,PTV(0,0,1),light,0);check(blocked.valid&&PTMax(blocked.radiance)==0,"shadow rejection retained");
    light.roughness=-1;check(PTEvaluateDirect(m,PTV(0,0,1),light,1).valid==0,"invalid effective roughness rejected");
    puts("PASS: native light radius formula, point limit, horizon, invalid inputs, floor highlight, diffuse invariance, RIS consistency and shadow retention");
}
