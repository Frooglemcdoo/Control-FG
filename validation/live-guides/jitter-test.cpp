#include <initializer_list>
#include <cmath>
#include <cassert>
#include <cstdio>
#include "../../src/shaders/rr_jitter_ray.hlsli"
int main(){
 unsigned cases=0;
 for(unsigned w: {640u,1280u,2560u,3840u})for(unsigned h:{360u,720u,1440u,2160u})
 for(int ix=-8;ix<=8;++ix)for(int iy=-8;iy<=8;++iy)for(unsigned corner=0;corner<5;++corner){
  float jx=float(ix)/16,jy=float(iy)/16,sx=1.25f,sy=2.0f;
  float px=corner==0?.5f:corner==1?w-.5f:corner==2?w*.25f:corner==3?w*.75f:w*.5f;
  float py=corner==0?.5f:corner==1?h-.5f:corner==2?h*.75f:corner==3?h*.25f:h*.5f;
  double vx=ControlRayX(px,float(w),sx,jx),vy=ControlRayY(py,float(h),sy,jy);
  // Independent forward projection with native P*T offsets, then viewport.
  double ndcX=vx*sx+2.0*double(jx)/w,ndcY=vy*sy-2.0*double(jy)/h;
  assert(std::fabs((ndcX+1)*w*.5-px)<.001);assert(std::fabs((1-ndcY)*h*.5-py)<.001);++cases;
 }
 assert(ControlRayX(.5f,1,1,.25f)==-.5f);
 assert(ControlRayY(.5f,1,1,.25f)==.5f);
 assert(ControlRayX(.5f,1,1,0)==0&&ControlRayY(.5f,1,1,0)==0);
 std::printf("PASS: %u shared shader inverse-ray / independent forward-projection cases, positive/negative/zero jitter and both axes\n",cases);
}
