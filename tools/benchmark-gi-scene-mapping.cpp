// Synthetic CPU-only workload; not an in-game frame-time or GPU benchmark.
// Default: current persistent path. For a historical baseline compile with
// HEADER set to that revision's scene_producer_table.h and omit CACHE.
#ifndef HEADER
#define HEADER "../src/gi_reuse/scene_producer_table.h"
#define CACHE
#endif
#include HEADER
#include <cassert>
#include <cstdio>
#include <chrono>
using namespace gi_reuse;
using History=OrderedASMetadata<>;
static SceneByteView Bytes(std::uint64_t address,std::uint64_t token,const void* data,std::size_t size){auto bytes=std::make_shared<std::vector<unsigned char>>(size);std::memcpy(bytes->data(),data,size);return {address,token,1,bytes};}
static void Run(unsigned width){
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

 auto first=nativeBindings[0];const unsigned unique=1950,instances=11344;
 nativeBindings.assign(unique,first);
 for(unsigned n=0;n<unique;++n)nativeBindings[n].constants[3]=n;
 assert(BuildNativeGITable(44,2,1,nativeBindings,p.identifiers.nativeOpaque,p.identifiers.nativeAlpha,table));
 std::vector<unsigned char> many(instances*64);
 for(unsigned n=0;n<instances;++n){auto* dst=many.data()+n*64;std::memcpy(dst,instance,64);unsigned row=(n%unique)*2;std::memcpy(dst+52,&row,4);}
 GrowingASJournal<8> next;next.Reset();next.Append({ASOperation::Build,{768,0,16384,instances,true},0});
 auto nr=History::Freeze(next,next.generation,{{12,0}});assert(history.Submit(1,{nr}));auto cut=history.Mark(1);
 auto bytes=Bytes(16384,23,many.data(),many.size());std::vector<SceneASReference> refs(instances,{512,11,2});
 auto frozen=std::make_shared<GIFrozenGeometry>();frozen->owner=std::make_shared<int>(1);
 frozen->indices={4096,width==2?8u:12u,65536,width==2?8u:12u};frozen->positions={8192,44,73728,44};witnesses[0].frozen=frozen;
 GISceneProducerPlan plan;
 #ifdef CACHE
 GIValidatedMappingCache cache;
 #endif
 auto go=[&](){assert(PrepareGISceneProducer(history,cut,1,{768,12,4},bytes,refs,witnesses,owners,table,nativeBindings,45,p.identifiers,plan,nullptr
 #ifdef CACHE
 ,&cache
 #endif
 )==GISceneProducerError::None);};
 for(unsigned i=0;i<5;++i)go();
 auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<50;++i)go();
 printf("width=%u average_ms=%.3f\n",width,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/50);
}
int main(){Run(2);Run(4);}
