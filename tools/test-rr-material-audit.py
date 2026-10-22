"""Exercise the production material capture against deterministic native/OS mocks."""
from pathlib import Path
import subprocess, tempfile, shutil
root = Path(__file__).resolve().parents[1]
source = r'''
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#define WINAPI
using DWORD=unsigned long;
constexpr unsigned ERROR_ALREADY_EXISTS=183, WT_EXECUTEDEFAULT=0;
static unsigned writes=0,copies=0;static bool worker=false,queueOkay=true,writeOkay=true,mutate=false;
static std::vector<std::pair<DWORD(*)(void*),void*>> jobs;
static DWORD GetEnvironmentVariableW(const wchar_t*,wchar_t* out,DWORD){out[0]=L'X';out[1]=0;return 1;}
static bool CreateDirectoryW(const wchar_t*,void*){assert(worker);return true;}
static DWORD GetLastError(){return 0;}
static DWORD GetCurrentProcessId(){return 7;}
static bool QueueUserWorkItem(DWORD(*fn)(void*),void* p,unsigned){if(!queueOkay)return false;jobs.emplace_back(fn,p);return true;}
template<class... T>static void Log(const char*,T...){assert(worker);}
namespace RRGuideExportDetail {struct File {
 explicit File(const std::wstring&){assert(worker);}
 bool Write(const void*,size_t){assert(worker);++writes;return writeOkay;}
 bool Finish(){assert(worker);return writeOkay;}
};}
namespace RRAlbedoShader {
constexpr size_t kMaximumBlobBytes=256*1024;
struct NativeDescriptor {uintptr_t shaderBase;uint32_t stage;int32_t identifier;uint64_t size;};
static bool ReadDescriptor(uintptr_t record,size_t offset,NativeDescriptor* out){
 if(!record)return false;
 *out={record+offset,uint32_t(offset==0x10?0:4),int32_t(record+offset),32};
 if(mutate&&copies)++out->identifier;
 return true;
}
static bool CopyResident(const NativeDescriptor* d,uint8_t* bytes,size_t size){
 assert(!worker);++copies;memset(bytes,int(d->identifier&255),size);return true;
}
}
#include "rr_hair_shader_audit.h"
static void drain(){worker=true;for(auto job:jobs)job.first(job.second);jobs.clear();worker=false;}
static void request(unsigned frame){rrHairShaderAuditRequested=true;RRMaterialAudit::BeginFrame(frame);}
int main(){
 using namespace RRMaterialAudit;
 Capture(1,1,0,100,200);assert(copies==0&&jobs.empty()); // Explicit request required.
 request(10);Capture(10,1,0,100,200);assert(copies==4&&jobs.size()==1&&writes==0);
 Capture(10,1,1,300,400);assert(copies==4); // One pair per frame.
 BeginFrame(11);Capture(11,1,0,100,200);assert(copies==4); // Repeated material is deduplicated.
 for(unsigned n=0;n<3;++n){BeginFrame(12+n);Capture(12+n,1,n+1,1000+n*100,2000+n*100);}
 assert(jobs.size()==4&&copies==16);
 auto* first=static_cast<Slot*>(jobs[0].second);const auto originalByte=first->bytes[0][0];
 BeginFrame(16);Capture(16,1,7,5000,6000);assert(copies==16); // No overwrite or wait with busy slots.
 assert(first->bytes[0][0]==originalByte);
 drain();assert(writes==20);for(auto& slot:slots)assert(!slot.busy);
 BeginFrame(17);copies=0;mutate=true;Capture(17,1,8,5000,6000);
 assert(copies==1&&jobs.empty());for(auto& slot:slots)assert(!slot.busy);mutate=false;
 BeginFrame(18);queueOkay=false;Capture(18,1,8,5000,6000);
 assert(jobs.empty());for(auto& slot:slots)assert(!slot.busy);queueOkay=true;
 BeginFrame(19);Capture(19,1,8,5000,6000);assert(jobs.size()==1);writeOkay=false;
 unsigned oldWrites=writes;drain();assert(writes==oldWrites+4); // No complete marker on export failure.
 writeOkay=true;request(100);
 for(unsigned n=0;n<30;++n){BeginFrame(101+n);Capture(101+n,1,n,10000+n*100,20000+n*100);drain();}
 assert(count==StandardLimit&&standardCount==StandardLimit);
 for(unsigned n=0;n<30;++n){BeginFrame(131+n);Capture(131+n,2,n,30000+n*100,40000+n*100);drain();}
 assert(count==PairLimit);assert(!Wants(1)&&!Wants(2));
 request(200);for(unsigned n=0;n<FrameLimit;++n)BeginFrame(201+n);
 oldWrites=copies;Capture(400,1,0,80000,90000);assert(copies==oldWrites&&!Wants(1));
 // Invalid/null descriptors never publish a job or consume a worker slot.
 request(500);Capture(500,1,0,0,10);assert(jobs.empty());
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'test.cpp';src.write_text(source);out=Path(d)/'test'
    compiler=shutil.which('g++') or shutil.which('clang++') or shutil.which('cl')
    assert compiler,'C++ compiler required'
    if Path(compiler).stem.lower()=='cl':
        out=out.with_suffix('.exe');args=[compiler,'/nologo','/std:c++17','/EHsc',str(src),'/I'+str(root/'src'),'/Fe:'+str(out),'/Fo:'+str(Path(d)/'test.obj')]
    else:args=[compiler,'-std=c++17','-Wall','-Wextra',str(src),'-I'+str(root/'src'),'-o',str(out)]
    subprocess.run(args,check=True);subprocess.run([str(out)],check=True)
print('PASS: production material capture budget, deduplication, ownership, failure and expiry tests')
