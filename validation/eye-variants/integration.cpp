#include <cassert>
#include <fstream>
#include <iterator>
#include "../../src/rr_albedo_prepare.h"
#include "../../src/rr_albedo_shader.h"
using namespace control_rr_albedo;
static std::vector<std::vector<uint8_t>> programs;
static uintptr_t tableAddress;
struct Descriptor {uint64_t reserved;uint32_t stage;int32_t id;uint64_t size;};
const char* resident(int id){return reinterpret_cast<const char*>(programs.at(id).data());}
bool readMemory(uintptr_t a,void* out,size_t n){std::memcpy(out,reinterpret_cast<void*>(a),n);return true;}
bool getShader(uintptr_t,uint32_t key,uintptr_t* out){*out=0;for(unsigned i=0;i<4;++i){uint32_t k;readMemory(tableAddress+i*0xA8+4,&k,4);if(k==key){*out=tableAddress+i*0xA8;return true;}}return false;}
bool validate(void* context,uintptr_t a,uintptr_t b){RRAlbedoShader::Result r{};return static_cast<RRAlbedoShader::Session*>(context)->ValidatePair(a,b,r);}
template<class T> void put(unsigned char* p,size_t offset,T value){std::memcpy(p+offset,&value,sizeof(value));}
int main(int argc,char** argv){
 assert(argc==4);
 for(int i=1;i<4;++i){std::ifstream f(argv[i],std::ios::binary);assert(f);programs.emplace_back(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());}
 RRAlbedoShader::residentGetter=resident;
 Descriptor originalVs{0,0,0,programs[0].size()},otherVs{0,0,1,programs[1].size()},pixel{0,4,2,programs[2].size()};
 alignas(8) unsigned char technique[0x248]{},table[4*0xA8]{},original[0xA8]{};
 tableAddress=reinterpret_cast<uintptr_t>(table);
 put(technique,4,uint32_t(0x02000000));put(technique,0x238,tableAddress);put(technique,0x240,uint32_t(4));
 put(original,0x10,reinterpret_cast<uintptr_t>(&originalVs));
 for(unsigned i=0;i<4;++i){put(table,i*0xA8+4,uint32_t(0x02000000+i*0x100));put(table,i*0xA8+0x10,reinterpret_cast<uintptr_t>(i==0?&originalVs:&otherVs));put(table,i*0xA8+0x50,reinterpret_cast<uintptr_t>(&pixel));}
 RRAlbedoShader::Session session;Access a{readMemory,getShader,nullptr,&session,validate};uintptr_t chosen=0;EyeVariantEvidence e{};
 assert(SelectEyeVariant(a,reinterpret_cast<uintptr_t>(technique),reinterpret_cast<uintptr_t>(original),chosen,&e));
 assert(chosen==tableAddress && e.matchCount==1 && session.PairCount()==4);
 // Two matching programs must reject, even though each passes the real parser.
 put(table,0xA8+0x10,reinterpret_cast<uintptr_t>(&originalVs));RRAlbedoShader::Session second;a.candidateContext=&second;
 assert(!SelectEyeVariant(a,reinterpret_cast<uintptr_t>(technique),reinterpret_cast<uintptr_t>(original),chosen,&e) && !chosen && e.matchCount==2);
}
