#pragma once
#include <climits>
#include "directstorage_archive_cache.h"
#include "native_loader_candidate.h"
namespace control_storage {
// Request-owned early reads. Game threads use try-locks only. The worker owns
// path resolution, file opening and DS submission; no native pointer is retained.
class EarlyReader {
    using U64=std::uint64_t;
    enum class FileState {Empty,Opening,Ready,Failed};
    enum class State {Free,Queued,Pending,Ready,Discard};
    struct File {
        U64 pack=0,epoch=0,serial=0;FileState state=FileState::Empty;
        HANDLE duplicate=INVALID_HANDLE_VALUE,guard=INVALID_HANDLE_VALUE;
        BY_HANDLE_FILE_INFORMATION info{};ComPtr<IDStorageFile> source;
    };
    struct Job {
        loader_candidate::Candidate request{};State state=State::Free;
        unsigned file=0;U64 admitted=0,epoch=0;unsigned char* data=nullptr;
    };
    File files[FileCount];Job jobs[BlockCount];
    ComPtr<IDStorageFactory> factory;ComPtr<IDStorageQueue> queue;ComPtr<IDStorageStatusArray> status;
    SRWLOCK lock=SRWLOCK_INIT;HANDLE wake=nullptr,thread=nullptr;
    std::atomic<bool> stop{false};std::atomic<U64> epoch{1};U64 fileSerial=0;bool disabled=false;
    struct Unlock {SRWLOCK* value;~Unlock(){ReleaseSRWLockExclusive(value);}};
    struct PreserveError {DWORD value=GetLastError();~PreserveError(){SetLastError(value);}};
    static constexpr U64 LifetimeMs=250;
    static bool SameRequest(const loader_candidate::Candidate& a,const loader_candidate::Candidate& b){return a.owner==b.owner&&a.id==b.id;}
    int LatestFile(U64 pack)const {int result=-1;U64 serial=0;for(unsigned i=0;i<FileCount;++i)if(files[i].state!=FileState::Empty&&files[i].epoch==epoch.load()&&files[i].pack==pack&&files[i].serial>serial){result=static_cast<int>(i);serial=files[i].serial;}return result;}
    int ObserveFile(HANDLE native,U64 pack,const BY_HANDLE_FILE_INFORMATION& info){
        int existing=LatestFile(pack);
        if(existing>=0&&SameFile(files[existing].info,info))return existing;
        for(unsigned i=0;i<FileCount;++i)if(files[i].state==FileState::Empty){
            HANDLE owned=INVALID_HANDLE_VALUE;
            if(!DuplicateHandle(GetCurrentProcess(),native,GetCurrentProcess(),&owned,0,FALSE,DUPLICATE_SAME_ACCESS)){++counters.registrationFailed;return -1;}
            auto& f=files[i];f.pack=pack;f.epoch=epoch.load();f.serial=++fileSerial;f.info=info;f.duplicate=owned;f.state=FileState::Opening;
            ++counters.registered;SetEvent(wake);return static_cast<int>(i);
        }
        ++counters.fileCapacity;return -1;
    }
    void OpenFiles(){
        for(auto& f:files)if(f.state==FileState::Opening){
            wchar_t path[1024]{};const DWORD length=GetFinalPathNameByHandleW(f.duplicate,path,_countof(path),FILE_NAME_NORMALIZED);
            bool ok=length&&length<_countof(path)&&ArchivePath(path);
            if(ok){
                f.guard=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                BY_HANDLE_FILE_INFORMATION held{},direct{};
                ok=f.guard!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(f.guard,&held)&&SameFile(f.info,held)&&
                    SUCCEEDED(factory->OpenFile(path,IID_PPV_ARGS(&f.source)))&&SUCCEEDED(f.source->GetFileInformation(&direct))&&SameFile(f.info,direct);
            }
            CloseHandle(f.duplicate);f.duplicate=INVALID_HANDLE_VALUE;
            f.state=ok?FileState::Ready:FileState::Failed;
            if(ok)++counters.filesReady;
            else {++counters.registrationFailed;f.source.Reset();if(f.guard!=INVALID_HANDLE_VALUE){CloseHandle(f.guard);f.guard=INVALID_HANDLE_VALUE;}}
        }
    }
    void Harvest(U64 now){
        const auto currentEpoch=epoch.load();
        for(unsigned i=0;i<BlockCount;++i){auto& j=jobs[i];
            if(j.epoch!=currentEpoch){
                if(j.state==State::Pending)j.state=State::Discard;
                else if(j.state==State::Ready){counters.unusedBytes.fetch_add(j.request.range.bytes);j.state=State::Free;}
                else if(j.state==State::Queued)j.state=State::Free;
            }
            if((j.state==State::Pending||j.state==State::Discard)&&status->IsComplete(i)){
                const bool ok=SUCCEEDED(status->GetHResult(i));
                if(!ok){++counters.ioFailures;disabled=true;}
                if(!ok||j.state==State::Discard||now-j.admitted>=LifetimeMs){counters.unusedBytes.fetch_add(j.request.range.bytes);j.state=State::Free;}
                else {j.state=State::Ready;++counters.completed;}
            }
            if(j.state==State::Ready&&now-j.admitted>=LifetimeMs){counters.unusedBytes.fetch_add(j.request.range.bytes);++counters.expired;j.state=State::Free;}
            if(j.state==State::Queued&&(disabled||now-j.admitted>=LifetimeMs)){++counters.expired;j.state=State::Free;}
        }
    }
    void RetireFiles(){
        const auto currentEpoch=epoch.load();
        for(unsigned f=0;f<FileCount;++f)if(files[f].state!=FileState::Empty&&files[f].epoch!=currentEpoch){
            bool pending=false;for(const auto& j:jobs)if(j.file==f&&(j.state==State::Pending||j.state==State::Discard)){pending=true;break;}
            if(pending)continue;
            auto& file=files[f];file.source.Reset();
            if(file.duplicate!=INVALID_HANDLE_VALUE)CloseHandle(file.duplicate);
            if(file.guard!=INVALID_HANDLE_VALUE)CloseHandle(file.guard);
            file=File{};
        }
    }
    void SubmitQueued(){
        bool submitted=false;
        for(unsigned i=0;i<BlockCount;++i){auto& j=jobs[i];if(j.state!=State::Queued)continue;
            auto& f=files[j.file];if(f.state!=FileState::Ready){j.state=State::Free;continue;}
            DSTORAGE_REQUEST request{};request.Options.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;
            request.Options.DestinationType=DSTORAGE_REQUEST_DESTINATION_MEMORY;
            request.Options.CompressionFormat=DSTORAGE_COMPRESSION_FORMAT_NONE;
            const auto bytes=static_cast<UINT32>(j.request.range.bytes);
            request.Source.File={f.source.Get(),j.request.range.offset,bytes};request.Destination.Memory={j.data,bytes};request.UncompressedSize=bytes;
            queue->EnqueueRequest(&request);queue->EnqueueStatus(status.Get(),i);j.state=State::Pending;
            ++counters.submitted;counters.submittedBytes.fetch_add(bytes);submitted=true;
        }
        if(submitted){queue->Submit();++counters.batches;}
    }
    static DWORD WINAPI Run(void* value){return static_cast<EarlyReader*>(value)->Loop();}
    DWORD Loop(){
        for(;;){
            bool active=false,stopping=false;
            {
                AcquireSRWLockExclusive(&lock);Unlock unlock{&lock};stopping=stop.load();
                if(stopping){
                    for(auto& j:jobs){if(j.state==State::Pending)j.state=State::Discard;else if(j.state==State::Queued||j.state==State::Ready)j.state=State::Free;}
                    queue->CancelRequestsWithTag(0,0);
                }
                Harvest(GetTickCount64());RetireFiles();if(!stopping&&!disabled){OpenFiles();SubmitQueued();}
                for(const auto& j:jobs)if(j.state!=State::Free)active=true;
            }
            if(stopping&&!active)return 0;
            WaitForSingleObject(wake,active?2:INFINITE);
        }
    }
public:
    struct Counters {
        std::atomic<U64> registered{0},filesReady{0},registrationFailed{0},fileCapacity{0},offered{0},admitted{0},cold{0},capacity{0},busy{0},duplicates{0};
        std::atomic<U64> invalidations{0},completed{0},submitted{0},submittedBytes{0},batches{0},served{0},servedBytes{0},late{0},avoided{0},expired{0},unusedBytes{0},ioFailures{0},identityRejected{0},positionRejected{0};
    } counters;
    void Invalidate(){PreserveError error;epoch.fetch_add(1);++counters.invalidations;if(wake)SetEvent(wake);}
    bool Initialize(IDStorageFactory* value){
        if(!value)return false;factory=value;
        DSTORAGE_QUEUE_DESC desc{};desc.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;desc.Capacity=128;
        desc.Priority=DSTORAGE_PRIORITY_LOW;desc.Name="ControlFG early archive requests";
        if(FAILED(factory->CreateQueue(&desc,IID_PPV_ARGS(&queue)))||FAILED(factory->CreateStatusArray(BlockCount,"ControlFG early completion",IID_PPV_ARGS(&status))))return false;
        for(auto& j:jobs){j.data=static_cast<unsigned char*>(VirtualAlloc(nullptr,BlockBytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!j.data)return false;}
        wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!wake)return false;
        thread=CreateThread(nullptr,0,&Run,this,0,nullptr);return thread!=nullptr;
    }
    void Offer(const loader_candidate::Candidate* values,unsigned count,U64 selectedId,U64 selectedOwner){
        PreserveError error;
        if(stop.load()||!queue||!values)return;
        if(!TryAcquireSRWLockExclusive(&lock)){++counters.busy;return;}Unlock unlock{&lock};if(disabled)return;
        const U64 now=GetTickCount64();Harvest(now);if(disabled)return;bool signal=false;
        for(unsigned n=0;n<count&&n<16;++n){const auto& v=values[n];++counters.offered;
            if(!v.id||!v.owner||!v.pack||!v.range.bytes||v.range.bytes>BlockBytes||(v.id==selectedId&&v.owner==selectedOwner))continue;
            const int f=LatestFile(v.pack);if(f<0||files[f].state!=FileState::Ready){++counters.cold;continue;}
            const auto& info=files[f].info;const U64 length=(U64(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
            if(v.range.offset>length||v.range.bytes>length-v.range.offset)continue;
            bool duplicate=false;for(const auto& j:jobs)if(j.state!=State::Free&&SameRequest(j.request,v)){duplicate=true;break;}
            if(duplicate){++counters.duplicates;continue;}
            Job* free=nullptr;for(auto& j:jobs)if(j.state==State::Free){free=&j;break;}
            if(!free){++counters.capacity;continue;}
            free->request=v;free->file=static_cast<unsigned>(f);free->admitted=now;free->epoch=epoch.load();free->state=State::Queued;++counters.admitted;signal=true;
        }
        if(signal)SetEvent(wake);
    }
    // Called at the audited synchronous ReadFile site in a registered PackFile
    // worker scope. A miss only retires that request; it NEVER submits new I/O.
    bool TryConsume(HANDLE native,const loader_candidate::Candidate& actual,void* destination,DWORD count,LPDWORD received){
        PreserveError error;
        if(stop.load()||!queue||!native||native==INVALID_HANDLE_VALUE||!destination||!received||!count||count>BlockBytes||count!=actual.range.bytes)return false;
        BY_HANDLE_FILE_INFORMATION info{};if(!GetFileInformationByHandle(native,&info))return false;
        if(!TryAcquireSRWLockExclusive(&lock)){++counters.busy;return false;}Unlock unlock{&lock};if(disabled)return false;
        ObserveFile(native,actual.pack,info);Harvest(GetTickCount64());if(disabled)return false;
        for(auto& j:jobs)if(j.state!=State::Free&&SameRequest(j.request,actual)){
            const bool same=j.request.pack==actual.pack&&j.request.range.offset==actual.range.offset&&j.request.range.bytes==actual.range.bytes&&SameFile(files[j.file].info,info);
            if(!same)++counters.identityRejected;
            if(same&&j.state==State::Ready){
                LARGE_INTEGER zero{},position{},next{};next.QuadPart=static_cast<LONGLONG>(actual.range.offset+count);
                if(actual.range.offset<=static_cast<U64>(LLONG_MAX)-count&&SetFilePointerEx(native,zero,&position,FILE_CURRENT)&&
                    position.QuadPart>=0&&static_cast<U64>(position.QuadPart)==actual.range.offset&&SetFilePointerEx(native,next,nullptr,FILE_BEGIN)){
                    memcpy(destination,j.data,count);*received=count;j.state=State::Free;++counters.served;counters.servedBytes.fetch_add(count);return true;
                }
                ++counters.positionRejected;
            }
            if(j.state==State::Pending){j.state=State::Discard;++counters.late;}
            else if(j.state==State::Queued){j.state=State::Free;++counters.avoided;}
            else if(j.state==State::Ready){counters.unusedBytes.fetch_add(j.request.range.bytes);j.state=State::Free;}
            return false;
        }
        return false;
    }
    // Test/init-failure only, outside DllMain. On timeout caller must retain this
    // object: pending destinations and sources cannot be destroyed early.
    bool Stop(DWORD timeout=30000){
        stop=true;if(wake)SetEvent(wake);
        if(thread&&WaitForSingleObject(thread,timeout)!=WAIT_OBJECT_0)return false;
        if(thread){CloseHandle(thread);thread=nullptr;}if(wake){CloseHandle(wake);wake=nullptr;}
        if(queue)queue->Close();queue.Reset();status.Reset();
        for(auto& j:jobs)if(j.data){VirtualFree(j.data,0,MEM_RELEASE);j.data=nullptr;}
        for(auto& f:files){f.source.Reset();if(f.guard!=INVALID_HANDLE_VALUE)CloseHandle(f.guard);if(f.duplicate!=INVALID_HANDLE_VALUE)CloseHandle(f.duplicate);f.guard=f.duplicate=INVALID_HANDLE_VALUE;}
        factory.Reset();return true;
    }
};
}
