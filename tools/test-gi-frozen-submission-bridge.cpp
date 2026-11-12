#include "../src/gi_frozen_submission_bridge.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
namespace b=gi_frozen_submission;
static unsigned before=0,after=0,aborted=0,other=0;
static ID3D12CommandQueue* queue=reinterpret_cast<ID3D12CommandQueue*>(16);
static UINT64 Begin(ID3D12CommandQueue* q,UINT count,ID3D12CommandList*const*)noexcept{
 assert(q==queue);++before;return count?99:0;
}
static void End(ID3D12CommandQueue* q,UINT64 token,bool submitted)noexcept{
 assert(q==queue&&(token==99||token==0));++after;if(!submitted)++aborted;
}
static void Other(ID3D12CommandQueue*,UINT64,bool)noexcept{++other;}
int main(){
 const b::Callbacks observer{Begin,End},replacement{Begin,Other},invalid{Begin,nullptr};
 {b::Call call(queue,1,nullptr);call.Submitted();}assert(!before&&!after);
 b::callbacks=&observer;
 {b::Call call(queue,1,nullptr);call.Submitted();call.Finish();call.Finish();}
 assert(before==1&&after==1&&!aborted);
 try{b::Call call(queue,1,nullptr);throw std::runtime_error("native Execute failure");}catch(const std::runtime_error&){}
 assert(before==2&&after==2&&aborted==1);
 {b::Call call(queue,0,nullptr);b::callbacks=&replacement;call.Submitted();}
 assert(before==3&&after==3&&aborted==1&&!other); // same observer even for token zero
 b::callbacks=&invalid;{b::Call call(queue,1,nullptr);call.Submitted();}assert(before==3);
 b::callbacks=nullptr;
 std::puts("PASS frozen submission bridge: matched observer, actual-return success, exception quarantine, token zero and exactly-once notification.");
}
