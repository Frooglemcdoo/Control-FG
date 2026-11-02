#include "../src/gi_reuse/index_snapshot_plan.h"
#include <cassert>
#include <cstdio>
using gi_reuse::IndexSnapshotPlan;
int main(){
 IndexSnapshotPlan p;
 assert(IndexSnapshotPlan::Make(6,0,6,p)&&p.elements==3&&p.copyBytes==8);
 assert(IndexSnapshotPlan::Make(266,2,264,p)&&p.prefixBytes==2&&p.copyBytes==264);
 assert(IndexSnapshotPlan::Make(6,4,2,p)&&p.sourceOffset==4&&p.elements==1);
 assert(IndexSnapshotPlan::Make(UINT64_MAX,UINT64_MAX-3,2,p));
 assert(!IndexSnapshotPlan::Make(UINT64_MAX,UINT64_MAX-3,4,p));
 assert(!IndexSnapshotPlan::Make(512,1,6,p));assert(!IndexSnapshotPlan::Make(512,0,3,p));
 assert(IndexSnapshotPlan::Make(IndexSnapshotPlan::MaxBytes,0,IndexSnapshotPlan::MaxBytes,p));
 assert(p.groupsX<=65535&&p.groupsY<=65535&&std::uint64_t(p.groupsX)*p.groupsY*64>=p.words);
 assert(!IndexSnapshotPlan::Make(IndexSnapshotPlan::MaxBytes+2,0,IndexSnapshotPlan::MaxBytes+2,p));
 for(std::uint64_t owner=1;owner<80;++owner)for(std::uint64_t off=0;off<=owner+1;++off)for(std::uint64_t bytes=0;bytes<owner+2;++bytes){
  const bool valid=bytes&&!(off%2)&&!(bytes%2)&&off+bytes<=owner;
  assert(IndexSnapshotPlan::Make(owner,off,bytes,p)==valid);
  if(valid){assert(p.sourceOffset+p.prefixBytes==off);assert(p.elements*2==bytes);assert(p.copyBytes>=bytes&&p.copyBytes-bytes<4);}
 }
 std::puts("PASS uint16 snapshot plan: exact short tails, two-byte offsets, overflow, 256 MiB bound, 2-D dispatch and exhaustive short ranges");
}
