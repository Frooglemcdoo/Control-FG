from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={}
with tempfile.TemporaryDirectory() as directory:
 t=Path(directory)
 adapter=(root/'src/rr_reflection_d3d12.h').read_text()
 start=adapter.index(' bool QueryBudget(');end=adapter.index(' static D3D12_RESOURCE_DESC Descriptor(',start)
 (t/'budget-query.inc').write_text(adapter[start:end])
 hooks=(root/'src/rr_reflection_hooks.h').read_text()
 start=hooks.index('static std::uintptr_t RRReflectionHookBind(')
 end=hooks.index('static bool RRReflectionInstall(',start)
 prefix='''#include <cstdint>
#include <cassert>
#include "rr_user_control.h"
#include <stdexcept>
#include <iostream>
using DWORD=unsigned long;
static DWORD error=13;static unsigned nativeCalls=0,observerCalls=0;static void* expected=nullptr;static bool fail=false;
static DWORD GetLastError(){return error;} static void SetLastError(DWORD e){error=e;}
static std::uintptr_t Original(void* owner){++nativeCalls;assert(owner==expected);assert(error==13);if(fail)throw std::runtime_error("native");error=27;return UINT64_C(0xabcdef1287654321);}
static auto rrReflectionOriginalBind=&Original;
static bool rrReflectionCaptureEnabled=true;
static void RRReflectionAfterBind(void* owner){assert(owner==expected);++observerCalls;error=999;}
'''
 suffix='''
int main(){control_rr::RRUserSetMode(control_rr::RRUserMode::Full);expected=reinterpret_cast<void*>(0x1234);assert(RRReflectionHookBind(expected)==UINT64_C(0xabcdef1287654321));assert(nativeCalls==1&&observerCalls==1&&error==27);
error=13;fail=true;try{RRReflectionHookBind(expected);assert(false);}catch(const std::runtime_error&){}assert(nativeCalls==2&&observerCalls==1);
error=13;fail=false;rrReflectionCaptureEnabled=false;assert(RRReflectionHookBind(expected)==UINT64_C(0xabcdef1287654321));assert(nativeCalls==3&&observerCalls==1&&error==27);
std::cout<<"PASS actual hook preserves owner, native result and LastError, calls native exactly once, propagates native exceptions\\n";}
'''
 (t/'hook.cpp').write_text(prefix+hooks[start:end]+suffix)
 runtime=(root/'src/rr_reflection_runtime.h').read_text()
 begin=runtime.index('__declspec(noinline) static RRReflectionOwner* RRReflectionCreateOwner()')
 finish=runtime.index('// Called synchronously',begin)
 factory=runtime[begin:finish].replace('__declspec(noinline)','__attribute__((noinline))')
 assert '__try' not in factory and 'catch(...)' in factory
 bind=runtime[runtime.index('static void RRReflectionAfterBind('):runtime.index('static void RRReflectionAfterPresent(')]
 assert 'new(' not in bind and 'RRReflectionCreateOwner()' in bind and '__try' in bind
 factory_prefix='''#include <new>
#include <cstdlib>
#include <cassert>
#include <iostream>
static bool failAllocation=false,failConstructor=false;
static unsigned constructions=0,placementCleanup=0,normalCleanup=0;
struct RRReflectionOwner {
 static void* operator new(std::size_t size,const std::nothrow_t&) noexcept {return failAllocation?nullptr:std::malloc(size);}
 static void operator delete(void* memory,const std::nothrow_t&) noexcept {++placementCleanup;std::free(memory);}
 static void operator delete(void* memory) noexcept {++normalCleanup;std::free(memory);}
 RRReflectionOwner(){++constructions;if(failConstructor)throw 17;}
};
'''
 factory_suffix='''
int main(){failAllocation=true;assert(!RRReflectionCreateOwner()&&constructions==0);
failAllocation=false;failConstructor=true;assert(!RRReflectionCreateOwner()&&constructions==1&&placementCleanup==1);
failConstructor=false;auto* owner=RRReflectionCreateOwner();assert(owner&&constructions==2);delete owner;assert(normalCleanup==1);
std::cout<<"PASS actual isolated factory: allocation failure, constructor exception cleanup, successful ownership\\n";}
'''
 (t/'factory.cpp').write_text(factory_prefix+factory+factory_suffix)
 build=(root/'Build.cmd').read_text().splitlines()
 for test in ('test_backend.cpp','windows-max.cpp','present-test.cpp'):
  assert sum(line.startswith('cl ') and ('\\'+test+' ') in line for line in build)==1,test
 for test in ('reflection-backend','reflection-windows-max','reflection-present'):
  assert build.count('build\\'+test+'.exe')==1,test
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
  for name,source in [('backend',here/'test_backend.cpp'),('present',here/'present-test.cpp'),('windows_max',here/'windows-max.cpp'),('hook',t/'hook.cpp'),('factory',t/'factory.cpp'),('budget_query',here/'budget-query-test.cpp')]:
   exe=t/(mode+'-'+name)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I'+str(root/'src'),'-I'+str(t),str(source),'-o',str(exe)],check=True)
   results[mode+'-'+name]=subprocess.check_output([str(exe)],text=True,env=env).strip()
files=[p for p in (root/'src').glob('rr_reflection*.h')]+[here/'run.py',here/'test_backend.cpp',here/'present-test.cpp',here/'windows_compile.cpp',here/'windows-max.cpp']
files+=[here/'budget-query-test.cpp']
files+=[root/'src/rr_memory_budget.h',root/'src/rr_dimensions.h']
summary={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)},'windows_compiled':False,'gpu_executed':False,'rr_enabled':False}
(here/'results.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(results,indent=2))
