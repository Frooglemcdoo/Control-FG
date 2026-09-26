#include "../../src/rr_projection_validation.h"
#include <cassert>
int main(){
 double p[16]={2,0,0,0,0,3,0,0,0,0,0,1,0,0,0.1,0};
 double q[16]={0.5,0,0,0,0,1.0/3,0,0,0,0,0,10,0,0,1,0};
 float out[16]{};assert(control_rr::CopyValidatedProjection(p,q,out));assert(out[0]==2&&out[11]==1);
 q[0]=2;out[0]=99;assert(!control_rr::CopyValidatedProjection(p,q,out));assert(out[0]==99);
 q[0]=0.5;p[0]=std::numeric_limits<double>::quiet_NaN();assert(!control_rr::CopyValidatedProjection(p,q,out));
 p[0]=std::numeric_limits<double>::infinity();assert(!control_rr::CopyValidatedProjection(p,q,out));
 p[0]=1e100;assert(!control_rr::CopyValidatedProjection(p,q,out));
 double zero[16]{};assert(!control_rr::CopyValidatedProjection(zero,zero,out));
 assert(!control_rr::CopyValidatedProjection(nullptr,q,out));
 double w2v[12]={1,0,0,0,1,0,0,0,1,-10,-20,-30};
 double v2w[12]={1,0,0,0,1,0,0,0,1,10,20,30};
 float affine[16]{};
 assert(control_rr::CopyValidatedAffine43(w2v,v2w,affine));
 assert(affine[0]==1&&affine[5]==1&&affine[10]==1&&affine[15]==1);
 assert(affine[12]==-10&&affine[13]==-20&&affine[14]==-30);
 v2w[9]=11;affine[12]=77;
 assert(!control_rr::CopyValidatedAffine43(w2v,v2w,affine)&&affine[12]==77);
 v2w[9]=10;w2v[0]=std::numeric_limits<double>::quiet_NaN();
 assert(!control_rr::CopyValidatedAffine43(w2v,v2w,affine));
}
