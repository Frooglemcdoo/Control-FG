#include "../src/gi_reuse/instance_metadata_mapping.h"
#include <cassert>
#include <cstdio>
using namespace gi_reuse;
int main(){
 auto geometry=GeometryMetadata::Create({TriangleMetadata{4096,8192,0,12,3,3,6,57,0}});assert(geometry);
 auto receipt=std::make_shared<ASCutReceipt>();
 ASJournal<8> journal;ASLedger<16> ledger;
 journal.Append({ASOperation::Build,{256,0,0,1,false,geometry},0});
 journal.Append({ASOperation::Copy,{512},256});
 journal.Append({ASOperation::Build,{768,0,16384,1,true},0,receipt});
 // Later writes in the SAME submitted list must not change the earlier cut.
 journal.Append({ASOperation::Build,{512,0,0,1,false,geometry},0});
 assert(ledger.Submit(1,1,journal,0));auto first=receipt->Load();assert(first);
 assert(first->queue==1&&first->fence==1&&first->event==2&&first->generation==0&&first->destination==768);
 assert(first->Find(512)->revision==2&&ledger.Find(512)->revision==4);
 unsigned char bytes[64]{};std::uint64_t address=512;std::memcpy(bytes+56,&address,8);
 InstanceMappingSummary map;assert(MapInstanceMetadata(bytes,64,*first,map)&&map.matched==1);
 address=1024;std::memcpy(bytes+56,&address,8);assert(MapInstanceMetadata(bytes,64,*first,map)&&map.missing==1);
 address=768;std::memcpy(bytes+56,&address,8);assert(MapInstanceMetadata(bytes,64,*first,map)&&map.wrongType==1);
 address=0;std::memcpy(bytes+56,&address,8);assert(MapInstanceMetadata(bytes,64,*first,map)&&map.nullBLAS==1);
 assert(!MapInstanceMetadata(bytes,63,*first,map));
 assert(ledger.Submit(1,2,journal,0));auto replay=receipt->Load();assert(replay&&replay->fence==2&&replay->Find(512)->revision==6);
 assert(first->fence==1&&first->Find(512)->revision==2); // published copies stay immutable
 // Failure after the cut must publish nothing, even if a previous replay succeeded.
 journal.Append({ASOperation::Copy,{1024},99999});
 assert(!ledger.Submit(1,3,journal,0)&&!receipt->Load());
 assert(first->Find(512)->revision==2);
 std::puts("PASS instance metadata mapping: exact event, same-list later writes, copy lineage, replay, immutable cuts, failure clearing and missing/type/null checks");
}
