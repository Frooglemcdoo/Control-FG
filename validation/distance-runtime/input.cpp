// Production snapshot integration with bounded host memory and a native-compare
// trampoline. This exercises the Windows source logic, not Windows SEH itself.
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#undef __try
#define __try try
#define __except(x) catch(...)
using DWORD=unsigned long;
void *verifiedD3d=nullptr,*verifiedRenderer=nullptr;
constexpr std::uintptr_t kViewVtableRva=0x62e1f0;
struct CameraSnapshot {unsigned long long engineFrame;void* view;double worldToView[12],viewToWorld[12];};
static CameraSnapshot camera{};
static bool ReadCamera(CameraSnapshot* out,DWORD*,const char**){*out=camera;return true;}
static bool ReadEngineFrameSafe(unsigned long long* out,DWORD*){*out=camera.engineFrame;return true;}
struct Span{std::uintptr_t start;std::size_t size;};
static std::vector<Span> spans;
static std::uintptr_t tlsArray[2]{};
static std::uintptr_t __readgsqword(unsigned x){assert(x==0x58);return reinterpret_cast<std::uintptr_t>(tlsArray);}
namespace control_rr_reflection {
struct Context {unsigned long long frame;std::uintptr_t view;};
struct Shape {std::uint64_t width;unsigned height,layers;};
struct Snapshot {Context context;struct {Shape shape;} material;};
struct WindowsAccess {static bool Read(void*,std::uintptr_t a,void* out,std::size_t n){
 for(const auto& s:spans)if(a>=s.start&&a-s.start<=s.size&&n<=s.size-(a-s.start)){std::memcpy(out,reinterpret_cast<void*>(a),n);return true;}
 return false;
}};
}
#include "../../src/rr_distance_input.h"
static bool mutateWhenDepth=false;
static bool Compare(int id,const void* data,int* difference){
 control_rr_provider::Entry e{};assert(RRProviderEntry(nullptr,id,e));
 *difference=std::memcmp(reinterpret_cast<void*>(tlsArray[0]+0x4250+e.offset),data,e.size);
 if(id==9&&mutateWhenDepth){*reinterpret_cast<float*>(tlsArray[0]+0x4250+7*128)+=1;mutateWhenDepth=false;}
 return true;
}
template<class T> void put(std::uintptr_t a,const T& x){std::memcpy(reinterpret_cast<void*>(a),&x,sizeof(x));}
int main(){
 const auto d=reinterpret_cast<std::uintptr_t>(mmap(nullptr,0x200000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));assert(d!=std::uintptr_t(MAP_FAILED));
 std::vector<unsigned char> renderer(0x129f000),tls(0x44250),rendererTls(64),primary(4096),bound(4096),depth(256);
 auto address=[](auto& v){return reinterpret_cast<std::uintptr_t>(v.data());};
 verifiedD3d=reinterpret_cast<void*>(d);verifiedRenderer=renderer.data();const auto r=address(renderer),v=address(bound);
 spans.push_back({d,0x200000});for(auto* b:{&renderer,&tls,&rendererTls,&primary,&bound,&depth})spans.push_back({address(*b),b->size()});
 spans.push_back({reinterpret_cast<std::uintptr_t>(tlsArray),sizeof(tlsArray)});
 tlsArray[0]=address(tls);tlsArray[1]=address(rendererTls);put(d+0x1115fc,0u);put(r+0x80283c,1u);put(tlsArray[1]+8,v);put(v,r+kViewVtableRva);
 unsigned char jump[]{0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};auto fn=reinterpret_cast<std::uintptr_t>(&Compare);std::memcpy(jump+2,&fn,8);std::memcpy(reinterpret_cast<void*>(d+0x16a40),jump,sizeof(jump));
 camera.engineFrame=41;camera.view=primary.data();for(int i=0;i<3;++i)camera.worldToView[i*3+i]=camera.viewToWorld[i*3+i]=1;
 std::memcpy(bound.data()+0x20,camera.worldToView,96);std::memcpy(bound.data()+0x80,camera.viewToWorld,96);
 put(address(primary)+0x5b8,7);put(address(primary)+0x518,10);put(r+0x129e5e8,8);put(r+0x1296350,9);
 for(auto pair:{std::pair<int,int>{7,64},{8,8},{9,16},{10,64}})put(d+0x112840+pair.first*16,control_rr_provider::Entry{pair.first*128,0,pair.second,0});
 float matrix[16]{};for(int i=0;i<4;++i)matrix[i*4+i]=1;matrix[12]=0.125f;
 std::memcpy(tls.data()+0x4250+7*128,matrix,64);
 float world[16]{};world[0]=2;world[1]=0.25f;world[4]=-0.5f;world[5]=3;world[10]=4;world[15]=1;std::memcpy(tls.data()+0x4250+10*128,world,64);float inv[]{1.f/2560,1.f/1440};std::memcpy(tls.data()+0x4250+8*128,inv,8);
 put(tlsArray[0]+0x4250+9*128,std::uintptr_t(123));put(tlsArray[0]+0x4250+9*128+8,address(depth));put(address(depth)+0x88,std::uintptr_t(456));
 control_rr_reflection::Snapshot source{{41,address(primary)},{{2560,1440,2}}};RRDistanceInput out{};const char* reason=nullptr;
 assert(RRDistanceReadInput(source,out,&reason)&&out.valid&&out.constants.clipToView[12]==0.125f&&out.depthResource==456);
 // Primary projection and global CPU mirrors are intentionally absent/stale.
 // A different/unavailable renderer TLS view is irrelevant: all math uses bound providers.
 put(tlsArray[1]+8,std::uintptr_t(0));
 assert(RRDistanceReadInput(source,out,&reason)&&out.constants.basis[0][0]==2&&out.constants.basis[1][1]==3&&out.constants.basis[2][2]==4&&out.constants.basis[0][1]==-0.5f&&out.constants.basis[1][0]==0.25f);
 mutateWhenDepth=true;assert(!RRDistanceReadInput(source,out,&reason)&&std::strcmp(reason,"producer_inputs_changed")==0);
 put(tlsArray[0]+0x4250+7*128,1.f);
 put(tlsArray[0]+0x4250+10*128,NAN);assert(!RRDistanceReadInput(source,out,&reason));put(tlsArray[0]+0x4250+10*128,2.f);
 source.context.frame=42;assert(!RRDistanceReadInput(source,out,&reason));source.context.frame=41;
 put(tlsArray[0]+0x4250+7*128,NAN);assert(!RRDistanceReadInput(source,out,&reason));put(tlsArray[0]+0x4250+7*128,1.f);
 put(d+0x112840+7*16+8,128);assert(!RRDistanceReadInput(source,out,&reason));put(d+0x112840+7*16+8,64);
 put(tlsArray[0]+0x4250+8*128,1.f/1920);assert(!RRDistanceReadInput(source,out,&reason));
 assert(munmap(reinterpret_cast<void*>(d),0x200000)==0);
 puts("PASS production input: bound provider bytes with stale mirrors, bound world basis/frame rejection, nonfinite matrix, wrong size and wrong extent");
}
