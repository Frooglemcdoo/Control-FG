#include "../src/gi_reuse/triangle_sample_validation.h"
#include <vector>
#include "../src/gi_reuse/sample_diversity.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace gi_reuse;
int main(){
 for(unsigned width:{2u,4u}){const unsigned first=width==4?65536:0,count=first+3;
  std::vector<unsigned char> v(count*16),idx(3*width);const float positions[3][3]={{0,0,0},{1,0,0},{0,1,0}};
  for(unsigned i=0;i<3;++i){std::memcpy(v.data()+(first+i)*16,positions[i],12);auto id=first+i;for(unsigned j=0;j<width;++j)idx[i*width+j]=static_cast<unsigned char>(id>>(j*8));}
  TriangleSampleSummary s;assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size()-4,count,16,s)&&s.triangles==1&&!s.outOfRange&&!s.nonfinite&&!s.degenerate);
  assert(!ValidateTriangleSample(idx.data(),idx.size()-1,width,v.data(),v.size(),count,16,s));
  assert(!ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size()-5,count,16,s));
  assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size(),count-1,16,s)&&s.outOfRange==1);
  float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(v.data()+first*16,&nan,4);
  assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size(),count,16,s)&&s.nonfinite==1);
  assert(s.inactive==1&&s.firstNonfinite.present);
  std::memcpy(v.data()+first*16,positions[0],12);
  assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size(),count,16,s)&&s.first.indices[2]==first+2&&s.first.bits[1][0]==0x3f800000u);
  const float line[3][3]={{0,0,0},{1,0,0},{2,0,0}};
  for(unsigned i=0;i<3;++i)std::memcpy(v.data()+(first+i)*16,line[i],12);
  assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size(),count,16,s)&&s.collinear==1&&!s.coincidentPositions&&!s.repeatedIndices&&s.firstDegenerate.present);
  std::memset(v.data(),0,v.size());assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size(),count,16,s)&&s.degenerate==1&&s.coincidentPositions==1&&s.zeroPositions==1);
  std::memcpy(idx.data()+width,idx.data(),width);assert(ValidateTriangleSample(idx.data(),idx.size(),width,v.data(),v.size(),count,16,s)&&s.repeatedIndices==1&&!s.coincidentPositions);
 }
 SampleDiversity<2> diversity;SampleKey key{1,2,0,0,12,3,3,2,0};
 assert(diversity.Reserve(key)&&diversity.Reserve(key)&&!diversity.Reserve(key));
 key.geometry=1;assert(diversity.Reserve(key));key.geometry=2;assert(!diversity.Reserve(key));
 key.geometry=1;assert(diversity.Reserve(key));key.vertexOwner=0;assert(!diversity.Reserve(key));
 std::puts("PASS paired triangles: uint16/uint32 high indices, padded stride, exact extent, truncation, range, NaN and degeneracy");
}
