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
alignas(8) static unsigned char loader[0x820]{},queueRecords[40*0x188]{};
static unsigned expectedCount=16;
static void CopyMock(void* out,void* selected){assert(insideSelection&&GetLastError()==701);memcpy(out,selected,0x188);SetLastError(702);}
static bool QueueSelect(void* self,void* out){
 assert(self==loader);insideSelection=true;SelectedCopyHook(out,queueRecords+39*0x188);assert(GetLastError()==702);insideSelection=false;return true;
}
static void OfferMock(const loader_candidate::Candidate* values,unsigned count,U64 selected,U64 selectedOwner){
 assert(!insideSelection&&count==expectedCount&&selected==40&&selectedOwner==reinterpret_cast<U64>(loader));
 for(unsigned i=0;i<count;++i){assert(values[i].id==39-i&&values[i].owner==selectedOwner&&values[i].range.offset==132&&values[i].range.bytes==512);}
 ++offered;SetLastError(999);
}
static unsigned CompleteMock(void* self,unsigned limit){assert(self==loader&&limit==17&&GetLastError()==801);SetLastError(802);return 9;}
static void ReleaseMock(void* self,void* loaded){assert(self==loader&&loaded==record&&GetLastError()==901);SetLastError(902);}
static unsigned NestedComplete(void* self,unsigned limit){
 assert(self==loader&&limit==18&&GetLastError()==803);native_loader_completion::complete=&CompleteMock;
 SetLastError(801);assert(native_loader_completion::Renderer(self,17)==9&&GetLastError()==802);SetLastError(804);return 10;
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
 // Snapshot the next records in native tail order, excluding the selected tail.
 for(unsigned i=0;i<40;++i){auto* r=queueRecords+i*0x188;memcpy(r,record,0x188);Put(r,0x178,i+1);}
 Put(loader,0x508,reinterpret_cast<U64>(queueRecords));Put(loader,0x510,40);
 offerEarly=&OfferMock;selectedCopy=&CopyMock;selectNew=&QueueSelect;
 SetLastError(701);assert(SelectNew(loader,record)&&offered==1&&GetLastError()==702&&currentBatch==nullptr&&queueCandidates==16);
 // Only 32 records may be examined: an eligible far-away record is ignored.
 for(unsigned i=7;i<39;++i)Put(queueRecords+i*0x188,0x120,0);
 SetLastError(701);assert(SelectNew(loader,record)&&offered==1&&queueCandidates==16&&queueSnapshots==2);
 // Incorrect list shape fails closed without an offer.
 Put(loader,0x510,39);SetLastError(701);assert(SelectNew(loader,record)&&offered==1&&queueShapeRejected==1);
 Put(record,0x178,7);
 // Completion wrappers retain limits, return values, LastError and exclude nested timing.
 using namespace native_loader_completion;complete=&CompleteMock;release=&ReleaseMock;
 SetLastError(801);assert(Runtime(loader,17)==9&&GetLastError()==802&&runtimeTime.count==1);
 SetLastError(901);ReleaseHook(loader,record);assert(GetLastError()==902&&releaseTime.count==1);
 complete=&NestedComplete;SetLastError(803);assert(Runtime(loader,18)==10&&GetLastError()==804&&runtimeTime.count==2&&rendererTime.count==0&&nested==1&&depth==0);
 Put(loader,0x818,3);unsigned heap=2;memcpy(loader+0x54c,&heap,4);ObserveWait(reinterpret_cast<U64>(loader));
 SetLastError(801);assert(Renderer(loader,17)==9&&GetLastError()==802&&rendererTime.count==1);
 assert(waitSamples==1&&waitingCompletions==3&&waitingHeap==2&&completionMax==3&&heapMax==2);
 readEarly=&EarlyMock;packRead=&PackEarlyMock;
 {WorkerScope scope(owner,record);const auto reads=readTime.count.load();assert(PackReadHook(pack,handle,output,32,512)==512&&readTime.count==reads&&!current->reading);}
 offerEarly=nullptr;readEarly=nullptr;
 puts("PASS R238 queue order, scan cap, selected exclusion, completion nesting and forwarding");
 puts("PASS R237 handoff: post-selection callback, copied range metadata, scoped serving and native API timing excludes served reads");
 puts("PASS R236 stage forwarding: five-argument read ABI, native results, LastError, resolved range comparison and nested worker scopes");
}
