#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
namespace gi_reuse {
struct TriangleSampleSummary {std::uint64_t vertexHash=14695981039346656037ull;std::uint32_t triangles=0,outOfRange=0,nonfinite=0,degenerate=0;};
// Object-space positions only. Does not infer TLAS identity or apply transforms.
inline bool ValidateTriangleSample(const unsigned char* indices,std::uint64_t indexBytes,std::uint32_t width,
 const unsigned char* vertices,std::uint64_t vertexBytes,std::uint32_t count,std::uint64_t stride,TriangleSampleSummary& out) noexcept {
 out={};if(!indices||!vertices||(width!=2&&width!=4)||!indexBytes||indexBytes%(3*width)||!count||stride<12||stride%4||
  std::uint64_t(count-1)>(UINT64_MAX-12)/stride||std::uint64_t(count-1)*stride+12>vertexBytes)return false;
 for(std::uint64_t i=0;i<vertexBytes;++i){out.vertexHash^=vertices[i];out.vertexHash*=1099511628211ull;}
 for(std::uint64_t base=0;base<indexBytes;base+=3*width){
  double p[3][3]{};bool range=true,finite=true;
  for(unsigned k=0;k<3;++k){const auto* b=indices+base+k*width;std::uint32_t id=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8);
   if(width==4)id|=(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);
   if(id>=count){range=false;continue;}
   float xyz[3];std::memcpy(xyz,vertices+std::uint64_t(id)*stride,12);
   for(unsigned j=0;j<3;++j){p[k][j]=xyz[j];if(!std::isfinite(xyz[j]))finite=false;}
  }
  ++out.triangles;if(!range){++out.outOfRange;continue;}if(!finite){++out.nonfinite;continue;}
  double a[3],b[3];for(unsigned j=0;j<3;++j){a[j]=p[1][j]-p[0][j];b[j]=p[2][j]-p[0][j];}
  const double x=a[1]*b[2]-a[2]*b[1],y=a[2]*b[0]-a[0]*b[2],z=a[0]*b[1]-a[1]*b[0];
  if(x==0&&y==0&&z==0)++out.degenerate;
 }
 return true;
}
}
