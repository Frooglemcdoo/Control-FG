#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <new>
#include <cassert>
#include <cstring>
#include <cstdint>
// Installation is audited separately against the supplied runtime. Exercise
// the real cache and ReadFile hook against real files and DirectStorage here.
static HMODULE selfModule=nullptr;
static bool HashMatches(HMODULE,const char*){return false;}
static std::wstring ModulePath(HMODULE){return {};}
static void Log(const char*,...) noexcept {}
static void* AllocateExecutableRelayNear(const void*) noexcept {return nullptr;}
static void** FindImport(HMODULE,const char*,const char*) {return nullptr;}
#include "../../src/directstorage_game_runtime.h"
static void Check(bool condition,const char* label){if(!condition){printf("FAIL %s error=%lu\n",label,GetLastError());fflush(stdout);ExitProcess(1);}}
static int testThreadIndex=1;
static int TestIndex(){return testThreadIndex;}
struct NativeFileFixture {void* unused=nullptr;HANDLE handle=nullptr;std::uint64_t mode=0,position=0;};
static std::uint64_t NativeSync(void*,void* opaque,void* output,std::uint64_t count){
    auto* f=static_cast<NativeFileFixture*>(opaque);DWORD got=0;
    if(!native_directstorage::SyncReadHook(f->handle,output,static_cast<DWORD>(count),&got,nullptr))return 0;
    f->position+=got;return got;
}
int wmain(){
    using namespace control_storage;
    ComPtr<IDStorageFactory> factory;Check(SUCCEEDED(DStorageGetFactory(IID_PPV_ARGS(&factory))),"factory");
    ArchiveCache cache;Check(cache.Initialize(factory.Get()),"initialize");
    wchar_t temp[MAX_PATH]{},base[MAX_PATH]{};Check(GetTempPathW(MAX_PATH,temp)!=0,"temp");
    Check(GetTempFileNameW(temp,L"cfa",0,base)!=0,"tempname");DeleteFileW(base);
    const std::wstring path=std::wstring(base)+L".rmdp",other=std::wstring(base)+L"-other.rmdp";
    const unsigned size=BlockBytes*3+131;
    std::vector<unsigned char> expected(size);unsigned random=0x851a32;
    for(auto& b:expected){random^=random<<13;random^=random>>17;random^=random<<5;b=static_cast<unsigned char>(random);}
    for(const auto& name:{path,other}){
        HANDLE file=CreateFileW(name.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        Check(file!=INVALID_HANDLE_VALUE,"create");DWORD written=0;
        if(name==other)for(auto& b:expected)b^=0x5a;
        Check(WriteFile(file,expected.data(),size,&written,nullptr)&&written==size,"write");CloseHandle(file);
        if(name==other)for(auto& b:expected)b^=0x5a;
    }
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
    Check(file!=INVALID_HANDLE_VALUE,"open");
    HANDLE syncFile=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(syncFile!=INVALID_HANDLE_VALUE,"sync open");
    native_directstorage::original=&ReadFile;
    unsigned char syncOutput[127]{};DWORD syncReceived=0;
    for(unsigned index=0;index<65;++index){
        Check(native_directstorage::SyncReadHook(syncFile,syncOutput,127,&syncReceived,nullptr)&&syncReceived==127,"sync passthrough");
        Check(memcmp(syncOutput,expected.data()+index*127,127)==0,"sync bytes and implicit position");
    }
    Check(native_directstorage::syncCalls.load()==65&&native_directstorage::syncSamples.load()==2,"sync sampling stride");
    Check(native_directstorage::syncArchiveSamples.load()==2&&native_directstorage::syncBytes.load()==65*127,"sync archive identification and byte count");
    Check(!native_directstorage::SyncReadHook(INVALID_HANDLE_VALUE,syncOutput,127,&syncReceived,nullptr)&&GetLastError()==ERROR_INVALID_HANDLE,"sync failure preserves error");
    CloseHandle(syncFile);
    HANDLE event=CreateEventW(nullptr,TRUE,FALSE,nullptr);Check(event!=nullptr,"event");
    native_directstorage::original=&ReadFile;native_directstorage::cache=&cache;
    unsigned char output[4096];memset(output,0xcc,sizeof(output));OVERLAPPED ov{};ov.hEvent=event;ov.Offset=713;
    DWORD received=0;const BOOL first=native_directstorage::ReadHook(file,output,1367,&received,&ov);
    if(!first)Check(GetLastError()==ERROR_IO_PENDING,"native fallback pending");
    Check(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"native fallback event");
    Check(GetOverlappedResult(file,&ov,&received,FALSE)&&received==1367,"native fallback result");
    Check(memcmp(output,expected.data()+713,1367)==0&&output[1367]==0xcc,"native fallback bytes");
    Check(native_directstorage::fallbacks.load()==1,"cold native fallback");
    const auto deadline=GetTickCount64()+30000;
    while(!cache.TryRead(file,713,output,1367)){
        Check(GetTickCount64()<deadline,"cache completion timeout");Sleep(1);
    }
    ResetEvent(event);ov={};ov.Offset=713;ov.hEvent=event;received=0;memset(output,0xcc,sizeof(output));SetLastError(123);
    Check(native_directstorage::ReadHook(file,output,1367,&received,&ov)!=FALSE,"cache hook hit");
    Check(GetLastError()==123&&received==1367&&ov.Internal==0&&ov.InternalHigh==1367,"hit result ABI");
    Check(WaitForSingleObject(event,0)==WAIT_OBJECT_0,"hit completion event");
    Check(memcmp(output,expected.data()+713,1367)==0&&output[1367]==0xcc,"hit exact bytes");
    Check(native_directstorage::served.load()==1,"real served counter");
    // The synthetic leaf reproduces mov rax,[rcx]; jmp [rax+90h]. It
    // exercises the real tail patch, scoped wrapper and native bookkeeping.
    syncFile=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(syncFile!=INVALID_HANDLE_VALUE,"sync cache open");
    NativeFileFixture nativeFile{};nativeFile.handle=syncFile;
    void* table[19]{};table[18]=reinterpret_cast<void*>(&NativeSync);void** fs=table;
    native_directstorage::nativeRead=&NativeSync;native_directstorage::threadIndex=&TestIndex;
    auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    Check(code!=nullptr,"tail fixture allocate");
    const unsigned char leaf[]={0x48,0x8b,0x01,0x48,0xff,0xa0,0x90,0,0,0};memcpy(code,leaf,sizeof(leaf));
    control_storage::PackTailPatch tailPatch;
    Check(tailPatch.Prepare(code+3,reinterpret_cast<void*>(&native_directstorage::PackReadHook),code+4096),"tail prepare");
    Check(tailPatch.Exchange(true)&&tailPatch.healthy,"tail install");
    auto invoke=reinterpret_cast<native_directstorage::NativeRead>(code);
    LARGE_INTEGER seek{};seek.QuadPart=713;Check(SetFilePointerEx(syncFile,seek,nullptr,FILE_BEGIN)!=FALSE,"sync seek");nativeFile.position=713;
    SetLastError(123);memset(output,0xcc,sizeof(output));
    Check(invoke(&fs,&nativeFile,output,1367)==1367,"sync cache through tail");
    Check(GetLastError()==123&&nativeFile.position==2080,"sync last error and native bookkeeping");
    Check(native_directstorage::syncServed.load()==1&&native_directstorage::ownedArchiveHandle==nullptr,"scoped cache served and restored");
    LARGE_INTEGER current{},zero{};Check(SetFilePointerEx(syncFile,zero,&current,FILE_CURRENT)&&current.QuadPart==2080,"OS pointer advanced");
    Check(memcmp(output,expected.data()+713,1367)==0&&output[1367]==0xcc,"sync exact cached bytes");
    // An unscoped caller and shared slot zero must always use native ReadFile.
    Check(NativeSync(&fs,&nativeFile,output,127)==127,"unscoped native read");
    testThreadIndex=0;Check(invoke(&fs,&nativeFile,output,127)==127,"shared slot native read");
    Check(native_directstorage::syncServed.load()==1,"unowned reads cannot consume cache");testThreadIndex=1;
    // Force a cold synchronous read on a separate file identity.
    HANDLE cold=CreateFileW(other.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(cold!=INVALID_HANDLE_VALUE,"cold sync open");NativeFileFixture coldFile{};coldFile.handle=cold;
    Check(invoke(&fs,&coldFile,output,127)==127&&coldFile.position==127,"cold scoped native fallback");
    for(unsigned i=0;i<127;++i)Check(output[i]==static_cast<unsigned char>(expected[i]^0x5a),"cold scoped bytes");
    Check(native_directstorage::syncServed.load()==1,"cold scoped read not claimed as served");CloseHandle(cold);
    seek.QuadPart=size-10;Check(SetFilePointerEx(syncFile,seek,nullptr,FILE_BEGIN)!=FALSE,"EOF seek");nativeFile.position=size-10;
    Check(invoke(&fs,&nativeFile,output,127)==10&&nativeFile.position==size,"EOF native short read");
    Check(memcmp(output,expected.data()+size-10,10)==0,"EOF exact bytes");
    Check(tailPatch.Exchange(false)&&memcmp(code,leaf,sizeof(leaf))==0,"tail restore");
    Check(!tailPatch.Exchange(false),"tail double rollback rejected");
    VirtualFree(code,0,MEM_RELEASE);CloseHandle(syncFile);
    puts("PASS R233 synchronous archive cache: executable tail dispatch, registered-worker scope, slot-zero and unscoped fallback, exact cached bytes, native and OS positions, LastError, short EOF, patch rollback");
    Check(!cache.TryRead(file,size-10,output,11),"EOF fallback");
    Check(!cache.TryRead(INVALID_HANDLE_VALUE,0,output,1),"invalid handle fallback");
    Check(!cache.TryRead(file,0,output,0),"zero fallback");
    Check(!cache.TryRead(file,0,output,BlockBytes+1),"large fallback before touching destination");
    Check(!cache.TryRead(file,BlockBytes-10,output,20),"cross block fallback");
    HANDLE mutation=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(mutation==INVALID_HANDLE_VALUE,"archive mutation blocked while cached");
    HANDLE second=CreateFileW(other.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
    Check(second!=INVALID_HANDLE_VALUE,"other file");memset(output,0xcc,sizeof(output));
    Check(!cache.TryRead(second,713,output,1367)&&output[0]==0xcc,"different identity is not a hit");
    // Exercise exact short final block, cancellation, and ownership-safe drain.
    const auto tailDeadline=GetTickCount64()+30000;
    while(!cache.TryRead(file,BlockBytes*3,output,131)){Check(GetTickCount64()<tailDeadline,"tail completion");Sleep(1);}
    Check(memcmp(output,expected.data()+BlockBytes*3,131)==0,"tail equality");
    cache.CancelPrefetch();Check(!cache.TryRead(file,713,output,1367),"cancel disables cache consumption");
    const auto closeDeadline=GetTickCount64()+30000;
    while(!cache.CloseIfIdle()){Check(GetTickCount64()<closeDeadline,"drain timeout");Sleep(1);}
    native_directstorage::cache=nullptr;CloseHandle(second);CloseHandle(file);CloseHandle(event);
    Check(DeleteFileW(path.c_str())&&DeleteFileW(other.c_str()),"cleanup");
    puts("PASS R230 DirectStorage archive cache: real file bytes, native cold fallback, cached ReadFile event/ABI, file identity, mutation exclusion, EOF/bounds, tail block, cancellation and drain");
    return 0;
}
