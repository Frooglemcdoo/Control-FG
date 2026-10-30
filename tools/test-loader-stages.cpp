#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
static void Log(const char*,...){}
static bool HashMatches(HMODULE,const char*){return false;}
static void* AllocateExecutableRelayNear(const void*){return nullptr;}
static bool Rel32Fits(const void*,const void*){return false;}
#include "../src/native_cpu_call_patch.h"
#include "../src/native_loader_trace_runtime.h"
using namespace native_loader_stages;
static unsigned char record[0x188]{},pack[0x380]{},entry[0x30]{},handle[16]{},output[512]{};
static bool insideSelection=false;
static unsigned offered=0;
static void* owner=reinterpret_cast<void*>(0x1234);
static void Put(void* p,unsigned offset,U64 v){memcpy(static_cast<unsigned char*>(p)+offset,&v,8);}
static void LookupMock(void* self,void* records,U64 count,int stride){
 assert(self==pack&&records==record&&count==1&&stride==0x188&&GetLastError()==101);
 Put(record,0x120,reinterpret_cast<U64>(pack));Put(record,0x128,100100);Put(record,0x130,4096);SetLastError(102);
}
static bool SelectMock(void* self,void* out){insideSelection=true;assert(self==owner&&out==record&&selectionOwner==reinterpret_cast<U64>(owner));LookupHook(pack,record,1,0x188);assert(GetLastError()==102);insideSelection=false;return true;}
static U64 PackMock(void* p,void* h,void* out,U64 offset,U64 count){
 assert(p==pack&&h==handle&&out==output&&offset==32&&count==512&&GetLastError()==201);SetLastError(202);return 511;
}
static BOOL WINAPI ReadMock(HANDLE h,LPVOID out,DWORD size,LPDWORD done,LPOVERLAPPED ov){
 assert(h==reinterpret_cast<HANDLE>(0x2345)&&out==output&&size==512&&done&&ov==nullptr&&GetLastError()==301);*done=511;SetLastError(302);return TRUE;
}
static BOOL WINAPI WaitMock(PCONDITION_VARIABLE cv,PSRWLOCK lock,DWORD ms,ULONG flags){
 assert(cv==reinterpret_cast<PCONDITION_VARIABLE>(0x3456)&&lock==reinterpret_cast<PSRWLOCK>(0x4567)&&ms==99&&flags==1&&GetLastError()==401);SetLastError(402);return FALSE;
}
static void InterleaveMock(void* p){assert(p==owner&&GetLastError()==501);SetLastError(502);}
static unsigned char many[257*0x188]{};
static void LookupMany(void*,void* records,U64 count,int stride){assert(records==many&&count==257&&stride==0x188);SetLastError(601);}
static void OfferMock(const loader_candidate::Candidate* values,unsigned count,U64 selected,U64 selectedOwner){
 assert(!insideSelection&&count==1&&selected==7&&selectedOwner==reinterpret_cast<U64>(owner)&&values[0].range.offset==132&&values[0].range.bytes==512);++offered;SetLastError(999);
}
static bool EarlyMock(HANDLE,const loader_candidate::Candidate& value,void*,DWORD bytes,LPDWORD received){
 assert(value.owner==reinterpret_cast<U64>(owner)&&value.id==7&&value.pack==reinterpret_cast<U64>(pack)&&value.range.offset==132&&bytes==512);*received=512;SetLastError(998);return true;
}
static U64 PackEarlyMock(void*,void*,void* out,U64,U64){DWORD received=0;SetLastError(777);assert(ReadHook(reinterpret_cast<HANDLE>(0x2345),out,512,&received,nullptr)&&received==512&&GetLastError()==777);return received;}
int main(){
 lookup=&LookupMock;selectNew=&SelectMock;packRead=&PackMock;read=&ReadMock;wait=&WaitMock;interleave=&InterleaveMock;
 Put(pack,0x288,100000);Put(entry,0x20,100);Put(entry,0x28,4096);Put(handle,0,reinterpret_cast<U64>(entry));
 Put(record,0x178,7);Put(record,0x158,32);Put(record,0x150,512);
 SetLastError(101);assert(SelectNew(owner,record)&&GetLastError()==102&&selectionOwner==0&&candidates.stored==1);
 {
  WorkerScope scope(owner,record);assert(current==&scope.context&&current->candidate);
  SetLastError(201);assert(PackReadHook(pack,handle,output,32,512)==511&&GetLastError()==202&&rangeEqual==1);
  Put(entry,0x20,101);SetLastError(201);assert(PackReadHook(pack,handle,output,32,512)==511&&rangeMismatch==1);Put(entry,0x20,100);
  DWORD received=0;SetLastError(301);assert(ReadHook(reinterpret_cast<HANDLE>(0x2345),output,512,&received,nullptr)&&received==511&&GetLastError()==302&&readBytes==511);
  SetLastError(401);assert(!WaitHook(reinterpret_cast<PCONDITION_VARIABLE>(0x3456),reinterpret_cast<PSRWLOCK>(0x4567),99,1)&&GetLastError()==402);
  SetLastError(501);InterleaveHook(owner);assert(GetLastError()==502);
  {WorkerScope nested(owner,record);assert(current==&nested.context&&!current->candidate);}assert(current==&scope.context);
 }
 assert(current==nullptr&&noPackRead==0&&packTime.count==2&&readTime.count==1&&bufferWait.count==1&&interleaveTime.count==1&&leadTime.count==1);
 SetLastError(201);assert(PackReadHook(pack,handle,output,32,512)==511&&packTime.count==2&&GetLastError()==202);
 // Invalid and oversized batches do not dereference or walk unbounded native records.
 selectionOwner=0;SetLastError(101);LookupHook(pack,record,1,0x188);assert(candidates.stored==1);
 lookup=&LookupMany;selectionOwner=reinterpret_cast<U64>(owner);LookupHook(pack,many,257,0x188);
 assert(capped==1&&invalid==256&&inspected==257&&GetLastError()==601);selectionOwner=0;
 offerEarly=&OfferMock;SetLastError(101);lookup=&LookupMock;assert(SelectNew(owner,record)&&offered==1&&GetLastError()==102&&currentBatch==nullptr);
 readEarly=&EarlyMock;packRead=&PackEarlyMock;
 {WorkerScope scope(owner,record);const auto reads=readTime.count.load();assert(PackReadHook(pack,handle,output,32,512)==512&&readTime.count==reads&&!current->reading);}
 offerEarly=nullptr;readEarly=nullptr;
 puts("PASS R237 handoff: post-selection callback, copied range metadata, scoped serving and native API timing excludes served reads");
 puts("PASS R236 stage forwarding: five-argument read ABI, native results, LastError, resolved range comparison and nested worker scopes");
}
