#include "../src/rr_hit_origin_shared.h"
#include <fstream>
#include <cassert>
#include <iostream>
#include <limits>
int main(int argc,char** argv){
 assert(argc==2);std::ifstream input(argv[1]);assert(input);
 unsigned count=0;double worst=0;uint x,y;
 while(input>>x>>y){
  float ix,iy,depth;RRHDClipToView m{};input>>ix>>iy>>depth;
  RRHDVector4* columns[4]{&m.column0,&m.column1,&m.column2,&m.column3};
  for(auto* c:columns)input>>c->x>>c->y>>c->z>>c->w;
  double reference[3];for(auto& v:reference)input>>v;assert(input);
  auto primary=RRHDPrimaryFromClip(x,y,ix,iy,depth,m);assert(primary.valid);
  float actual[3]{primary.position.x,primary.position.y,primary.position.z};
  for(unsigned i=0;i<3;++i){double error=std::abs(actual[i]-reference[i])/(1+std::abs(reference[i]));assert(error<2e-6);if(error>worst)worst=error;}
  RRHDVector hit{primary.position.x+3,primary.position.y+4,primary.position.z};
  auto distance=RRHDFromView(7,primary.position,hit,{{1,0,0},{0,1,0},{0,0,1}});
  assert(distance.status==RRHDHit&&std::abs(distance.distance-5)<2e-6);++count;
 }
 assert(count==2048);
 RRHDClipToView perspective{{2,0,0,0},{0,3,0,0},{0,0,0,1},{.25f,-.125f,1,0}};
 auto p=RRHDPrimaryFromClip(0,0,1,1,.5f,perspective);
 assert(p.valid&&p.position.x==.5f&&p.position.y==-.25f&&p.position.z==2);
 assert(!RRHDPrimaryFromClip(0,0,1,1,0,perspective).valid);
 assert(!RRHDPrimaryFromClip(0,0,0,1,.5f,perspective).valid);
 assert(!RRHDPrimaryFromClip(0,0,1,-1,.5f,perspective).valid);
 assert(!RRHDPrimaryFromClip(0,0,1,1,1.01f,perspective).valid);
 assert(!RRHDPrimaryFromClip(0,0,1,1,-.1f,perspective).valid);
 const auto nan=std::numeric_limits<float>::quiet_NaN();
 assert(!RRHDPrimaryFromClip(0,0,1,1,nan,perspective).valid);
 perspective.column0.w=nan;assert(!RRHDPrimaryFromClip(0,0,1,1,.5f,perspective).valid);
 auto nonHit=RRHDFromView(RRHDMissMarker,{nan,nan,nan},{nan,nan,nan},{});
 assert(nonHit.status==RRHDMiss&&nonHit.distance==0);
 std::cout<<"PASS "<<count<<" native-SSA fixtures; perspective divide, asymmetric offsets, depth/finite guards and distance integration; worst error "<<worst<<'\n';
}
