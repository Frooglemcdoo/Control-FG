#include "../src/rr_hit_distance_shared.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
static void need(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static RRHDBasis identity(){return {{1,0,0},{0,1,0},{0,0,1}};}
int main(){
 const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
 const RRHDVector invalid{nan,inf,-inf};
 for(uint id=0;id<65536;++id) {
  const auto status=RRHDClassify(id);
  need(status==(id<65534?RRHDHit:id==65534?RRHDNoRay:RRHDMiss),"all 16-bit material classes");
 }
 for(uint id: {65534u,65535u,65536u,0xffffffffu}) {
  auto r=RRHDFromView(id,invalid,invalid,identity());
  need(r.status==RRHDClassify(id)&&r.distance==0,"non-hit never fabricates distance from positions");
 }
 auto a=RRHDFromView(7,{1,2,3},{4,6,3},identity());need(a.status==RRHDHit&&a.distance==5,"3-4-5 separation");
 a=RRHDFromView(0,{1,2,3},{1,2,3},identity());need(a.status==RRHDHit&&a.distance==0,"zero-distance hit remains a hit");
 a=RRHDFromView(0,invalid,{1,2,3},identity());need(a.status==RRHDInvalid,"invalid primary");
 a=RRHDFromView(0,{1,2,3},invalid,identity());need(a.status==RRHDInvalid,"invalid hit");
 auto b=identity();b.row1.x=nan;a=RRHDFromView(0,{0,0,0},{1,2,3},b);need(a.status==RRHDInvalid,"invalid transform");
 a=RRHDFromView(1,{0,0,0},{1e30f,0,0},identity());need(a.status==RRHDHit&&a.distance==1e30f,"large finite separation avoids squared overflow");
 a=RRHDFromView(1,{0,0,0},{1e-30f,0,0},identity());need(a.status==RRHDHit&&a.distance==1e-30f,"small finite separation avoids squared underflow");
 a=RRHDFromView(1,{-3e38f,0,0},{3e38f,0,0},identity());need(a.status==RRHDInvalid,"subtraction overflow rejected");
 std::mt19937 gen(812731);std::uniform_real_distribution<float> pos(-1000,1000),mat(-3,3);
 double worst=0;
 for(unsigned i=0;i<100000;++i){
  RRHDVector p{pos(gen),pos(gen),pos(gen)},h{pos(gen),pos(gen),pos(gen)};
  RRHDBasis m{{mat(gen),mat(gen),mat(gen)},{mat(gen),mat(gen),mat(gen)},{mat(gen),mat(gen),mat(gen)}};
  auto r=RRHDFromView(i%65534,p,h,m);
  const double delta[3]={double(h.x)-p.x,double(h.y)-p.y,double(h.z)-p.z};
  const double rows[3][3]={{m.row0.x,m.row0.y,m.row0.z},{m.row1.x,m.row1.y,m.row1.z},{m.row2.x,m.row2.y,m.row2.z}};
  double w[3]={};for(int j=0;j<3;++j)for(int k=0;k<3;++k)w[j]+=rows[j][k]*delta[k];
  double ref=std::hypot(w[0],w[1],w[2]);double err=std::abs(double(r.distance)-ref)/(1+ref);if(err>worst)worst=err;
  need(r.status==RRHDHit&&err<2e-5,"independent double matrix/distance reference");
 }
 std::printf("PASS 65536 material classifications; 100000 independent matrix/distance cases; invalid/miss/no-ray/zero/extreme cases; worst relative error %.9g\n",worst);
}
