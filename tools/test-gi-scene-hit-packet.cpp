#include "../src/gi_reuse/native_gi_job_route.h"
#include "../src/gi_reuse/native_gi_table.h"
#include "../src/gi_reuse/native_gi_route.h"
#include "../src/gi_reuse/scene_traversal_closure.h"
#include "../src/gi_reuse/scene_producer_table.h"
#include <cassert>
#include <cstdio>
using namespace gi_reuse;
using History=OrderedASMetadata<>;
static SceneByteView Bytes(std::uint64_t address,std::uint64_t token,const void* data,std::size_t size){
 auto bytes=std::make_shared<std::vector<unsigned char>>(size);std::memcpy(bytes->data(),data,size);return {address,token,1,bytes};
}
static void CheckBothWidths(unsigned width){
 History history;TriangleMetadata triangle{8192,4096,0,16,3,3,6,width==2?57u:42u,0};
 auto metadata=GeometryMetadata::Create({triangle});assert(metadata);
 GrowingASJournal<8> journal;journal.Reset();journal.Append({ASOperation::Build,{256,0,0,1,false,metadata},0});
 journal.Append({ASOperation::Copy,{512},256});journal.Append({ASOperation::Build,{768,0,16384,1,true},0});
 auto recording=History::Freeze(journal,journal.generation,{{10,0},{11,10},{12,0}});assert(recording&&history.Submit(1,{recording}));
 SceneHitPacket p;p.cut=history.Mark(1);p.tlas=768;p.tlasToken=12;p.tlasRevision=3;p.epoch=1;p.pipeline=44;
 auto& g=p.geometry;g.blas=512;g.token=11;g.revision=2;g.epoch=1;
 unsigned char indexBytes[12]{};for(unsigned i=0;i<3;++i)std::memcpy(indexBytes+i*width,&i,width);
 const float vertices[12]={0,0,0,99, 1,0,0,99, 0,1,0,99};unsigned char attributes[48]{};
 g.indices=Bytes(4096,20,indexBytes,width==2?8:12);g.positions=Bytes(8192,21,vertices,44);g.attributes=Bytes(12288,22,attributes,48);
 auto& b=g.binding;b.indices={{4096,width==2?8u:12u,1},4096};b.positions={{8192,44,1},8192};b.attributes={{12288,48,1},12288};
 b.indexStride=width;b.indexCount=3;b.vertexCount=3;b.positionStride=16;b.attributeStride=16;b.normalOffset=4;b.uvOffset=12;
 b.material=2;b.materialCount=3;b.epoch=1;b.float3Positions=b.sharedVertexIndexing=b.noGeometryTransform=true;
 unsigned char instance[64]{};float transform[12]={-2,0,0,10,0,3,0,20,0,0,1,30};std::memcpy(instance,transform,48);
 std::uint32_t idMask=0x01000000|1234,hitFlags=4;std::uint64_t blas=512;
 std::memcpy(instance+48,&idMask,4);std::memcpy(instance+52,&hitFlags,4);std::memcpy(instance+56,&blas,8);
 p.instances=Bytes(16384,23,instance,64);
 p.identifiers.nativeOpaque[0]=1;p.identifiers.nativeAlpha[0]=2;p.identifiers.privateOpaque[0]=3;p.identifiers.privateAlpha[0]=4;
 PTSceneAdmission::Record admitted;assert(PTSceneAdmission::Admit(b,admitted)==PTSceneAdmission::Error::none);
 std::vector<PTNativeBinding::Binding> nativeBindings(3);
 for(auto& binding:nativeBindings){binding.type=1;binding.indices=admitted.indices;binding.attributes=admitted.attributes;std::memcpy(binding.constants,admitted.constants,40);}
 NativeGITable table;
 assert(BuildNativeGITable(44,1,1,nativeBindings,p.identifiers.nativeOpaque,p.identifiers.nativeAlpha,table));
 assert(table.Active(4)&&!table.Active(5)&&table.bytes.size()==6*96);
 // Resolve the complete active table from current BLAS witnesses, not a single
 // index-address candidate. Unreferenced native rows retain their slots.
 PTOwnedScene::Index owners;
 owners.Insert({20,4096,width==2?8u:12u});owners.Insert({21,8192,44});owners.Insert({22,12288,48});
 PTOwnedScene::Triangle witness{4096,8192,16,20,21,width,3,3};assert(owners.Observe(witness));
 std::vector<GISceneGeometry> witnesses={{{512,11,2},0,witness}};
 GISceneProducerPlan producer;
 auto prepare=[&](){return PrepareGISceneProducer(history,p.cut,1,{768,12,3},p.instances,
     {{512,11,2}},witnesses,owners,table,nativeBindings,45,p.identifiers,producer);};
 assert(prepare()==GISceneProducerError::None);
 assert((producer.table.bound==std::vector<unsigned char>{0,0,1}));
 assert((producer.allocationTokens==std::vector<std::uint64_t>{20,21,22}));
 assert(producer.table.records[0]==PTShaderTableAdapter::Bytes{}&&producer.table.records[4][0]==4);
 const auto preserved=producer.table.records;
 witnesses[0].blas.revision=1;assert(prepare()==GISceneProducerError::Geometry);witnesses[0].blas.revision=2;
 witnesses[0].triangle.positionToken=99;assert(prepare()==GISceneProducerError::Geometry);witnesses[0].triangle=witness;
 witnesses[0].triangle.positions+=4;assert(prepare()==GISceneProducerError::Geometry);witnesses[0].triangle=witness;
 witnesses.push_back(witnesses[0]);assert(prepare()==GISceneProducerError::Geometry);witnesses.pop_back();
 owners.Remove(21);assert(prepare()==GISceneProducerError::Geometry);owners.Insert({21,8192,44});
 assert(producer.table.records==preserved&&prepare()==GISceneProducerError::None);
 {
  // Two BLAS geometries may share indices yet use different position buffers.
  // Preserve the per-instance material row; reject an actual SBT-row collision.
  History multiple;assert(multiple.Submit(1,{recording}));
  auto secondTriangle=triangle;secondTriangle.vertices=8448;
  GrowingASJournal<8> changes;changes.Reset();
  changes.Append({ASOperation::Build,{1536,0,0,1,false,GeometryMetadata::Create({secondTriangle})},0});
  changes.Append({ASOperation::Build,{1792,0,16384,2,true},0});
  auto changed=History::Freeze(changes,changes.generation,{{30,0},{31,0}});
  assert(changed&&multiple.Submit(1,{changed}));const auto cut=multiple.Mark(1);
  owners.Insert({24,8448,44});auto secondWitness=witness;secondWitness.positions=8448;secondWitness.positionToken=24;
  assert(owners.Observe(secondWitness));
  auto distinct=nativeBindings;distinct[0].type=0;distinct[0].constants[3]=1;
  NativeGITable distinctTable;
  assert(BuildNativeGITable(44,2,1,distinct,p.identifiers.nativeOpaque,p.identifiers.nativeAlpha,distinctTable));
  unsigned char two[128];std::memcpy(two,instance,64);std::memcpy(two+64,instance,64);
  std::uint32_t firstRow=0;std::uint64_t secondBLAS=1536;
  std::memcpy(two+52,&firstRow,4);std::memcpy(two+64+56,&secondBLAS,8);
  auto both=Bytes(16384,23,two,sizeof(two));
  const auto prepareBoth=[&](){return PrepareGISceneProducer(multiple,cut,1,{1792,31,5},both,
      {{512,11,2},{1536,30,4}},{{{512,11,2},0,witness},{{1536,30,4},0,secondWitness}},
      owners,distinctTable,distinct,45,p.identifiers,producer);};
  assert(prepareBoth()==GISceneProducerError::None);
  assert((producer.table.bound==std::vector<unsigned char>{1,0,1}));
  assert(producer.table.geometry[0].material==1&&producer.table.geometry[2].material==2);
  assert(producer.table.geometry[0].positions.address==8192&&producer.table.geometry[2].positions.address==8448);
  std::memcpy(two+64+52,&firstRow,4);both=Bytes(16384,23,two,sizeof(two));
  assert(prepareBoth()==GISceneProducerError::Conflict);
  assert(producer.table.geometry[2].positions.address==8448); // failed transaction preserves last valid plan
  owners.Remove(24);
 }
 p.shaderTable=Bytes(32768,24,table.bytes.data(),table.bytes.size());SceneHitResult result;
 assert(ResolveSceneHit(history,p,result)==SceneHitError::None);
 assert(result.material==2&&result.tableRow==4&&result.instanceID==1234&&result.alphaTestedExport);
 assert(result.world[1].x==8&&result.world[2].y==23&&result.geometricNormal.z==-1);
 // R305 contains shared geometry with distinct material bindings in one table.
 // Select by instance contribution, never by index/attribute addresses alone.
 {const auto original=p;auto shared=nativeBindings;shared[0].type=0;shared[0].constants[3]=1;
  NativeGITable alternate;assert(BuildNativeGITable(44,1,1,shared,p.identifiers.nativeOpaque,p.identifiers.nativeAlpha,alternate));
  unsigned char firstInstance[64];std::memcpy(firstInstance,instance,64);std::uint32_t contribution=0;std::memcpy(firstInstance+52,&contribution,4);
  p.instances=Bytes(16384,23,firstInstance,64);p.shaderTable=Bytes(32768,24,alternate.bytes.data(),alternate.bytes.size());
  assert(ResolveSceneHit(history,p,result)==SceneHitError::Table); // stale material from row 4
  p.geometry.binding.material=1;
  assert(ResolveSceneHit(history,p,result)==SceneHitError::None&&result.material==1&&result.tableRow==0&&!result.alphaTestedExport);
  p=original;
 }
 auto saved=p;p.geometry.revision=1;assert(ResolveSceneHit(history,p,result)==SceneHitError::Version&&result.material==0);p=saved;
 p.geometry.token=99;assert(ResolveSceneHit(history,p,result)==SceneHitError::Order);p=saved;
 p.geometry.positions.epoch=2;assert(ResolveSceneHit(history,p,result)==SceneHitError::Bytes);p=saved;
 p.geometry.binding.material=1;assert(ResolveSceneHit(history,p,result)==SceneHitError::Table);p=saved;
 p.identifiers.nativeAlpha[0]=9;assert(ResolveSceneHit(history,p,result)==SceneHitError::Table);p=saved;
 p.primitive=1;assert(ResolveSceneHit(history,p,result)==SceneHitError::Geometry);p=saved;
 p.instanceIndex=1234;assert(ResolveSceneHit(history,p,result)==SceneHitError::Instance);p=saved; // InstanceID is NOT array index
 p.shaderTable=Bytes(32768,24,table.bytes.data(),table.bytes.size()-1);assert(ResolveSceneHit(history,p,result)==SceneHitError::Table);p=saved;
 unsigned badIndex=99;std::memcpy(indexBytes,&badIndex,width);p.geometry.indices=Bytes(4096,20,indexBytes,width==2?8:12);
 assert(ResolveSceneHit(history,p,result)==SceneHitError::Index);p=saved;
 float collapsed[12]{};p.geometry.positions=Bytes(8192,21,collapsed,44);assert(ResolveSceneHit(history,p,result)==SceneHitError::Degenerate);p=saved;
 // Full-array traversal closure cannot be inferred from one successful hit.
 SceneTraversalClosure closure;SceneASReference top{768,12,3};
 assert(ResolveTraversalClosure(history,p.cut,1,top,p.instances,{{512,11,2}},closure)==TraversalError::None);
 assert(closure.bottoms.size()==1&&closure.bottoms[0].token==11);
 assert(ResolveTraversalClosure(history,p.cut,1,top,p.instances,{},closure)==TraversalError::Instances&&closure.bottoms.empty());
 unsigned char allInstances[3*64]{};
 std::memcpy(allInstances,instance,64);std::memcpy(allInstances+64,instance,64);
 // Second instance is masked out, but still belongs to the lifetime closure.
 std::uint32_t masked=999;std::memcpy(allInstances+64+48,&masked,4);
 GrowingASJournal<8> full;full.Reset();full.Append({ASOperation::Build,{1024,0,16384,3,true},0});
 auto fullRecording=History::Freeze(full,full.generation,{{13,0}});assert(history.Submit(1,{fullRecording}));
 const auto fullCut=history.Mark(1);top={1024,13,4};
 auto whole=Bytes(16384,23,allInstances,sizeof(allInstances));
 std::vector<SceneASReference> refs={{512,11,2},{512,11,2},{}};
 assert(ResolveTraversalClosure(history,fullCut,1,top,whole,refs,closure)==TraversalError::None&&closure.bottoms.size()==1);
 auto partial=Bytes(16384,23,allInstances,64);
 assert(ResolveTraversalClosure(history,fullCut,1,top,partial,refs,closure)==TraversalError::Instances&&closure.cut==0);
 refs[1].token=88;assert(ResolveTraversalClosure(history,fullCut,1,top,whole,refs,closure)==TraversalError::Identity);
 refs[1]={};assert(ResolveTraversalClosure(history,fullCut,1,top,whole,refs,closure)==TraversalError::Identity);
 refs[1]={512,11,2};refs[2]={512,11,2};
 assert(ResolveTraversalClosure(history,fullCut,1,top,whole,refs,closure)==TraversalError::Identity);
 refs[2]={};whole.epoch=2;
 assert(ResolveTraversalClosure(history,fullCut,1,top,whole,refs,closure)==TraversalError::Instances);
 // A later native build on another queue makes the copied source dependency ambiguous.
 assert(history.Submit(2,{recording}));assert(ResolveSceneHit(history,p,result)==SceneHitError::Order);
}
static void CheckRouteAndTable(){
 NativeGIRoute route;
 const auto bind=[&](){route.RayGeneration("rayGenMain");route.Miss(0,"missMain");
  route.Hit(0,0,true,true,nullptr,"closestHitMain");
  route.Hit(0,1,true,false,"anyHitAlphaTest","closestHitMain");};
 route.Begin(1,2);bind();assert(route.Ready());
 route.Begin(1,2);assert(!route.Ready()); // no stale exports after next begin
 route.RayGeneration("rayGenMain");assert(!route.Ready());
 route.Begin(2,2);bind();assert(!route.Ready()); // reflection layout
 route.Begin(1,2);bind();route.Hit(0,1,true,true,nullptr,"closestHitMain");assert(!route.Ready());
 route.Begin(1,2);bind();route.Miss(1,"missMain");assert(!route.Ready());
 route.Begin(1,2);bind();route.RayGeneration("reflectionRayGeneration");assert(!route.Ready());
 NativeGITable table;PTShaderTableAdapter::Identifier a{},b{};a[0]=1;b[0]=2;
 std::vector<PTNativeBinding::Binding> bindings(2);bindings[1].type=1;
 assert(BuildNativeGITable(1,1,1,bindings,a,b,table));
 assert(table.bytes[0]==1&&table.bytes[192]==2&&table.Active(2)&&!table.Active(1));
 bindings[1].type=2;
 assert(!BuildNativeGITable(1,1,1,bindings,a,b,table)&&table.bytes.empty()&&!table.pipeline);
 bindings[1].type=1;
 assert(!BuildNativeGITable(1,1,2,bindings,a,b,table));
 assert(!BuildNativeGITable(1,1,1,bindings,a,a,table));
 assert(!BuildNativeGITable(0,1,1,bindings,a,b,table));
}
static void CheckJobHandoff(){
 using namespace GINativeJobRoute;
 TableLayout layout{4096,1024,32,64,32,64,96,6};U64 address=0,bytes=0;
 assert(HitRange(layout,3,address,bytes)&&address==4224&&bytes==576);
 auto bad=layout;bad.hitRecords=3;assert(!HitRange(bad,3,address,bytes)&&!address&&!bytes);
 bad=layout;bad.bytes=703;assert(!HitRange(bad,3,address,bytes));
 bad=layout;bad.buffer=UINT64_MAX-63;assert(!HitRange(bad,3,address,bytes));
 Router route;Ticket t;t.serial=1;t.list=2;t.pipeline=3;t.counter=4;t.engineFrame=5;
 t.writer={6,7,8,1024,1,3};t.layout=layout;Ticket seen;
 assert(route.Schedule(0,t));assert(!route.Schedule(0,t));
 auto wrong=t.writer;wrong.rays=2;assert(!route.Begin(0,wrong,seen)&&!seen.serial);
 assert(!route.Finish(0,1));assert(route.Begin(0,t.writer,seen)&&seen.serial==1);
 assert(!route.Begin(0,t.writer,seen)&&!seen.serial);assert(!route.Finish(0,2));
 assert(route.Finish(0,1));t.serial=2;assert(route.Schedule(0,t));
 assert(!route.Finish(0,1));assert(route.Begin(0,t.writer,seen)&&seen.serial==2);
 assert(route.Finish(0,2));t.writer.rays=2;assert(!route.Schedule(0,t));
 t.writer.rays=1;t.writer.count=65537;assert(!route.Schedule(0,t));
 t.writer.count=3;assert(!route.Schedule(16,t));
}
int main(){CheckJobHandoff();CheckRouteAndTable();CheckBothWidths(2);CheckBothWidths(4);std::puts("PASS complete traversal closure (masked/duplicate/null/partial/identity/epoch) and combined scene packet: ordered clone, owner/version, instance index, 16/32-bit geometry, padded vertices, mirrored transform, normal, material/SBT, stale/bounds/degenerate/competing-writer rejection");}
