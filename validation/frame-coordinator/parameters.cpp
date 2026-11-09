#include "../../src/rr_guide_parameters.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
struct Resource {};
struct Bindings {Resource* normal;Resource* diffuse;Resource* specular;float worldToView[16]{},viewToClip[16]{};};
struct Api {
 mutable std::map<std::string,void*> resources,pointers;
 mutable std::map<std::string,int> integers;
 mutable int reset=0,writes=0,reads=0;
 int loseWrite=0,loseRead=0;
 void Resource(const char* name,::Resource* value) const {if(++writes!=loseWrite)resources[name]=value;}
 void Pointer(const char* name,void* value) const {if(++writes!=loseWrite)pointers[name]=value;}
 void Integer(const char* name,int value) const {if(++writes!=loseWrite){integers[name]=value;if(std::string(name)=="Reset")reset=value;}}
 bool ResourceEquals(const char* name,::Resource* value) const {return ++reads!=loseRead&&resources.at(name)==value;}
 bool PointerEquals(const char* name,const void* value) const {return ++reads!=loseRead&&pointers.at(name)==value;}
 bool IntegerEquals(const char* name,int value) const {if(++reads==loseRead)return false;auto it=integers.find(name);return it!=integers.end()&&it->second==value;}
};
int main(){
 Resource color,normal,diffuse,specular,distance;Bindings b{&normal,&diffuse,&specular};
 for(int mode=0;mode<3;++mode)for(int fail=0;fail<=13;++fail){
  Api api;for(auto* key:{"DLSS.Input.DiffuseAlbedo","GBuffer.DiffuseAlbedo","DLSS.Input.SpecularAlbedo","GBuffer.SpecularAlbedo","GBuffer.Normals","GBuffer.Roughness","GBuffer.SpecularMvec","MotionVectorsReflection","DLSSD.SpecularHitDistance"})api.resources[key]=&color;
  api.integers["DLSS.Roughness.Mode"]=0;
  api.pointers["WorldToViewMatrix"]=api.pointers["ViewToClipMatrix"]=nullptr;
  if(mode==1)api.loseWrite=fail;
  if(mode==2)api.loseRead=fail;
  assert(control_rr::SetGuideParameters(api,b,&distance,true)==(mode==0||fail==0));
 }
 Api noReset;assert(control_rr::SetGuideParameters(noReset,b,(Resource*)nullptr,false));assert(noReset.reset==0);
 assert(noReset.resources["GBuffer.SpecularMvec"]==nullptr);
 assert(noReset.resources["MotionVectorsReflection"]==nullptr);
 assert(noReset.resources["DLSSD.SpecularHitDistance"]==nullptr);
 assert(control_rr::SetGuideParameters(noReset,b,&distance,false));
 assert(noReset.resources["DLSSD.SpecularHitDistance"]==&distance);
 assert(control_rr::SetGuideParameters(noReset,b,(Resource*)nullptr,false));
 assert(noReset.resources["DLSSD.SpecularHitDistance"]==nullptr);
 for(int bit=0;bit<7;++bit){
  void* n=nullptr;void* d=nullptr;void* s=nullptr;void* c=&color;
  bool active=true,selected=true,stopped=false,readable=true;
  switch(bit){case 0:active=false;break;case 1:selected=false;break;case 2:stopped=true;break;case 3:readable=false;break;case 4:c=nullptr;break;case 5:n=&normal;break;case 6:d=&diffuse;break;}
  auto* old=n;assert(!control_rr::PrepareNativeAAArguments(active,selected,stopped,readable,c,n,d,s));assert(n==old);
 }
 void* n=nullptr;void* d=nullptr;void* s=nullptr;
 assert(control_rr::PrepareNativeAAArguments(false,false,false,false,nullptr,n,d,s));assert(!n&&!d&&!s);
 assert(control_rr::PrepareNativeAAArguments(true,true,false,true,&color,n,d,s));assert(n==&color&&d==&color&&s==&color);
 puts("PASS all r21r retained r21q guide aliases/setters/getters reject RR, optional hit distance binds and clears without stale values, packed roughness mode and matrices verified, SR arguments unchanged");
}
