#pragma once
#include "native_loader_candidate.h"
#include "rr_indirect_call_windows.h"
namespace native_loader_stages {
using U64=loader_trace::U64;
inline U64 Tick(){LARGE_INTEGER t{};QueryPerformanceCounter(&t);return static_cast<U64>(t.QuadPart);}
inline U64 Field(const void* p,unsigned offset){U64 v;memcpy(&v,static_cast<const unsigned char*>(p)+offset,8);return v;}
struct Metric {
    std::atomic<U64> count{0},ticks{0},maximum{0};
    void Add(U64 elapsed){++count;ticks.fetch_add(elapsed,std::memory_order_relaxed);loader_trace::Tracker::Max(maximum,elapsed);}
};
inline Metric readTime,packTime,bufferWait,interleaveTime,lookupTime,leadTime;
inline loader_candidate::Table candidates;
inline std::atomic<U64> batches{0},batchEntries{0},inspected{0},invalid{0},capped{0},rangeEqual{0},rangeMismatch{0},noPackRead{0},readBytes{0},readFailures{0},workers{0},newSelections{0},oldSelections{0};
inline thread_local U64 selectionOwner=0;
using OfferEarly=void(*)(const loader_candidate::Candidate*,unsigned,U64,U64);
using ReadEarly=bool(*)(HANDLE,const loader_candidate::Candidate&,void*,DWORD,LPDWORD);
inline OfferEarly offerEarly=nullptr;
inline ReadEarly readEarly=nullptr;
struct Batch {loader_candidate::Candidate entries[16]{};unsigned count=0;};
inline thread_local Batch* currentBatch=nullptr;
struct Context {bool candidate=false,observed=false,reading=false;U64 owner=0,id=0;loader_candidate::Candidate value{},actual{};};
inline thread_local Context* current=nullptr;
struct WorkerScope {
    Context context{};Context* previous=current;
    WorkerScope(void* self,void* record){
        context.owner=reinterpret_cast<U64>(self);context.id=Field(record,0x178);
        const auto now=Tick();context.candidate=candidates.Take(Field(record,0x178),reinterpret_cast<U64>(self),now,context.value);
        if(context.candidate)leadTime.Add(now-context.value.ticks);current=&context;++workers;
    }
    ~WorkerScope(){if(context.candidate&&!context.observed)++noPackRead;current=previous;}
};
using Select=bool(*)(void*,void*);
inline Select selectNew=nullptr,selectOld=nullptr;
inline bool SelectCall(void* self,void* out,Select original){
    Batch batch{};
    struct Scope {U64 previous;Batch* batch;~Scope(){selectionOwner=previous;currentBatch=batch;}} scope{selectionOwner,currentBatch};
    selectionOwner=reinterpret_cast<U64>(self);currentBatch=offerEarly?&batch:nullptr;
    const bool result=original(self,out);const DWORD error=GetLastError();
    // selectNextFile has released native locks. Only copied scalars cross here.
    if(result&&offerEarly&&batch.count)offerEarly(batch.entries,batch.count,Field(out,0x178),selectionOwner);
    SetLastError(error);return result;
}
inline bool SelectNew(void* self,void* out){++newSelections;return SelectCall(self,out,selectNew);}
inline bool SelectOld(void* self,void* out){++oldSelections;return SelectCall(self,out,selectOld);}
using Lookup=void(*)(void*,void*,U64,int);
inline Lookup lookup=nullptr;
inline void LookupHook(void* self,void* records,U64 count,int stride){
    const DWORD incoming=GetLastError();const auto begin=Tick();SetLastError(incoming);
    lookup(self,records,count,stride);const DWORD error=GetLastError();lookupTime.Add(Tick()-begin);
    ++batches;batchEntries.fetch_add(count,std::memory_order_relaxed);
    if(selectionOwner&&records&&stride==0x188){
        // Bound observer work while native selection holds its lock. No I/O here.
        const U64 limit=count<256?count:256;capped.fetch_add(count-limit,std::memory_order_relaxed);
        for(U64 i=0;i<limit;++i){
            auto* record=static_cast<unsigned char*>(records)+i*0x188;++inspected;
            auto* pack=reinterpret_cast<void*>(Field(record,0x120));loader_candidate::Range range{};
            if(!pack||!loader_candidate::Resolve(Field(record,0x128),Field(pack,0x288),Field(record,0x130),Field(record,0x158),Field(record,0x150),range)){++invalid;continue;}
            const loader_candidate::Candidate value{Field(record,0x178),selectionOwner,reinterpret_cast<U64>(pack),Tick(),range};
            candidates.Put(value);
            if(currentBatch&&currentBatch->count<16&&range.bytes<=2*1024*1024)currentBatch->entries[currentBatch->count++]=value;
        }
    }
    SetLastError(error);
}
using PackRead=U64(*)(void*,void*,void*,U64,U64);
inline PackRead packRead=nullptr;
inline U64 PackReadHook(void* pack,void* handle,void* output,U64 offset,U64 count){
    if(!current)return packRead(pack,handle,output,offset,count);
    const DWORD incoming=GetLastError();
    auto* entry=reinterpret_cast<void*>(Field(handle,0));loader_candidate::Range actual{};
    const bool valid=entry&&loader_candidate::Resolve(Field(entry,0x20),0,Field(entry,0x28),offset,count,actual);
    if(current->candidate){
        current->observed=true;const auto& expected=current->value;
        if(valid&&expected.pack==reinterpret_cast<U64>(pack)&&expected.range.offset==actual.offset&&expected.range.bytes==actual.bytes)++rangeEqual;else ++rangeMismatch;
    }
    struct Scope {Context* context;bool reading;loader_candidate::Candidate previous;~Scope(){context->reading=reading;context->actual=previous;}} scope{current,current->reading,current->actual};
    current->reading=valid;current->actual={current->id,current->owner,reinterpret_cast<U64>(pack),0,actual};
    const auto begin=Tick();SetLastError(incoming);const auto result=packRead(pack,handle,output,offset,count);
    const DWORD error=GetLastError();packTime.Add(Tick()-begin);SetLastError(error);return result;
}
using Read=BOOL(WINAPI*)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
inline Read read=nullptr;
inline BOOL WINAPI ReadHook(HANDLE file,LPVOID out,DWORD bytes,LPDWORD received,LPOVERLAPPED overlapped){
    if(!current)return read(file,out,bytes,received,overlapped);
    const DWORD incoming=GetLastError();
    if(readEarly&&current->reading&&!overlapped&&readEarly(file,current->actual,out,bytes,received)){SetLastError(incoming);return TRUE;}
    const auto begin=Tick();SetLastError(incoming);
    const BOOL result=read(file,out,bytes,received,overlapped);const DWORD error=GetLastError();
    readTime.Add(Tick()-begin);if(result&&received)readBytes.fetch_add(*received,std::memory_order_relaxed);if(!result)++readFailures;
    SetLastError(error);return result;
}
using Wait=BOOL(WINAPI*)(PCONDITION_VARIABLE,PSRWLOCK,DWORD,ULONG);
inline Wait wait=nullptr;
inline BOOL WINAPI WaitHook(PCONDITION_VARIABLE condition,PSRWLOCK lock,DWORD milliseconds,ULONG flags){
    if(!current)return wait(condition,lock,milliseconds,flags);
    const DWORD incoming=GetLastError();const auto begin=Tick();SetLastError(incoming);
    const BOOL result=wait(condition,lock,milliseconds,flags);const DWORD error=GetLastError();bufferWait.Add(Tick()-begin);SetLastError(error);return result;
}
using Interleave=void(*)(void*);
inline Interleave interleave=nullptr;
inline void InterleaveHook(void* self){
    if(!current){interleave(self);return;}
    const DWORD incoming=GetLastError();const auto begin=Tick();SetLastError(incoming);interleave(self);
    const DWORD error=GetLastError();interleaveTime.Add(Tick()-begin);SetLastError(error);
}
inline void Poll(U64 present,double scale){
    Log("LOADER_R236_CANDIDATES present=%llu new_select=%llu old_select=%llu batches=%llu entries=%llu inspected=%llu capped=%llu invalid=%llu stored=%llu matched=%llu missing=%llu dropped=%llu replaced=%llu range_equal=%llu range_mismatch=%llu no_pack_read=%llu workers=%llu read_bytes=%llu read_failures=%llu",
        present,newSelections.load(),oldSelections.load(),batches.load(),batchEntries.load(),inspected.load(),capped.load(),invalid.load(),candidates.stored.load(),candidates.matched.load(),candidates.missing.load(),candidates.dropped.load(),candidates.replaced.load(),rangeEqual.load(),rangeMismatch.load(),noPackRead.load(),workers.load(),readBytes.load(),readFailures.load());
    const char* names[]={"readfile","pack_read","buffer_wait","interleave","lookup","resolved_lead"};
    Metric* metrics[]={&readTime,&packTime,&bufferWait,&interleaveTime,&lookupTime,&leadTime};
    for(unsigned i=0;i<6;++i){auto& m=*metrics[i];Log("LOADER_R236_STAGE present=%llu stage=%s count=%llu total_ms=%.3f max_ms=%.3f",present,names[i],m.count.load(),m.ticks.load()*scale,m.maximum.load()*scale);}
}
}
