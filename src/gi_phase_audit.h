#pragma once
#include <windows.h>
#include <d3d12.h>
#include <atomic>
#include <array>
#include <cstdio>
#include <cstdint>
// Opt-in, bounded CPU metadata only. No GPU commands, waits, resources or readback.
namespace gi_phase_audit {
using U64=unsigned long long;
using FrameReader=U64(*)() noexcept;
inline std::atomic<FrameReader> frameReader{nullptr};
inline constexpr unsigned capacity=8192;
struct Event {std::atomic<bool> ready{false};unsigned kind=0;std::array<U64,12> data{};};
inline Event events[capacity];
inline std::atomic<unsigned> next{0},dropped{0};
inline std::atomic<U64> firstFrame{0},startedAt{0};
inline std::atomic<bool> stopped{false};
inline const GUID markerKey={0xa574c93e,0x3d21,0x46df,{0x95,0x42,0xb7,0x69,0x3e,0x2a,0x04,0x25}};
struct Marker {unsigned count=0,overflow=0;U64 ids[16]{};};
inline bool Enabled() noexcept {
 static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_PHASE_AUDIT",v,4)==1&&v[0]=='1';}();return enabled;
}
inline U64 Frame() noexcept {const auto read=frameReader.load();return read?read():0;}
inline U64 Emit(unsigned kind,std::array<U64,12> data) noexcept {
 const unsigned slot=next.fetch_add(1,std::memory_order_relaxed);
 if(slot>=capacity){++dropped;stopped=true;return 0;}
 auto& e=events[slot];e.kind=kind;e.data=data;e.ready.store(true,std::memory_order_release);return U64(slot)+1;
}
inline bool Window() noexcept {
 if(!Enabled()||stopped.load(std::memory_order_relaxed)||!firstFrame.load())return false;
 // A wall-clock cap also handles pauses, a level reload or an unreadable frame.
 if(GetTickCount64()-startedAt.load()>3000){stopped=true;return false;}return true;
}
inline bool PhaseWindow(U64 frame) noexcept {
 if(!Enabled()||stopped.load()||!frame)return false;
 U64 tick=0;startedAt.compare_exchange_strong(tick,GetTickCount64());
 U64 empty=0;firstFrame.compare_exchange_strong(empty,frame);
 const U64 first=firstFrame.load();
 if(frame<first||frame-first>=24)return false;
 return Window();
}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled())return;
 Marker empty{};list->SetPrivateData(markerKey,sizeof(empty),&empty);
}
inline U64 Phase(ID3D12GraphicsCommandList* list,unsigned kind,U64 frame,U64 generation,
 unsigned x,unsigned y,unsigned z,void* root,void* pso,void* heap0,void* heap1,bool complete) noexcept {
 if(!PhaseWindow(frame))return 0;
 const U64 id=Emit(kind,{frame,reinterpret_cast<U64>(list),generation,x,y,z,
  reinterpret_cast<U64>(root),reinterpret_cast<U64>(pso),reinterpret_cast<U64>(heap0),reinterpret_cast<U64>(heap1),U64(complete),GetCurrentThreadId()});
 Marker m{};UINT bytes=sizeof(m);
 if(FAILED(list->GetPrivateData(markerKey,&bytes,&m))||bytes!=sizeof(m))m={};
 if(m.count<16)m.ids[m.count++]=id;else m.overflow=1;
 if(FAILED(list->SetPrivateData(markerKey,sizeof(m),&m)))Emit(9,{id,1});
 return id;
}
inline U64 Submit(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Window()||!lists)return 0;
 // Each marker was reset at successful command-list Reset, not ClearState.
 // IDs refer to recording events, not to recyclable COM pointer identity alone.
 const U64 call=Emit(3,{Frame(),reinterpret_cast<U64>(queue),count,GetCurrentThreadId(),unsigned(queue->GetDesc().Type)});
 for(UINT i=0;i<count;++i){if(!lists[i])continue;Marker m{};UINT bytes=sizeof(m);
  if(FAILED(lists[i]->GetPrivateData(markerKey,&bytes,&m))||bytes!=sizeof(m)||!m.count)continue;
  if(m.count>16){Emit(9,{call,2});continue;}
  for(unsigned j=0;j<m.count;++j)Emit(4,{call,i,reinterpret_cast<U64>(lists[i]),m.ids[j],j,m.overflow});
 }
 return call;
}
inline void Fence(ID3D12CommandQueue* queue,ID3D12Fence* fence,U64 value,bool signal,HRESULT hr) noexcept {
 if(Window())Emit(5,{Frame(),reinterpret_cast<U64>(queue),reinterpret_cast<U64>(fence),value,U64(signal),static_cast<unsigned long>(hr),GetCurrentThreadId()});
}
// Called only by the existing UI timer. Render/worker hooks never do file I/O.
inline void Poll() noexcept {
 if(!Enabled())return;
 static HANDLE file=INVALID_HANDLE_VALUE;static unsigned written=0;static U64 lastFlush=0;
 const auto now=GetTickCount64();if(now-lastFlush<500)return;lastFlush=now;
 if(firstFrame.load())Window();
 if(file==INVALID_HANDLE_VALUE){wchar_t path[MAX_PATH]{};
  const DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",path,MAX_PATH);
  if(!size||size>=MAX_PATH-80)return;
  wcscat_s(path,L"\\ControlFGProbe");CreateDirectoryW(path,nullptr);
  wchar_t name[64]{};swprintf_s(name,L"\\GI-Phase-R425-%lu.jsonl",GetCurrentProcessId());wcscat_s(path,name);
  file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file==INVALID_HANDLE_VALUE)return;
 }
 const unsigned reserved=next.load();const unsigned end=reserved<capacity?reserved:capacity;
 static unsigned previousWritten=~0u,previousDropped=~0u;static bool previousStopped=false;
 if(written==end&&previousWritten==written&&previousDropped==dropped.load()&&previousStopped==stopped.load())return;
 while(written<end){const auto& e=events[written];if(!e.ready.load(std::memory_order_acquire))break;
  char line[512]{};const int length=snprintf(line,sizeof(line),"{\"seq\":%u,\"kind\":%u,\"data\":[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu]}\n",written+1,e.kind,
   e.data[0],e.data[1],e.data[2],e.data[3],e.data[4],e.data[5],e.data[6],e.data[7],e.data[8],e.data[9],e.data[10],e.data[11]);
  DWORD bytes=0;if(length<=0||length>=int(sizeof(line))||!WriteFile(file,line,DWORD(length),&bytes,nullptr)||bytes!=DWORD(length))return;
  ++written;
 }
 char status[160]{};const int n=snprintf(status,sizeof(status),"{\"status\":true,\"first_frame\":%llu,\"stopped\":%u,\"reserved\":%u,\"written\":%u,\"dropped\":%u}\n",firstFrame.load(),unsigned(stopped.load()),next.load(),written,dropped.load());
 DWORD bytes=0;WriteFile(file,status,DWORD(n),&bytes,nullptr);FlushFileBuffers(file);
 previousWritten=written;previousDropped=dropped.load();previousStopped=stopped.load();
}
}
