#include "../../src/rr_reflectance_projection.h"
#include <cassert>
#include <cstdio>
using namespace control_rr_reflectance;
int main() {
    double m[16]={1.199023685885926,0,0,0,0,2.1315976796788418,0,0,0,0,1.0000444650650024,1,0,0,-0.20000889599336524,0};
    float sx=0,sy=0;assert(SupportedProjection(m,&sx,&sy));
    unsigned char g[]={128,128,128,0};
    for(unsigned y=0;y<1440;y+=17) for(unsigned x=0;x<2560;x+=19) {
        float v=PixelViewCosine(g,x,y,2560,1440,sx,sy);assert(std::isfinite(v) && v>=-1 && v<=1);
        // Independent homogeneous perspective round trip for a point on each ray.
        double px=(2*(double(x)+.5)/2560-1)/sx,py=(1-2*(double(y)+.5)/1440)/sy;
        double clipX=px*sx,clipY=py*sy,clipW=1;
        assert(std::fabs((clipX/clipW+1)*1280-.5-x)<1e-9);
        assert(std::fabs((1-clipY/clipW)*720-.5-y)<1e-9);
    }
    m[8]=.01;assert(!SupportedProjection(m,&sx,&sy));m[8]=0;
    m[0]=0;assert(!SupportedProjection(m,&sx,&sy));m[0]=1;
    m[15]=1;assert(!SupportedProjection(m,&sx,&sy));m[15]=0;
    m[5]=std::numeric_limits<double>::quiet_NaN();assert(!SupportedProjection(m,&sx,&sy));
    puts("PASS: R5 projection shape, perspective pixel round trips, unsupported cameras rejected");
}
