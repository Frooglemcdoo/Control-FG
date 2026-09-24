#include "../../src/fg_native_source_policy.h"
#include <cassert>
#include <cstring>
#include <map>
#include <vector>
#include <cstdio>
using namespace control_fg_native_source;
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> slots;
    template<class T> void put(std::uintptr_t address,T value){std::vector<unsigned char> bytes(sizeof(value));std::memcpy(bytes.data(),&value,sizeof(value));slots[address]=bytes;}
    bool read(std::uintptr_t address,void* output,std::size_t size){auto p=slots.find(address);if(p==slots.end()||p->second.size()!=size)return false;std::memcpy(output,p->second.data(),size);return true;}
};
int main(){
 Memory m;constexpr std::uintptr_t global=0x1000,device=0x2000,wrapper=0x3000,texture0=0x4000,texture1=0x5000,resource0=0x6000,resource1=0x7000;
 m.put(global,device);m.put(device+ChainOffset,wrapper);m.put(device+TexturesOffset,texture0);m.put(device+TexturesOffset+8,texture1);
 m.put(texture0+ResourceOffset,resource0);m.put(texture1+ResourceOffset,resource1);
 std::uintptr_t shadows[]={resource0,resource1};
 auto choose=[&](unsigned fallback){return Select(global,wrapper,shadows,2,fallback,[&](auto address,void* out,std::size_t size){return m.read(address,out,size);});};
 // Both healthy and reversed DXGI orders must use the current native resource.
 for(unsigned native=0;native<2;++native)for(unsigned destination=0;destination<2;++destination){
  m.put(device+IndexOffset,native);const auto r=choose(destination);assert(r.status==Status::Matched&&r.source==native&&r.nativeIndex==native);
 }
 // Map by resource identity, never assume native index equals shadow array index.
 m.put(device+IndexOffset,0u);shadows[0]=resource1;shadows[1]=resource0;assert(choose(0).source==1);
 shadows[0]=resource0;shadows[1]=resource1;
 m.put(device+IndexOffset,2u);assert(choose(1).status==Status::BadIndex&&choose(1).source==1);
 m.put(device+IndexOffset,0u);m.put(device+ChainOffset,wrapper+1);assert(choose(1).status==Status::WrongChain);
 m.put(device+ChainOffset,wrapper);shadows[0]=resource1;assert(choose(1).status==Status::NoResourceMatch);
 shadows[0]=resource0;shadows[1]=resource0;assert(choose(1).status==Status::AmbiguousResource);
 shadows[1]=resource1;
 for(auto address:{global,device+ChainOffset,device+IndexOffset,device+TexturesOffset,texture0+ResourceOffset}){
  auto saved=m.slots[address];m.slots.erase(address);assert(choose(1).status==Status::ReadFailed&&choose(1).source==1);m.slots[address]=saved;
 }
 auto reader=[&](auto address,void* out,std::size_t size){return m.read(address,out,size);};
 assert(Select(0,wrapper,shadows,2,1,reader).status==Status::UnverifiedModule);
 assert(Select(global,wrapper,shadows,3,1,reader).status==Status::UnsupportedCount);
 // A resize replaces identities: old resource must not be accepted as a new shadow.
 shadows[0]=0x8000;assert(choose(1).status==Status::NoResourceMatch);
 m.put(texture0+ResourceOffset,std::uintptr_t(0x8000));assert(choose(1).status==Status::Matched&&choose(1).source==0);
 std::puts("PASS native source: healthy/reversed, identity mapping, wrong chain, bounds, read failures, module gate, resize, ambiguity");
}
