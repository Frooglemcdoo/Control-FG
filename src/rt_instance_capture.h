#pragma once
#include <array>
#include <mutex>
// R393: bounded copies at the existing native CPU instance upload, before the
// original memcpy. Never consumes a native pointer after this callback returns.
// This is source-data evidence, NOT GPU lifetime/retirement proof.
namespace RTInstanceCapture {
constexpr unsigned kSamples=48;
constexpr size_t kMaxBytes=2u*1024u*1024u;
using Lock=void(*)(void*,UINT64,UINT64,unsigned,void**);
using Copy=void*(*)(void*,const void*,size_t);
inline Lock originalLock=nullptr;
inline Copy originalCopy=nullptr;
inline RRIndirectCallPatch lockPatch;
inline RRAlbedoCallPatch copyPatch{};
struct Locked {void* mapped=nullptr;UINT64 address=0,resource=0,wrapper=0,bytes=0;};
inline thread_local Locked locked;
struct Sample {UINT64 tick=0,address=0,resource=0,wrapper=0;DWORD thread=0;std::vector<unsigned char> bytes;};
inline std::array<Sample,kSamples> samples;
inline std::mutex mutex;
inline std::atomic<bool> ready{false},done{false};
inline UINT64 started=0,last=0;
inline unsigned captured=0;
inline std::atomic<UINT64> uploads{0},rejected{0},busy{0};
inline std::wstring output;
inline bool ReadLock(void* wrapper,UINT64 offset,UINT64 bytes,void** mapped,Locked& out) noexcept {
 out={};__try {
  if(!wrapper||!mapped||!*mapped||offset||!bytes||bytes>kMaxBytes||bytes%64)return false;
  auto* p=static_cast<unsigned char*>(wrapper);
  const auto capacity=*reinterpret_cast<UINT64*>(p+0x30);
  out={*mapped,*reinterpret_cast<UINT64*>(p+0x48),*reinterpret_cast<UINT64*>(p),reinterpret_cast<UINT64>(wrapper),bytes};
  return capacity>=bytes&&out.address&&out.resource;
 }__except(EXCEPTION_EXECUTE_HANDLER){out={};return false;}
}
inline bool ReadBytes(void* to,const void* from,size_t bytes) noexcept {
 __try {if(!to||!from)return false;std::memcpy(to,from,bytes);return true;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool Write(HANDLE file,const void* data,size_t bytes) noexcept {
 if(bytes>MAXDWORD)return false;DWORD wrote=0;
 return WriteFile(file,data,static_cast<DWORD>(bytes),&wrote,nullptr)&&wrote==bytes;
}
inline DWORD WINAPI Save(void*) noexcept {
 // Only owned vectors are used on this thread. Native pointers below are
 // opaque diagnostic numbers, never dereferenced or retained as resources.
 try {
  std::lock_guard<std::mutex> guard(mutex);
  HANDLE file=CreateFileW(output.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file==INVALID_HANDLE_VALUE){Log("RT_INSTANCE_R393 save_failed win32=%lu",GetLastError());return 1;}
  const UINT64 header[]={0x3130333933545243ull,1,captured,uploads.load(),rejected.load(),busy.load()};
  bool ok=Write(file,header,sizeof(header));
  for(unsigned i=0;i<captured&&ok;++i){const auto& s=samples[i];
   const UINT64 record[]={s.tick,s.address,s.resource,s.wrapper,s.thread,static_cast<UINT64>(s.bytes.size())};
   ok=Write(file,record,sizeof(record))&&Write(file,s.bytes.data(),s.bytes.size());
  }
  if(ok)ok=FlushFileBuffers(file)!=FALSE;CloseHandle(file);
  Log("RT_INSTANCE_R393 saved=%u samples=%u uploads=%llu rejected=%llu busy=%llu source=pre_native_memcpy gpu_snapshot=0 native_retirement=0 file=%ls",unsigned(ok),captured,uploads.load(),rejected.load(),busy.load(),output.c_str());
  if(ok)MessageBeep(MB_OK);
  for(auto& s:samples){std::vector<unsigned char>().swap(s.bytes);}
  return ok?0:1;
 }catch(...){Log("RT_INSTANCE_R393 save_failed exception=1");return 1;}
}
inline void LockHook(void* wrapper,UINT64 offset,UINT64 bytes,unsigned mode,void** pointer){
 originalLock(wrapper,offset,bytes,mode,pointer);const DWORD error=GetLastError();locked={};
 if(ready.load(std::memory_order_relaxed)&&!done.load(std::memory_order_relaxed))ReadLock(wrapper,offset,bytes,pointer,locked);
 SetLastError(error);
}
inline void Capture(void* to,const void* from,size_t bytes) noexcept {
 if(!ready.load(std::memory_order_relaxed)||done.load(std::memory_order_relaxed)){locked={};return;}
 const auto current=locked;locked={};++uploads;
 if(!current.address||to!=current.mapped||bytes!=current.bytes){++rejected;return;}
 try {
  std::unique_lock<std::mutex> guard(mutex,std::try_to_lock);if(!guard.owns_lock()){++busy;return;}
  if(done.load(std::memory_order_acquire)||captured>=kSamples)return;
  const auto now=GetTickCount64();if(!started){started=now;Log("RT_INSTANCE_R393 armed=1 warmup_ms=5000 samples=48 interval_ms=500 no_settings_changed=1");}
  if(now-started<5000||now-last<500)return;
  auto& sample=samples[captured];sample.bytes.resize(bytes);
  if(!ReadBytes(sample.bytes.data(),from,bytes)){sample.bytes.clear();++rejected;return;}
  sample.tick=now;sample.address=current.address;sample.resource=current.resource;sample.wrapper=current.wrapper;sample.thread=GetCurrentThreadId();
  last=now;++captured;
  if(captured==kSamples){
   done.store(true,std::memory_order_release);
   const HANDLE thread=CreateThread(nullptr,0,&Save,nullptr,0,nullptr);
   if(thread)CloseHandle(thread);else Log("RT_INSTANCE_R393 save_thread_failed win32=%lu",GetLastError());
  }
 }catch(...){done=true;Log("RT_INSTANCE_R393 capture_failed exception=1");}
}
inline void* CopyHook(void* to,const void* from,size_t bytes){
 const DWORD error=GetLastError();Capture(to,from,bytes);SetLastError(error);
 return originalCopy(to,from,bytes); // Exactly one native copy, every time.
}
inline bool Install(HMODULE renderer) noexcept {
 try {
  if(!renderer||renderer!=verifiedRenderer||!HashMatches(renderer,kRendererHash))return false;
  wchar_t local[32768]{};const auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,_countof(local));
  if(!n||n>=_countof(local))return false;
  const auto directory=std::wstring(local)+L"\\ControlFGProbe";
  if(!CreateDirectoryW(directory.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;
  output=directory+L"\\RT-Instances-R393-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".bin";
  auto* b=reinterpret_cast<unsigned char*>(renderer);
  originalLock=reinterpret_cast<Lock>(*reinterpret_cast<void**>(b+0x5dfe70));originalCopy=reinterpret_cast<Copy>(b+0x588e0e);
  if(!lockPatch.Prepare(b+0xa8972,reinterpret_cast<void**>(b+0x5dfe70),reinterpret_cast<void*>(originalLock),reinterpret_cast<void*>(&LockHook))||
     !RRAlbedoPrepareCallPatch(b+0xa8987,reinterpret_cast<void*>(originalCopy),reinterpret_cast<void*>(&CopyHook),&copyPatch))return false;
  // Reserve outside rendering callbacks; actual committed memory grows only
  // with captured data. Maximum retained snapshot bytes: 96 MiB.
  for(auto& sample:samples)sample.bytes.reserve(kMaxBytes);
  if(!lockPatch.Exchange(true)||!lockPatch.healthy||!RRAlbedoExchangeCall(&copyPatch,true)||!copyPatch.writeHealthy){
   if(lockPatch.changed)lockPatch.Exchange(false);
   if(copyPatch.site&&!memcmp(copyPatch.site,copyPatch.replacement,5))RRAlbedoExchangeCall(&copyPatch,false);
   return false;
  }
  ready=true;Log("RT_INSTANCE_R393 installed=1 native_upload_preserved=1 replacement=0 source_boundary=0xA8987");return true;
 }catch(...){Log("RT_INSTANCE_R393 install_failed exception=1");return false;}
}
}
