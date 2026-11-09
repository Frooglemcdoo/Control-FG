#include "../src/rt_ptlas_plan.h"
#include <cassert>
#include <limits>
int main(){
 using namespace rt_ptlas;
 Instance a{};a.transform[0]=a.transform[5]=a.transform[10]=1;a.blas=256;
 a.idMask=0xff000007;a.contributionFlags=0x01000003;
 Planner p(4,2);std::vector<Instance> v{a,a};std::vector<std::uint32_t> parts{0,1};
 auto first=p.Prepare(v,parts);assert(first.initialize&&first.writes.size()==4);
 assert(first.writes[0].id==7&&first.writes[0].mask==255&&first.writes[0].contribution==3&&first.writes[0].flags==1);
 assert(first.writes[1].index==1&&first.writes[2].blas==0);
 p.Commit(std::move(first));assert(p.Prepare(v,parts).writes.empty());
 // Duplicate user IDs are legal: moving slot 1 must never alter slot 0.
 v[1].transform[3]=10;auto moving=p.Prepare(v,parts);
 assert(moving.writes.size()==1&&moving.writes[0].index==1);p.Commit(std::move(moving));
 v[0].idMask=7;auto mask=p.Prepare(v,parts);assert(mask.writes.size()==1&&mask.writes[0].mask==0);p.Commit(std::move(mask));
 v.resize(1);parts.resize(1);auto removed=p.Prepare(v,parts);
 assert(removed.writes.size()==1&&removed.writes[0].index==1&&removed.writes[0].blas==0);p.Commit(std::move(removed));
 // New object reuses an array slot, retaining native index semantics.
 v.push_back(a);parts.push_back(0xffffffffu);auto added=p.Prepare(v,parts);
 assert(added.writes.size()==1&&added.writes[0].partition==0xffffffffu);p.Commit(std::move(added));
 v[1].blas=512;auto lod=p.Prepare(v,parts);assert(lod.writes.size()==1&&lod.writes[0].blas==512);
 // Discarded recording must regenerate its writes.
 assert(p.Prepare(v,parts).writes.size()==1);p.Commit(std::move(lod));
 auto stale=p.Prepare(v,parts);p.Commit(p.Prepare(v,parts));
 bool rejected=false;try{p.Commit(std::move(stale));}catch(const std::logic_error&){rejected=true;}assert(rejected);
 parts[0]=2;rejected=false;try{p.Prepare(v,parts);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);parts[0]=0;
 v[0].transform[0]=std::numeric_limits<float>::quiet_NaN();rejected=false;
 try{p.Prepare(v,parts);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);v[0]=a;
 v[0].blas=257;rejected=false;try{p.Prepare(v,parts);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);v[0]=a;
 auto empty=p.Prepare({},{});assert(empty.writes.size()==2);p.Commit(std::move(empty));assert(p.Prepare({},{}).writes.empty());
}
