#include <sys/mman.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#define ABI __attribute__((ms_abi))
struct Value {char type; uint64_t bits;};
static std::map<std::string,Value> values;
static unsigned evaluations=0,creations=0,resultCode=1;
static void* parameterIdentity=reinterpret_cast<void*>(0x11111000);
static void* listIdentity=reinterpret_cast<void*>(0x22222000);
static void* featureIdentity=reinterpret_cast<void*>(0x33333000);
static void ABI SetResource(void* p,const char* k,void* v){assert(p==parameterIdentity);values[k]={'r',reinterpret_cast<uint64_t>(v)};}
static void ABI SetPointer(void* p,const char* k,void* v){assert(p==parameterIdentity);values[k]={'p',reinterpret_cast<uint64_t>(v)};}
static void ABI SetUInt(void* p,const char* k,unsigned v){assert(p==parameterIdentity);values[k]={'u',v};}
static void ABI SetInt(void* p,const char* k,int v){assert(p==parameterIdentity);values[k]={'i',static_cast<uint32_t>(v)};}
static void ABI SetFloat(void* p,const char* k,float v){assert(p==parameterIdentity);uint32_t b;std::memcpy(&b,&v,4);values[k]={'f',b};}
static unsigned ABI Evaluate(void* list,void* feature,void* params,void* callback){
 assert(list==listIdentity&&feature==featureIdentity&&params==parameterIdentity&&callback==nullptr);
 ++evaluations;return resultCode;
}
static unsigned ABI Create(void* list,unsigned kind,void* params,void** out){
 assert(list==listIdentity&&kind==13&&params==parameterIdentity);++creations;*out=featureIdentity;return resultCode;
}
static void Patch(unsigned char* image,unsigned rva,void* target){
 const unsigned char jump[6]={0xff,0x25,0,0,0,0};std::memcpy(image+rva,jump,6);std::memcpy(image+rva+6,&target,8);
}
template<class T> static void Put(std::array<unsigned char,1024>& a,unsigned offset,T v){std::memcpy(a.data()+offset,&v,sizeof(v));}
static void Want(const char* key,char type,uint64_t bits){assert(values.count(key));assert(values[key].type==type&&values[key].bits==bits);}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream f(argv[1],std::ios::binary);std::vector<unsigned char> raw((std::istreambuf_iterator<char>(f)),{});assert(!raw.empty());
 auto* image=static_cast<unsigned char*>(mmap(nullptr,raw.size(),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));assert(image!=MAP_FAILED);std::memcpy(image,raw.data(),raw.size());
 Patch(image,0x51e40,reinterpret_cast<void*>(&SetResource));Patch(image,0x51ea0,reinterpret_cast<void*>(&SetFloat));
 Patch(image,0x51ef0,reinterpret_cast<void*>(&SetInt));Patch(image,0x51f50,reinterpret_cast<void*>(&SetUInt));
 Patch(image,0x52010,reinterpret_cast<void*>(&SetPointer));Patch(image,0x528d0,reinterpret_cast<void*>(&Create));Patch(image,0x52a20,reinterpret_cast<void*>(&Evaluate));
 assert(mprotect(image,raw.size(),PROT_READ|PROT_EXEC)==0);
 using Eval=unsigned(ABI *)(void*,void*,void*,void*);
 using Make=unsigned(ABI *)(void*,unsigned,unsigned,void**,void*,void*);
 auto eval=reinterpret_cast<Eval>(image+0x1d460);auto make=reinterpret_cast<Make>(image+0x1d320);
 std::array<unsigned char,1024> input{};
 // Native structure slots from exact instructions, all resource identities are inert.
 for(unsigned off: {0u,8u,0x10u,0x18u,0x20u,0x28u,0x30u,0x38u,0x150u})Put<uint64_t>(input,off,0x10000000+off);
 Put<uint64_t>(input,0x228,0x40000000);Put<uint64_t>(input,0x230,0x50000000);
 for(unsigned rc: {1u,0xbad00001u}){
  values.clear();resultCode=rc;auto before=evaluations;assert(eval(listIdentity,featureIdentity,parameterIdentity,input.data())==rc);assert(evaluations==before+1);
  Want("DLSS.Input.DiffuseAlbedo",'r',0x10000000);Want("DLSS.Input.SpecularAlbedo",'r',0x10000008);
  Want("GBuffer.Normals",'r',0x10000010);Want("GBuffer.Roughness",'r',0x10000018);
  Want("Color",'r',0x10000020);Want("Output",'r',0x10000028);Want("Depth",'r',0x10000030);Want("MotionVectors",'r',0x10000038);
  Want("DLSSD.SpecularHitDistance",'r',0x10000150);
  Want("WorldToViewMatrix",'p',0x40000000);Want("ViewToClipMatrix",'p',0x50000000);
 }
 for(auto& kv:values)std::cout<<kv.first<<" "<<kv.second.type<<" 0x"<<std::hex<<kv.second.bits<<std::dec<<"\n";
 input={};Put<unsigned>(input,4,1);Put<unsigned>(input,8,1);Put<unsigned>(input,0xc,2560);Put<unsigned>(input,0x10,1440);Put<unsigned>(input,0x14,3840);Put<unsigned>(input,0x18,2160);
 for(unsigned rc: {1u,0xbad00001u}){
  values.clear();resultCode=rc;void* handle=nullptr;auto before=creations;
  assert(make(listIdentity,1,1,&handle,parameterIdentity,input.data())==rc);assert(creations==before+1);
  Want("DLSS.Denoise.Mode",'i',1);Want("DLSS.Roughness.Mode",'u',1);Want("DLSS.Use.HW.Depth",'u',1);
  Want("Width",'u',2560);Want("Height",'u',1440);Want("OutWidth",'u',3840);Want("OutHeight",'u',2160);
 }
 std::cout<<"PASS native RR create/evaluate instructions; exact guide keys; feature 13; success/failure forwarded once\n";
 munmap(image,raw.size());
}
