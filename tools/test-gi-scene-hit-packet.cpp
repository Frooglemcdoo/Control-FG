#include "../src/gi_reuse/scene_hit_packet.h"
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
 unsigned char table[5*96]{};auto* row=table+4*96;row[0]=2;std::memcpy(row+32,&admitted.indices,8);std::memcpy(row+40,&admitted.attributes,8);std::memcpy(row+48,admitted.constants,40);
 p.shaderTable=Bytes(32768,24,table,sizeof(table));SceneHitResult result;
 assert(ResolveSceneHit(history,p,result)==SceneHitError::None);
 assert(result.material==2&&result.tableRow==4&&result.instanceID==1234&&result.alphaTestedExport);
 assert(result.world[1].x==8&&result.world[2].y==23&&result.geometricNormal.z==-1);
 auto saved=p;p.geometry.revision=1;assert(ResolveSceneHit(history,p,result)==SceneHitError::Version&&result.material==0);p=saved;
 p.geometry.token=99;assert(ResolveSceneHit(history,p,result)==SceneHitError::Order);p=saved;
 p.geometry.positions.epoch=2;assert(ResolveSceneHit(history,p,result)==SceneHitError::Bytes);p=saved;
 p.geometry.binding.material=1;assert(ResolveSceneHit(history,p,result)==SceneHitError::Table);p=saved;
 p.identifiers.nativeAlpha[0]=9;assert(ResolveSceneHit(history,p,result)==SceneHitError::Table);p=saved;
 p.primitive=1;assert(ResolveSceneHit(history,p,result)==SceneHitError::Geometry);p=saved;
 p.instanceIndex=1234;assert(ResolveSceneHit(history,p,result)==SceneHitError::Instance);p=saved; // InstanceID is NOT array index
 p.shaderTable=Bytes(32768,24,table,sizeof(table)-1);assert(ResolveSceneHit(history,p,result)==SceneHitError::Table);p=saved;
 unsigned badIndex=99;std::memcpy(indexBytes,&badIndex,width);p.geometry.indices=Bytes(4096,20,indexBytes,width==2?8:12);
 assert(ResolveSceneHit(history,p,result)==SceneHitError::Index);p=saved;
 float collapsed[12]{};p.geometry.positions=Bytes(8192,21,collapsed,44);assert(ResolveSceneHit(history,p,result)==SceneHitError::Degenerate);p=saved;
 // A later native build on another queue makes the copied source dependency ambiguous.
 assert(history.Submit(2,{recording}));assert(ResolveSceneHit(history,p,result)==SceneHitError::Order);
}
int main(){CheckBothWidths(2);CheckBothWidths(4);std::puts("PASS combined scene packet: ordered clone, owner/version, instance index, 16/32-bit geometry, padded vertices, mirrored transform, normal, material/SBT, stale/bounds/degenerate/competing-writer rejection");}
