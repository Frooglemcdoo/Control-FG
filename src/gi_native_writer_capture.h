#pragma once
#include "gi_native_job_runtime.h"
#include <memory>
#include <string>
// Diagnostic-only whole-table capture. Copies CPU inputs before scheduling and
// writes them on a worker; no GPU readback or filtering changes.
namespace GINativeWriterCapture {
struct Export {GINativeJobRoute::Ticket ticket;gi_reuse::NativeGIWriterInputs inputs;unsigned ordinal=0;};
static std::atomic<unsigned> accepted{0},finished{0},failed{0},attempts{0};
static std::atomic<ULONGLONG> nextAt{0};
static bool Write(const wchar_t* path,const void* data,DWORD bytes) noexcept {
 const HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;const bool ok=WriteFile(f,data,bytes,&written,nullptr)&&written==bytes;CloseHandle(f);return ok;
}
static DWORD WINAPI Save(void* raw) {
 std::unique_ptr<Export> e(static_cast<Export*>(raw));bool ok=false;
 try {
 wchar_t root[32768]{};const DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",root,_countof(root));
 if(size&&size<_countof(root)-256){
  std::wstring directory=root;directory+=L"\\Control-FG";CreateDirectoryW(directory.c_str(),nullptr);
  directory+=L"\\Diagnostics";CreateDirectoryW(directory.c_str(),nullptr);
  wchar_t name[160]{};swprintf_s(name,L"\\GI-Writer-R305-%lu-%llu",GetCurrentProcessId(),e->ticket.serial);
  const auto base=directory+name;const auto& t=e->ticket;const auto& inputs=e->inputs;
  const auto bindingBytes=static_cast<DWORD>(inputs.bindings.size()*sizeof(PTNativeBinding::Binding));
  const auto tableBytes=static_cast<DWORD>(inputs.table.bytes.size());
  ok=Write((base+L"-bindings.bin").c_str(),inputs.bindings.data(),bindingBytes)&&
     Write((base+L"-table.bin").c_str(),inputs.table.bytes.data(),tableBytes);
  if(ok){
   char json[2048]{};const int n=sprintf_s(json,
    "{\"schema\":\"gi-native-writer-r305\",\"pid\":%lu,\"ordinal\":%u,\"serial\":%llu,\"engine_frame\":%llu,\"list\":\"%llx\",\"pipeline\":\"%llx\",\"counter\":%llu,\"width\":%u,\"height\":%u,\"count\":%d,\"binding_stride\":64,\"record_stride\":96,\"rows_per_geometry\":2,\"active_rows\":\"even\",\"hit_address\":\"%llx\",\"hit_bytes\":%llu,\"binding_bytes\":%lu,\"table_bytes\":%lu,\"cpu_inputs_only\":true,\"gpu_completion_proven\":false,\"allocation_ownership_proven\":false}\n",
    GetCurrentProcessId(),e->ordinal,t.serial,t.engineFrame,t.list,t.pipeline,t.counter,t.width,t.height,t.writer.count,t.hitTable,t.hitBytes,bindingBytes,tableBytes);
   ok=n>0&&Write((base+L".json").c_str(),json,static_cast<DWORD>(n));
  }
 }
 }catch(...){ok=false;}
 if(ok)++finished;else ++failed;
 Log("GI_WRITER_R305_EXPORT serial=%llu ordinal=%u complete=%u bindings=%d finished=%u failed=%u cpu_inputs_only=1 gpu_work=0",
  e->ticket.serial,e->ordinal,unsigned(ok),e->ticket.writer.count,finished.load(),failed.load());
 return 0;
}
static void Capture(GINativeJobRuntime::Phase phase,const GINativeJobRoute::Ticket& ticket) noexcept {
 if(phase!=GINativeJobRuntime::Phase::writerReady||accepted.load()>=4||attempts.load()>=16)return;
 const auto now=GetTickCount64();auto due=nextAt.load();
 if(now<due||!nextAt.compare_exchange_strong(due,now+10000))return;
 ++attempts;
 try {
  auto e=std::make_unique<Export>();e->ticket=ticket;
  if(!gi_reuse::SnapshotNativeGIWriter(ticket,e->inputs)){++failed;Log("GI_WRITER_R305_REJECT serial=%llu stage=cpu_inputs",ticket.serial);return;}
  e->ordinal=++accepted;
  const HANDLE thread=CreateThread(nullptr,0,&Save,e.get(),0,nullptr);
  if(!thread){++failed;Log("GI_WRITER_R305_REJECT serial=%llu stage=export_thread",ticket.serial);return;}
  e.release();CloseHandle(thread);
 }catch(...){++failed;Log("GI_WRITER_R305_REJECT serial=%llu stage=allocation",ticket.serial);}
}
static void Start() noexcept {
 char setting[4]{};
 if(GetEnvironmentVariableA("CONTROLTOH_GI_METADATA",setting,4)!=1||setting[0]!='1')return;
 const bool ready=GINativeJobRuntime::Register(&Capture);
 Log("GI_WRITER_R305_ARM ready=%u tables=4 interval_ms=10000 scope=whole_gi_binding_table native_outputs_unchanged=1",unsigned(ready));
}
}
