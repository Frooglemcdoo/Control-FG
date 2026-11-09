#include <cassert>
#include <map>
#include <cstring>
#include <vector>
#include "../../src/rr_albedo_prepare.h"
using namespace control_rr_albedo;
static std::map<std::uintptr_t,std::vector<unsigned char>> memory;
static constexpr uintptr_t Main=0x2000,Technique=0x3000,Original=0x4000,Table=0x5000,Material=0x6000,Name=0x7000;
static unsigned matchMask=0,calls=0;static bool changeTable=false,badLookup=false;
template<class T> void put(uintptr_t b,size_t off,T v) {auto& a=memory[b];if(a.size()<off+sizeof(v))a.resize(off+sizeof(v));std::memcpy(a.data()+off,&v,sizeof(v));}
bool read(uintptr_t address,void* out,size_t n) {for(auto& [base,data]:memory)if(address>=base && address-base<=data.size() && n<=data.size()-(address-base)){std::memcpy(out,data.data()+address-base,n);return true;}return false;}
bool lookup(uintptr_t technique,uint32_t key,uintptr_t* out) {
 *out=0;if(technique==Main){*out=Original;return true;}
 if(technique!=Technique)return false;
 for(unsigned i=0;i<4;++i){uint32_t k=0;read(Table+i*0xA8+4,&k,4);if(k==key){*out=Table+i*0xA8+(badLookup?8:0);return true;}}
 return false;
}
bool validate(void* context,uintptr_t original,uintptr_t candidate) {
 assert(context==&matchMask && original==Original);++calls;
 if(changeTable)put(Technique,0x240,uint32_t(5));
 return (matchMask&(1u<<((candidate-Table)/0xA8)))!=0;
}
void setup() {
 memory.clear();calls=0;changeTable=badLookup=false;
 put(Main,4,uint32_t(0x02000000));put(Main,8,uint32_t(0));put(Main,0x240,uint32_t(16));
 put(Technique,4,uint32_t(0x02000000));put(Technique,8,uint32_t(0));put(Technique,0x238,Table);put(Technique,0x240,uint32_t(4));
 put(Material,0x9A0,Main);put(Material,0x9D8,Technique);put(Main,0x100,Name);memory[Name]={'e','y','e',0};put(Original,4,uint32_t(0x02C00000));
 for(unsigned i=0;i<4;++i){auto c=Table+i*0xA8;put(Table,i*0xA8+4,uint32_t(0x02000000+i*0x100));for(unsigned s=0;s<7;++s)put(Table,i*0xA8+0x10+s*0x10,uintptr_t((s==0 || s==4)?0x9000:0));(void)c;}
}
int main() {
 Access a{read,lookup,nullptr,&matchMask,validate};uintptr_t selected=999;EyeVariantEvidence e{};
 for(unsigned mask=0;mask<16;++mask) {
  setup();matchMask=mask;
  const bool unique=mask && !(mask&(mask-1));
  assert(SelectEyeVariant(a,Technique,Original,selected,&e)==unique);
  assert(calls==4 && e.tableValid);
  if(!unique)assert(selected==0);
 }
 setup();matchMask=4;OpaqueBatch b{};b.shader=Original;b.mesh=0x9000;b.material=Material;b.instanceCount=1;
 uint8_t kind=0;assert(selectStandardAlbedo(a,b,selected,&kind) && kind==6 && selected==Table+2*0xA8);
 matchMask=3;RejectionExample rejection{};assert(!selectStandardAlbedo(a,b,selected));
 assert(DiagnoseReject(a,b,rejection)==RejectReason::EyeVariantNotUnique && rejection.eye.matchCount==2);
 setup();matchMask=4;changeTable=true;assert(!SelectEyeVariant(a,Technique,Original,selected,&e) && selected==0 && !e.tableValid);
 setup();badLookup=true;assert(!SelectEyeVariant(a,Technique,Original,selected) && calls==0);
 setup();put(Table,0xA8+4,uint32_t(0x02000000));assert(!SelectEyeVariant(a,Technique,Original,selected) && calls==0);
 setup();put(Technique,0x240,uint32_t(3));assert(!SelectEyeVariant(a,Technique,Original,selected));
 setup();put(Technique,0x238,uintptr_t(UINTPTR_MAX-8));assert(!SelectEyeVariant(a,Technique,Original,selected));
 setup();matchMask=4;put(Table,2*0xA8+0x20,uintptr_t(0x9000));assert(!SelectEyeVariant(a,Technique,Original,selected) && calls==3);
 setup();a.validateEyeCandidate=nullptr;assert(!selectStandardAlbedo(a,b,selected));a.validateEyeCandidate=validate;
 setup();put(Material,0x9D8,uintptr_t(0));rejection={};
 assert(DiagnoseReject(a,b,rejection)==RejectReason::AlbedoTechniqueUnreadable && !std::strcmp(rejection.materialFamily,"eye") && rejection.shaderKey==0x02C00000);
}
