#pragma once
#include <cstring>
#include <cstdint>
namespace gi_reuse {
// CPU setup identity only. Does not prove the worker's state object or GPU table.
// Reset on EVERY begin: shader names from a preceding setup must never carry over.
class NativeGIRoute {
 bool setup_=false,raygen_=false,miss_=false,opaque_=false,alpha_=false,invalid_=false;
 static bool Is(const char* a,const char* b) noexcept {return a&&std::strcmp(a,b)==0;}
public:
 void Begin(int rays,int types) noexcept {*this={};setup_=rays==1&&types==2;}
 void RayGeneration(const char* name) noexcept {raygen_=Is(name,"rayGenMain");}
 void Miss(int slot,const char* name) noexcept {
  if(slot!=0||!Is(name,"missMain"))invalid_=true;else miss_=true;
 }
 void Hit(int ray,int type,bool noIntersection,bool noAnyHit,const char* anyHit,const char* closest) noexcept {
  const bool common=ray==0&&noIntersection&&Is(closest,"closestHitMain");
  if(common&&type==0&&noAnyHit)opaque_=true;
  else if(common&&type==1&&Is(anyHit,"anyHitAlphaTest"))alpha_=true;
  else invalid_=true;
 }
 bool Ready() const noexcept {return setup_&&raygen_&&miss_&&opaque_&&alpha_&&!invalid_;}
};
}
