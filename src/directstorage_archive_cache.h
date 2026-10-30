#pragma once
#include <windows.h>
#include <dstorage.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cwchar>
namespace control_storage {
using Microsoft::WRL::ComPtr;
constexpr unsigned BlockBytes=2*1024*1024,BlockCount=16,FileCount=16;
inline bool SameFile(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b) noexcept {
    return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&
        a.nFileIndexLow==b.nFileIndexLow&&a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow&&
        a.ftLastWriteTime.dwHighDateTime==b.ftLastWriteTime.dwHighDateTime&&a.ftLastWriteTime.dwLowDateTime==b.ftLastWriteTime.dwLowDateTime;
}
inline bool ArchivePath(const wchar_t* path) noexcept {
    const auto n=wcslen(path);return n>=5&&_wcsicmp(path+n-5,L".rmdp")==0;
}
struct Counters {
    std::atomic<unsigned long long> requests{0},hits{0},hitBytes{0},submittedBytes{0},failures{0},busy{0};
};
// Process-owned storage: never retain the game's destination, OVERLAPPED or
// event. Only completed, successful blocks can be copied into a native request.
class ArchiveCache {
    struct File {ComPtr<IDStorageFile> source;HANDLE guard=INVALID_HANDLE_VALUE;BY_HANDLE_FILE_INFORMATION info{};};
    struct Block {unsigned char* data=nullptr;int file=-1;std::uint64_t offset=0,age=0;unsigned size=0;bool pending=false,ready=false;};
    File files[FileCount];Block blocks[BlockCount];
    ComPtr<IDStorageFactory> factory;ComPtr<IDStorageQueue> queue;ComPtr<IDStorageStatusArray> status;
    SRWLOCK lock=SRWLOCK_INIT;std::uint64_t clock=0;bool disabled=false;
    struct Unlock {SRWLOCK* value;~Unlock(){ReleaseSRWLockExclusive(value);}};
    int FindFile(HANDLE native,const BY_HANDLE_FILE_INFORMATION& info) noexcept {
        int freeSlot=-1;
        for(unsigned i=0;i<FileCount;++i){
            if(files[i].source&&SameFile(files[i].info,info))return static_cast<int>(i);
            if(!files[i].source&&freeSlot<0)freeSlot=static_cast<int>(i);
        }
        if(freeSlot<0)return -1;
        wchar_t path[1024]{};const DWORD length=GetFinalPathNameByHandleW(native,path,_countof(path),FILE_NAME_NORMALIZED);
        if(!length||length>=_countof(path)||!ArchivePath(path))return -1;
        // Deny mutation/deletion while cached. If a writer is already open,
        // this fails and the entire request remains on the native path.
        HANDLE guard=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(guard==INVALID_HANDLE_VALUE)return -1;
        BY_HANDLE_FILE_INFORMATION held{},direct{};ComPtr<IDStorageFile> source;
        const bool ok=GetFileInformationByHandle(guard,&held)&&SameFile(info,held)&&
            SUCCEEDED(factory->OpenFile(path,IID_PPV_ARGS(&source)))&&
            SUCCEEDED(source->GetFileInformation(&direct))&&SameFile(info,direct);
        if(!ok){CloseHandle(guard);return -1;}
        auto& f=files[freeSlot];f.guard=guard;f.info=info;f.source=source;return freeSlot;
    }
    void Harvest() noexcept {
        for(unsigned i=0;i<BlockCount;++i){auto& b=blocks[i];
            if(b.pending&&status->IsComplete(i)){
                b.pending=false;b.ready=SUCCEEDED(status->GetHResult(i));
                if(!b.ready){b.file=-1;disabled=true;counters.failures.fetch_add(1,std::memory_order_relaxed);}
            }
        }
    }
    bool Prefetch(int file,std::uint64_t offset,std::uint64_t fileSize) noexcept {
        if(offset>=fileSize)return false;
        for(const auto& b:blocks)if(b.file==file&&b.offset==offset&&(b.pending||b.ready))return false;
        unsigned choice=BlockCount;std::uint64_t oldest=UINT64_MAX;
        for(unsigned i=0;i<BlockCount;++i){const auto& b=blocks[i];
            if(!b.pending&&b.age<oldest){choice=i;oldest=b.age;}
        }
        if(choice==BlockCount)return false;
        auto& b=blocks[choice];b.file=file;b.offset=offset;b.age=++clock;b.ready=false;
        const auto available=fileSize-offset;b.size=available<BlockBytes?static_cast<unsigned>(available):BlockBytes;
        DSTORAGE_REQUEST request{};
        request.Options.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;
        request.Options.DestinationType=DSTORAGE_REQUEST_DESTINATION_MEMORY;
        request.Options.CompressionFormat=DSTORAGE_COMPRESSION_FORMAT_NONE;
        request.Source.File={files[file].source.Get(),offset,b.size};
        request.Destination.Memory={b.data,b.size};request.UncompressedSize=b.size;
        queue->EnqueueRequest(&request);queue->EnqueueStatus(status.Get(),choice);b.pending=true;
        counters.submittedBytes.fetch_add(b.size,std::memory_order_relaxed);return true;
    }
public:
    Counters counters;
    void CancelPrefetch() noexcept {
        AcquireSRWLockExclusive(&lock);Unlock unlock{&lock};disabled=true;
        if(queue)queue->CancelRequestsWithTag(0,0);
    }
    bool Initialize(IDStorageFactory* source) noexcept {
        if(!source)return false;factory=source;
        DSTORAGE_QUEUE_DESC desc{};desc.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;
        desc.Capacity=128;desc.Priority=DSTORAGE_PRIORITY_LOW;desc.Name="ControlFG archive prefetch";
        if(FAILED(factory->CreateQueue(&desc,IID_PPV_ARGS(&queue)))||
            FAILED(factory->CreateStatusArray(BlockCount,"ControlFG archive blocks",IID_PPV_ARGS(&status))))return false;
        for(auto& b:blocks){b.data=static_cast<unsigned char*>(VirtualAlloc(nullptr,BlockBytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!b.data)return false;}
        return true;
    }
    bool TryRead(HANDLE native,std::uint64_t offset,void* destination,unsigned count) noexcept {
        counters.requests.fetch_add(1,std::memory_order_relaxed);
        if(!count||count>BlockBytes||!destination||!queue)return false;
        BY_HANDLE_FILE_INFORMATION info{};
        if(!GetFileInformationByHandle(native,&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))return false;
        const std::uint64_t size=(std::uint64_t(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
        if(offset>size||count>size-offset)return false;
        if(!TryAcquireSRWLockExclusive(&lock)){counters.busy.fetch_add(1,std::memory_order_relaxed);return false;}
        Unlock unlock{&lock};const int file=FindFile(native,info);if(file<0)return false;
        Harvest();if(disabled)return false;bool hit=false;
        for(auto& b:blocks)if(b.ready&&b.file==file&&offset>=b.offset&&offset-b.offset<=b.size&&count<=b.size-(offset-b.offset)){
            memcpy(destination,b.data+static_cast<size_t>(offset-b.offset),count);b.age=++clock;hit=true;break;
        }
        // At most 16 outstanding requests + 16 statuses, below queue capacity.
        // Never recycle an in-flight destination or wait for its completion.
        const auto base=offset-offset%BlockBytes;
        bool submitted=Prefetch(file,base,size);submitted=Prefetch(file,base+BlockBytes,size)||submitted;
        if(submitted)queue->Submit();
        if(hit){counters.hits.fetch_add(1,std::memory_order_relaxed);counters.hitBytes.fetch_add(count,std::memory_order_relaxed);}
        return hit;
    }
    // Explicit teardown for tests / initialization failure, outside DllMain.
    // Production owns this object until process exit. Refuse to free in-flight memory.
    bool CloseIfIdle() noexcept {
        AcquireSRWLockExclusive(&lock);Unlock unlock{&lock};
        if(status)Harvest();for(const auto& b:blocks)if(b.pending)return false;
        if(queue)queue->Close();queue.Reset();status.Reset();
        for(auto& b:blocks)if(b.data){VirtualFree(b.data,0,MEM_RELEASE);b.data=nullptr;}
        for(auto& f:files){f.source.Reset();if(f.guard!=INVALID_HANDLE_VALUE){CloseHandle(f.guard);f.guard=INVALID_HANDLE_VALUE;}}
        factory.Reset();return true;
    }
};
}
