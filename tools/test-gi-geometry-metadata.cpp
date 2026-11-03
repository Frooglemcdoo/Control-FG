#include "../src/gi_reuse/submission_ledger.h"
#include <cassert>
#include <cstdio>
using namespace gi_reuse;
static TriangleMetadata Triangle(std::uint64_t vertices=4096){return {vertices,8192,0,12,8,6,6,57,1};}
static ASEvent Build(std::uint64_t address,const std::shared_ptr<const GeometryMetadata>& geometry){
 ASRecord r{};r.address=address;r.count=static_cast<std::uint32_t>(geometry->Size());r.geometry=geometry;
 return {ASOperation::Build,r,0};
}
int main(){
 std::vector<TriangleMetadata> source{Triangle(),Triangle(16384)};
 const auto geometry=GeometryMetadata::Create(source);assert(geometry&&geometry->Size()==2);
 source[0].vertices=32768;assert(geometry->At(0)->vertices==4096);assert(!geometry->At(2));
 ASLedger<4> live,snapshot;ASJournal<4> journal;journal.Reset();journal.Append(Build(256,geometry));
 assert(live.Submit(1,1,journal,journal.generation));const auto revision=live.Find(256)->revision;
 TriangleReadPlan plan;assert(live.PlanTriangle(256,revision,1,1,plan));assert(plan.geometry.vertices==16384&&plan.indexAddress==8198);
 assert(!live.PlanTriangle(256,revision,2,0,plan));assert(plan.asRevision==0);
 assert(!live.PlanTriangle(256,revision,0,2,plan));assert(!live.PlanTriangle(256,revision+1,0,0,plan));
 assert(!live.Snapshot(1,0,snapshot));assert(live.Snapshot(1,1,snapshot));
 // Compaction/clone lineage retains source descriptors at copy time, including
 // original GeometryIndex. Subsequent source rebuilds must not alter copies.
 journal.Reset();journal.Append({ASOperation::Copy,{512},256});assert(live.Submit(1,2,journal,journal.generation));
 const auto copyRevision=live.Find(512)->revision;
 auto changed=GeometryMetadata::Create(source);journal.Reset();journal.Append(Build(256,changed));assert(live.Submit(1,3,journal,journal.generation));
 assert(live.Find(256)->geometry->At(0)->vertices==32768);
 assert(live.PlanTriangle(512,copyRevision,0,0,plan)&&plan.geometry.vertices==4096);
 assert(snapshot.PlanTriangle(256,revision,0,0,plan)&&plan.geometry.vertices==4096);
 assert(!live.PlanTriangle(256,revision,0,0,plan));
 // Discard the caller's storage and rebuild: completed metadata remains owned.
 source.clear();changed.reset();assert(snapshot.Find(256)->geometry->At(1)->vertices==16384);
 journal.Reset();journal.Append({ASOperation::Invalidate,{256},0});assert(live.Submit(1,4,journal,journal.generation));
 assert(!live.PlanTriangle(256,revision,0,0,plan));assert(live.PlanTriangle(512,copyRevision,0,0,plan));
 for(int test=0;test<11;++test){auto t=Triangle();switch(test){
 case 0:t.vertexFormat=2;break;case 1:t.indexFormat=42;break;case 2:t.stride=14;break;
 case 3:t.vertexCount=0;break;case 4:t.indexCount=4;break;case 5:t.vertices=UINT64_MAX-3;break;
 case 6:t.indices=UINT64_MAX-1;break;case 7:t.transform=UINT64_MAX-15;break;
 case 8:t.flags=4;break;case 9:t.vertices=4097;break;case 10:t.indices=8193;break;}
 assert(!GeometryMetadata::Create({Triangle(),t})); // Whole geometry order rejected.
 }
 assert(!GeometryMetadata::Create({}));assert(!GeometryMetadata::Create(std::vector<TriangleMetadata>(4097,Triangle())));
 auto padded=Triangle();padded.stride=16;auto paddedGeometry=GeometryMetadata::Create({padded});assert(paddedGeometry&&GeometryMetadata::VertexBytes(padded)==124);
 GeometryReject rejection;std::uint32_t badIndex=0;auto bad=Triangle();bad.indexFormat=42;
 assert(!GeometryMetadata::Create({Triangle(),bad},&rejection,&badIndex)&&rejection==GeometryReject::IndexFormat&&badIndex==1);
 padded.stride=UINT64_MAX-3;assert(!GeometryMetadata::Create({padded},&rejection)&&rejection==GeometryReject::VertexRange);
 auto transformed=Triangle();transformed.transform=12288;assert(GeometryMetadata::Create({transformed}));
 ASLedger<4> invalid;auto mismatch=Build(256,geometry);mismatch.record.count=1;journal.Reset();journal.Append(mismatch);
 assert(!invalid.Submit(1,1,journal,journal.generation));assert(!invalid.Find(256));
 ASLedger<4> tlas;mismatch=Build(256,geometry);mismatch.record.topLevel=true;journal.Reset();journal.Append(mismatch);
 assert(!tlas.Submit(1,1,journal,journal.generation));
 // Native update changes vertex positions while topology/order stays fixed.
 ASLedger<4> dynamic;ASJournal<4> updates;auto first=Build(256,geometry);first.record.allowUpdate=true;
 updates.Append(first);assert(dynamic.Submit(1,1,updates,0));
 auto vertices=Triangle(32768);auto updated=GeometryMetadata::Create({vertices,Triangle(16384)});
 auto event=Build(512,updated);event.operation=ASOperation::Update;event.source=256;
 updates.Reset();updates.Append(event);assert(dynamic.Submit(1,2,updates,updates.generation));
 assert(dynamic.Find(512)->allowUpdate&&dynamic.Find(512)->geometry->At(0)->vertices==32768);
 assert(dynamic.Find(256)->geometry->At(0)->vertices==4096);
 vertices.indexCount=3;event.record.geometry=GeometryMetadata::Create({vertices,Triangle(16384)});event.source=512;
 updates.Reset();updates.Append(event);assert(!dynamic.Submit(1,3,updates,updates.generation));
 ASLedger<4> forbidden;updates.Reset();updates.Append(Build(256,geometry));assert(forbidden.Submit(1,1,updates,updates.generation));
 event.record.geometry=updated;event.source=256;updates.Reset();updates.Append(event);assert(!forbidden.Submit(1,2,updates,updates.generation));
 std::puts("PASS immutable BLAS geometry, ordered indices, copy/update lineage, stale revision rejection, bounded read plans, unsupported layout and overflow rejection");
}
