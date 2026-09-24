// Test the exact observation function with mock COM objects, not the Windows ABI.
#include "../../src/rr_part1_diagnostic.h"
#include <cassert>
#include <cstring>
#include <iostream>
using DWORD=unsigned int;
struct D3D12_HEAP_PROPERTIES { unsigned Type=0; };
using D3D12_HEAP_FLAGS=unsigned;
constexpr unsigned D3D12_RESOURCE_DIMENSION_BUFFER=1,DXGI_FORMAT_UNKNOWN=0,D3D12_TEXTURE_LAYOUT_ROW_MAJOR=1;
struct MockDesc {
 unsigned long long Width=1024;
 unsigned Dimension=1,Height=1,DepthOrArraySize=1,MipLevels=1,Format=0,Layout=1,Flags=0;
 struct {unsigned Count=1;} SampleDesc;
};
struct ID3D12Resource {
 MockDesc desc{};unsigned long long gpu=0xabc000;bool fail=false;long heapResult=0;unsigned calls=0;
 MockDesc GetDesc(){++calls;if(fail)throw 1;return desc;}
 unsigned long long GetGPUVirtualAddress(){return gpu;}
 long GetHeapProperties(D3D12_HEAP_PROPERTIES* p,D3D12_HEAP_FLAGS*){p->Type=1;return heapResult;}
};
static ID3D12Resource object;
static unsigned reads=0;static bool missing=false,change=false;
static bool RRAlbedoRead(std::uintptr_t p,void* out,std::size_t n) {
 ++reads;if(missing)return false;
 if(p==0x3000){auto v=reinterpret_cast<std::uintptr_t>(&object);if(change && reads>2)v+=8;std::memcpy(out,&v,n);return true;}
 if(p==0x3020){unsigned v=0x40;std::memcpy(out,&v,n);return true;}
 return false;
}
static DWORD GetExceptionCode(){return 123;}
#define SUCCEEDED(x) ((x)>=0)
#define __try try
#define __except(x) catch(...)
#define EXCEPTION_EXECUTE_HANDLER 1
#include "../../src/rr_part1_resource.h"
int main(){
 control_rr_part1::Sample s{};s.result=control_rr_part1::Result::Candidate;s.repeatedReadMatched=true;
 s.mappingReadable=true;s.mappingMode=2;s.metadata.buffer=0x3000;s.metadata.bytes=1024;s.metadata.gpuAddress=0xabc000;
 auto r=RRPart1ReadResource(s);assert(r.read && r.pointerMatched && r.descriptorMatched && r.heapType==1);
 object.desc.Width=1023;assert(!RRPart1ReadResource(s).descriptorMatched);object.desc.Width=1024;
 object.desc.Dimension=2;assert(!RRPart1ReadResource(s).descriptorMatched);object.desc.Dimension=1;
 object.gpu=0;assert(!RRPart1ReadResource(s).descriptorMatched);object.gpu=0xabc008;assert(!RRPart1ReadResource(s).descriptorMatched);object.gpu=0xabc000;
 reads=0;change=true;assert(!RRPart1ReadResource(s).pointerMatched);change=false;
 missing=true;assert(!RRPart1ReadResource(s).read);missing=false;
 object.fail=true;r=RRPart1ReadResource(s);assert(!r.read && r.fault==123);object.fail=false;
 object.heapResult=-1;r=RRPart1ReadResource(s);assert(r.heapResult==-1 && r.heapType==0);object.heapResult=0;
 const auto calls=object.calls;s.mappingMode=1;assert(!RRPart1ReadResource(s).read && object.calls==calls);
 s.mappingMode=2;s.repeatedReadMatched=false;assert(!RRPart1ReadResource(s).read && object.calls==calls);
 unsigned emitted=0;for(unsigned long long n=1;n<=10020;++n)if(n<=4||(n&(n-1))==0)++emitted;
 assert(emitted==15);
 std::cout<<"PASS: resource descriptor/address/pointer rejection, fault/mode gates, heap failure, log sampling\n";
}
