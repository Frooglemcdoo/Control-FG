#include "../src/gi_reuse/auto_pixel_audit.h"
#include <cassert>
#include <limits>
#include <iostream>
int main(){using namespace gi_reuse;
 assert(GIAuditX(2560)==256&&GIAuditY(1440)==464);
 assert(GIAuditWidth(19)==19&&GIAuditX(19)==0&&GIAuditY(17)==0);
 std::vector<GIAutoPixelAudit> p(19*17);
 for(auto& v:p){v.flags=7;v.weight=1;for(int k=0;k<3;++k)v.raw[k]=v.weighted[k]=.01f;}
 auto& a=p[8*19+9];for(int k=0;k<3;++k){a.raw[k]=2;a.weighted[k]=.5f;}a.weight=.25f;a.weightedPeak=.49f;
 auto s=GIAuditSummarize(p.data(),19,17);assert(s.valid==323&&s.guided==323&&s.spikes==1&&s.downweightedSpikes==1&&s.examples[0].index==161);
 assert(std::abs(s.examples[0].ratio-50)<.001f);
 a.weight=1.6666667f;s=GIAuditSummarize(p.data(),19,17);assert(s.spikes==1&&s.downweightedSpikes==0);
 a.weighted[0]=std::numeric_limits<float>::infinity();s=GIAuditSummarize(p.data(),19,17);assert(s.invalid==1&&s.valid==322&&s.spikes==0);
 a.flags=0;s=GIAuditSummarize(p.data(),19,17);assert(s.invalid==0&&s.valid==322);
 for(auto& v:p){v.flags=7;for(int k=0;k<3;++k)v.weighted[k]=.0001f;}
 auto& white=p[5*19+5];for(int k=0;k<3;++k)white.weighted[k]=1;
 auto& red=p[10*19+12];red.weighted[0]=.002f;red.weighted[1]=red.weighted[2]=.0001f;
 s=GIAuditSummarize(p.data(),19,17);assert(s.examples[0].index==5*19+5);assert(s.redOutliers==1&&s.redExamples[0].index==10*19+12);
 red.flags=0;s=GIAuditSummarize(p.data(),19,17);assert(s.redOutliers==0);
 assert(GIAuditRedBytes(512,512)==50331648);assert(GIAuditProducerBytes(19,17,4)%32==0);
 std::cout<<"PASS full ROI coordinates, post-weight spike detection, surviving downweighted spike, invalid clearing.\n";
}
