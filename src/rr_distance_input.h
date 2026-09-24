#pragma once
#include "rr_provider_auth.h"
struct RRDistanceConstants {
 float basis[3][4]{};
 unsigned width=0,height=0,layers=0,reserved=0;
 float clipToView[16]{};
 float invResolution[2]{},padding[2]{};
};
static_assert(sizeof(RRDistanceConstants)==144,"distance cbuffer ABI");
struct RRDistanceInput {
 control_rr_reflection::Context context{};
 RRDistanceConstants constants{};
 std::uintptr_t depthNative=0,depthResource=0;
 bool valid=false;
};
static bool RRProviderEntry(void*,std::int32_t id,control_rr_provider::Entry& entry) noexcept {
 // Conservative admission cap; these IDs come only from exact native wrappers.
 // Size equality is checked before cmpProviderData can read a candidate buffer.
 if(id<0||id>=4096||!verifiedD3d)return false;
 return control_rr_reflection::WindowsAccess::Read(nullptr,reinterpret_cast<std::uintptr_t>(verifiedD3d)+0x112840+std::uintptr_t(id)*16,&entry,sizeof(entry));
}
static bool RRProviderCompare(void*,std::int32_t id,const void* bytes,int& difference) noexcept {
 using Compare=bool (*)(int,const void*,int*);
 __try {
  auto fn=reinterpret_cast<Compare>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x16a40);
  return fn(id,bytes,&difference);
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRProviderReadBound(void*,std::int32_t offset,void* out,std::size_t size) noexcept {
 __try {
  const auto base=reinterpret_cast<std::uintptr_t>(verifiedD3d);
  unsigned index=0;std::uintptr_t tls=0;
  const auto read=&control_rr_reflection::WindowsAccess::Read;
  if(!base||offset<0||size>0x40000||std::size_t(offset)>0x40000-size||
   !read(nullptr,base+0x1115fc,&index,sizeof(index))||index>=4096||
   !read(nullptr,__readgsqword(0x58)+std::uintptr_t(index)*8,&tls,sizeof(tls))||!tls)return false;
  return read(nullptr,tls+0x4250+std::uintptr_t(offset),out,size);
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRDistanceReadInput(const control_rr_reflection::Snapshot& source,RRDistanceInput& out,const char** reason) noexcept {
 out={};*reason="camera_identity";
 __try {
  CameraSnapshot camera{};DWORD fault=0;const char* cameraReason=nullptr;
  if(!ReadCamera(&camera,&fault,&cameraReason)||camera.engineFrame!=source.context.frame||reinterpret_cast<std::uintptr_t>(camera.view)!=source.context.view)return false;
  const auto renderer=reinterpret_cast<std::uintptr_t>(verifiedRenderer);
  const auto read=&control_rr_reflection::WindowsAccess::Read;
  // IDs identify named providers, independent of the CPU view's matrix values.
  // Reconstruct entirely in the reflection producer's bound coordinate system.
  const auto providerView=source.context.view;
  int matrixId=-1,worldId=-1,inverseId=-1,depthId=-1;
  struct Matrix {float values[16];};struct Inverse {float values[2];};
  struct ShaderTexture {std::uintptr_t descriptor,native;};
  Matrix authenticatedMatrix{},authenticatedWorld{};Inverse authenticatedInverse{};ShaderTexture authenticatedDepth{};
  *reason="provider_id_read";
  if(!read(nullptr,providerView+0x5b8,&matrixId,4)||
   !read(nullptr,providerView+0x518,&worldId,4)||
   !read(nullptr,renderer+0x129e5e8,&inverseId,4)||
   !read(nullptr,renderer+0x1296350,&depthId,4))return false;
  const control_rr_provider::Access access{nullptr,&RRProviderEntry,&RRProviderCompare};
  *reason="clip_to_view_provider";
  if(!control_rr_provider::Snapshot(access,matrixId,&RRProviderReadBound,authenticatedMatrix))return false;
  *reason="view_to_world_provider";
  if(!control_rr_provider::Snapshot(access,worldId,&RRProviderReadBound,authenticatedWorld))return false;
  *reason="inverse_resolution_provider";
  if(!control_rr_provider::Snapshot(access,inverseId,&RRProviderReadBound,authenticatedInverse))return false;
  *reason="clip_depth_provider";
  if(!control_rr_provider::Snapshot(access,depthId,&RRProviderReadBound,authenticatedDepth)||!authenticatedDepth.native||!authenticatedDepth.descriptor)return false;
  *reason="depth_resource";
  RRDistanceInput input{};input.context=source.context;input.depthNative=authenticatedDepth.native;
  if(!read(nullptr,authenticatedDepth.native+0x88,&input.depthResource,sizeof(input.depthResource))||!input.depthResource)return false;
  auto& constants=input.constants;constants.width=static_cast<unsigned>(source.material.shape.width);
  constants.height=source.material.shape.height;constants.layers=source.material.shape.layers;
  memcpy(constants.clipToView,authenticatedMatrix.values,64);memcpy(constants.invResolution,authenticatedInverse.values,8);
  *reason="nonfinite_clip_matrix";
  for(float value:constants.clipToView)if(!std::isfinite(value))return false;
  *reason="native_inverse_resolution_extent";
  if(!(constants.invResolution[0]>0)||!(constants.invResolution[1]>0)||
   std::fabs(constants.invResolution[0]*constants.width-1.0f)>0.0001f||
   std::fabs(constants.invResolution[1]*constants.height-1.0f)>0.0001f)return false;
  // Bound shader matrices use float4x4 storage and left multiplication.
  // Convert their linear part to explicit output-component rows for dot products.
  *reason="nonfinite_world_basis";
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
   const float value=authenticatedWorld.values[j*4+i];if(!std::isfinite(value))return false;
   constants.basis[i][j]=static_cast<float>(value);
  }
  *reason="frame_changed";
  unsigned long long frame=0;if(!ReadEngineFrameSafe(&frame,&fault)||frame!=source.context.frame)return false;
  // Confirm the entire provider set is still current after resource validation.
  Matrix recheckMatrix{},recheckWorld{};Inverse recheckInverse{};ShaderTexture recheckDepth{};
  *reason="producer_inputs_changed";
  if(!control_rr_provider::Authenticate(access,matrixId,authenticatedMatrix,recheckMatrix)||
   !control_rr_provider::Authenticate(access,worldId,authenticatedWorld,recheckWorld)||
   !control_rr_provider::Authenticate(access,inverseId,authenticatedInverse,recheckInverse)||
   !control_rr_provider::Authenticate(access,depthId,authenticatedDepth,recheckDepth))return false;
  input.valid=true;out=input;*reason="ready";return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){out={};*reason="snapshot_exception";return false;}
}
