#pragma once
namespace control_rr {
// Every setup-only guide handle must be replaced and read back before the
// synchronous native evaluator runs. Matrix storage belongs to that invocation.
template<class Api,class Bindings,class Resource>
bool SetGuideParameters(const Api& api,const Bindings& b,Resource* distance,bool reset) {
 if(!b.diffuse||!b.specular||!b.normal)return false;
 // D1 caller supplies same-frame hit distance only for F; E/fallback pass null.
 // Always overwrite the optional input so no stale resource survives a switch.
 Resource* const none=nullptr;
 api.Resource("DLSS.Input.DiffuseAlbedo",b.diffuse);
 api.Resource("GBuffer.DiffuseAlbedo",b.diffuse);
 api.Resource("DLSS.Input.SpecularAlbedo",b.specular);
 api.Resource("GBuffer.SpecularAlbedo",b.specular);
 api.Resource("GBuffer.Normals",b.normal);
 // The reference path packs linear roughness in normal.w and aliases the
 // same RGBA16F guide through GBuffer.Roughness. Keep both bindings in sync.
 api.Resource("GBuffer.Roughness",b.normal);
 api.Integer("DLSS.Roughness.Mode",1);
 // GI27 selects exactly one specular reprojection path per evaluation.
 // Explicit reflected-image MVs own both documented/legacy MV aliases and
 // clear hit distance; OFF returns to the public v2.1.1 hit-distance path.
 Resource* const specMv=b.specularMotion;
 Resource* const specDistance=specMv?none:distance;
 api.Resource("GBuffer.SpecularMvec",specMv);
 api.Resource("MotionVectorsReflection",specMv);
 api.Resource("DLSSD.SpecularHitDistance",specDistance);
 api.Pointer("WorldToViewMatrix",const_cast<float*>(b.worldToView));
 api.Pointer("ViewToClipMatrix",const_cast<float*>(b.viewToClip));
 if(reset)api.Integer("Reset",1);
 return api.ResourceEquals("DLSS.Input.DiffuseAlbedo",b.diffuse)&&
  api.ResourceEquals("GBuffer.DiffuseAlbedo",b.diffuse)&&
  api.ResourceEquals("DLSS.Input.SpecularAlbedo",b.specular)&&
  api.ResourceEquals("GBuffer.SpecularAlbedo",b.specular)&&
  api.ResourceEquals("GBuffer.Normals",b.normal)&&
  api.ResourceEquals("GBuffer.Roughness",b.normal)&&
  api.IntegerEquals("DLSS.Roughness.Mode",1)&&
  api.ResourceEquals("GBuffer.SpecularMvec",specMv)&&
  api.ResourceEquals("MotionVectorsReflection",specMv)&&
  api.ResourceEquals("DLSSD.SpecularHitDistance",specDistance)&&
  api.PointerEquals("WorldToViewMatrix",b.worldToView)&&
  api.PointerEquals("ViewToClipMatrix",b.viewToClip)&&
  (!reset||api.IntegerEquals("Reset",1));
}
inline bool PrepareNativeAAArguments(bool active,bool selected,bool stopped,bool colorReadable,
 void* color,void*& normal,void*& diffuse,void*& specular) noexcept {
 if(!active&&!selected)return true;
 if(!active||!selected||stopped||!colorReadable||!color||normal||diffuse||specular)return false;
 normal=diffuse=specular=color;return true;
}
}
