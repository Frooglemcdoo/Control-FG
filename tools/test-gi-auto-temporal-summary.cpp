#include "../src/gi_reuse/auto_temporal_audit.h"
#include <cassert>
#include <limits>
#include <iostream>
int main(){using namespace gi_reuse;
 uint due=0;for(uint i=0;i<10000;++i)if(GIAuditPairDue(i))++due;assert(due==10);
 std::vector<GIAutoPixelAudit> a(19*17),b;
 for(auto& v:a){v.flags=7;for(float& c:v.raw)c=.0001f;for(float& c:v.weighted)c=.0001f;}
 a[161].weighted[0]=a[161].weighted[1]=a[161].weighted[2]=.0004f;
 auto small=GIAuditSummarize(a.data(),19,17);assert(small.spikes==0&&small.small2==1&&small.smallExamples[0].index==161);
 b=a;auto t=GIAuditTemporal(a.data(),b.data(),a.size());assert(t.compared==323&&t.changed25==0&&t.rgbAbsSum==0);
 b[161].weighted[0]=.001f;t=GIAuditTemporal(a.data(),b.data(),a.size());assert(t.changed25==1&&t.changed50==1&&t.examples[0].index==161);
 GIAuditPairCache cache;uint first=0,second=0;
 assert(!cache.Add(601,101,1,19,17,b.data(),t,first,second));
 assert(cache.Add(600,100,1,19,17,a.data(),t,first,second)&&first==100&&second==101&&t.changed25==1);
 assert(!cache.Add(1200,200,1,19,17,a.data(),t,first,second));
 assert(!cache.Add(1201,202,1,19,17,b.data(),t,first,second)); // gap
 assert(!cache.Add(1800,300,1,19,17,a.data(),t,first,second));
 assert(!cache.Add(1801,301,2,19,17,b.data(),t,first,second)); // version
 assert(!cache.Add(2400,400,1,19,17,a.data(),t,first,second));
 assert(!cache.Add(2401,401,1,18,17,b.data(),t,first,second)); // dimensions
 assert(!cache.Add(600,100,1,19,17,a.data(),t,first,second)); // late old pair
 b[161].weighted[0]=std::numeric_limits<float>::infinity();t=GIAuditTemporal(a.data(),b.data(),a.size());assert(t.invalid==1&&t.compared==322&&t.changed25==0);
 std::cout<<"PASS smaller spatial outliers, RGB temporal delta, ten-frame bounded schedule, reversed completion order, gap/version/dimension rejection and invalid samples.\n";
}
