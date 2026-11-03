#pragma once
#include "ordered_as_metadata.h"
#include "instance_sample_validation.h"
#include "pt_shader_table_adapter.h"
#include "true_geometry.h"
#include <array>
#include <cstring>
namespace gi_reuse {
// Offline composition of existing adapters. The host must capture these bytes at
// the stated completed versions; this class does not manufacture that evidence.
// No GPU/process addresses are dereferenced, and no native SBT is modified.
struct SceneByteView {
 std::uint64_t address=0,token=0,epoch=0;
 std::shared_ptr<const std::vector<unsigned char>> bytes;
 const unsigned char* Read(std::uint64_t start,std::uint64_t count) const noexcept {
  if(!token||!epoch||!bytes||!address||bytes->size()>UINT64_MAX-address||start<address)return nullptr;
  const auto offset=start-address;
  return offset<=bytes->size()&&count<=bytes->size()-offset?bytes->data()+offset:nullptr;
 }
};
struct SceneGeometryBytes {
 std::uint64_t blas=0,token=0,revision=0,epoch=0;
 std::uint32_t geometry=0;
 SceneByteView indices,positions,attributes;
 PTSceneAdmission::Geometry binding{};
};
struct SceneHitPacket {
 std::uint64_t cut=0,tlas=0,tlasToken=0,tlasRevision=0,epoch=0,pipeline=0;
 SceneByteView instances,shaderTable;
 PTShaderTableAdapter::Exports identifiers{};
 // The native GI producer uses contribution 0 and geometry multiplier 2.
 // Caller must establish the matching native pipeline/table, not just their sizes.
 std::uint32_t instanceIndex=0,primitive=0;
 SceneGeometryBytes geometry;
};
enum class SceneHitError {None,Identity,Order,Version,Instance,Geometry,Bytes,Binding,Table,Index,Degenerate};
struct SceneHitResult {
 std::array<GIPoint,3> world{};
 GIPoint geometricNormal{};
 std::uint32_t material=0,tableRow=0,instanceID=0,instanceFlags=0,geometryFlags=0;
 bool alphaTestedExport=false;
};
inline bool SameView(const SceneByteView& bytes,PTSceneAdmission::View view,std::uint64_t epoch) noexcept {
 return bytes.epoch==epoch&&view.allocation.epoch==epoch&&view.allocation.address==bytes.address&&
  bytes.bytes&&view.allocation.bytes==bytes.bytes->size()&&bytes.Read(view.address,1);
}
inline GIPoint TransformScenePoint(GIPoint p,const float* m) noexcept {
 return {m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],m[8]*p.x+m[9]*p.y+m[10]*p.z+m[11]};
}
template<std::size_t Capacity> SceneHitError ResolveSceneHit(const OrderedASMetadata<Capacity>& order,
 const SceneHitPacket& p,SceneHitResult& result) {
 result={};SceneHitResult out;
 if(!p.cut||!p.tlas||!p.tlasToken||!p.tlasRevision||!p.epoch||!p.pipeline)return SceneHitError::Identity;
 ASRecord top;if(order.Select(p.cut,p.tlasToken,p.tlas,top)!=PTSceneOrder::Status::ready)return SceneHitError::Order;
 if(top.revision!=p.tlasRevision)return SceneHitError::Version;
 if(!top.topLevel||p.instanceIndex>=top.count||p.instances.epoch!=p.epoch)return SceneHitError::Instance;
 const auto* raw=p.instances.Read(top.instances+std::uint64_t(p.instanceIndex)*64,64);
 InstanceRecord instance;if(!raw||!DecodeInstance(raw,instance)||!(instance.mask&1)||!instance.blas||instance.blas%256)return SceneHitError::Instance;
 for(float v:instance.transform)if(!std::isfinite(v))return SceneHitError::Instance;
 const auto& g=p.geometry;
 if(instance.blas!=g.blas||!g.token||!g.revision||g.epoch!=p.epoch)return SceneHitError::Identity;
 ASRecord bottom;if(order.Select(p.cut,g.token,g.blas,bottom)!=PTSceneOrder::Status::ready)return SceneHitError::Order;
 if(bottom.revision!=g.revision)return SceneHitError::Version;
 const auto* triangle=bottom.geometry?bottom.geometry->At(g.geometry):nullptr;
 if(bottom.topLevel||!triangle||GeometryMetadata::Validate(*triangle)!=GeometryReject::None||triangle->transform||p.primitive>=triangle->indexCount/3)return SceneHitError::Geometry;
 const auto& b=g.binding;
 if(b.epoch!=p.epoch||b.indices.address!=triangle->indices||b.positions.address!=triangle->vertices||
  b.indexStride!=GeometryMetadata::IndexSize(*triangle)||b.indexCount!=triangle->indexCount||
  b.vertexCount!=triangle->vertexCount||b.positionStride!=triangle->stride)return SceneHitError::Binding;
 if(!SameView(g.indices,b.indices,p.epoch)||!SameView(g.positions,b.positions,p.epoch)||!SameView(g.attributes,b.attributes,p.epoch))return SceneHitError::Bytes;
 const auto row=std::uint64_t(instance.hitGroup)+std::uint64_t(g.geometry)*2;
 if(row>UINT32_MAX||p.shaderTable.epoch!=p.epoch||!p.shaderTable.bytes||p.shaderTable.bytes->size()%96||
  row>=p.shaderTable.bytes->size()/96)return SceneHitError::Table;
 const auto* record=p.shaderTable.Read(p.shaderTable.address+row*96,96);
 PTShaderTableAdapter::Bytes converted;
 if(!record||PTShaderTableAdapter::Convert(record,96,p.identifiers,&b,converted)!=PTShaderTableAdapter::Status::ready)return SceneHitError::Table;
 const auto* indices=g.indices.Read(triangle->indices+std::uint64_t(p.primitive)*3*b.indexStride,3*b.indexStride);
 if(!indices)return SceneHitError::Bytes;
 for(unsigned i=0;i<3;++i){std::uint32_t index=0;std::memcpy(&index,indices+i*b.indexStride,b.indexStride);
  if(index>=triangle->vertexCount)return SceneHitError::Index;
  const auto* xyz=g.positions.Read(triangle->vertices+std::uint64_t(index)*triangle->stride,12);
  GIPoint point;if(!xyz)return SceneHitError::Bytes;std::memcpy(&point,xyz,12);
  if(!GIFinitePoint(point))return SceneHitError::Geometry;
  out.world[i]=TransformScenePoint(point,instance.transform);
  if(!GIFinitePoint(out.world[i]))return SceneHitError::Geometry;
 }
 // Reuse the scale-resistant geometric normal calculation, without claiming a
 // ray/triangle intersection. Accepted-hit and fresh alpha visibility are separate.
 const auto normal=GIRecoverTrueHit({0,0,0},{0,0,1},0,out.world[0],out.world[1],out.world[2]);
 if(!normal.valid)return SceneHitError::Degenerate;
 out.geometricNormal=normal.geometricNormal;out.material=b.material;out.tableRow=static_cast<std::uint32_t>(row);
 out.instanceID=instance.id;out.instanceFlags=instance.flags;out.geometryFlags=triangle->flags;
 out.alphaTestedExport=!std::memcmp(record,p.identifiers.nativeAlpha.data(),32);
 result=out;return SceneHitError::None;
}
}
