from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
here=Path(__file__).resolve().parent
outputs={}
with tempfile.TemporaryDirectory(prefix='control-part1-') as temp:
 for name in ('metadata-test','diagnostic-test','resource-test','copy-policy-test','copy-runtime-test'):
  for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
   binary=Path(temp)/(name+'-'+mode)
   subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(here/(name+'.cpp')),'-o',str(binary)],check=True)
   p=subprocess.run([str(binary)],check=True,text=True,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
   outputs[name+'-'+mode]=p.stdout.strip()
 # Compile the exact runtime observation adapter with host stubs. This checks
 # its frame gates, log format types and last-error preservation, not Windows SEH.
 capture=(root/'src/rr_albedo_capture.h').read_text()
 adapter=capture.split('static control_rr_part1::Sample RRPart1Observe',1)[1].split('static bool RRAlbedoGetShader',1)[0]
 harness='''
#include "rr_part1_diagnostic.h"
#include <cassert>
#include <cstring>
#include <map>
#include <cstdarg>
using DWORD=unsigned int;
static DWORD lastError=123;
struct RRAlbedoLastError { DWORD value=lastError; ~RRAlbedoLastError(){lastError=value;} };
static unsigned frameReads=0, memoryReads=0;
static bool rejectBefore=false, rejectAfter=false;
static bool ReadEngineFrameSafe(unsigned long long* out,DWORD*) {
 ++frameReads;lastError=99;
 *out=((frameReads==1 && rejectBefore)||(frameReads==2 && rejectAfter))?11:10;return true;
}
static bool RRAlbedoRead(std::uintptr_t a,void* out,std::size_t size) {
 ++memoryReads;
 static const std::map<std::uintptr_t,std::uint64_t> memory{
 {0x1060,0x2000},{0x20f0,0x3000},{0x3028,8},{0x3030,1024},
 {0x3048,0xabc000},{0x3040,0x4000},{0x4014,2}};
 auto i=memory.find(a);if(i==memory.end())return false;
 std::memcpy(out,&i->second,size);return true;
}
static void Log(const char*,...) __attribute__((format(printf,1,2)));
static void Log(const char*,...) {}
static control_rr_part1::Sample RRPart1Observe'''+adapter+'''
int main(){
 auto s=RRPart1Observe(0x1000,10,"prepare");assert(s.frameMatched && s.repeatedReadMatched && lastError==123);
 frameReads=memoryReads=0;rejectBefore=true;s=RRPart1Observe(0x1000,10,"prepare");
 assert(!s.frameMatched && memoryReads==0 && lastError==123);
 frameReads=memoryReads=0;rejectBefore=false;rejectAfter=true;s=RRPart1Observe(0x1000,10,"join");
 assert(!s.frameMatched && memoryReads>0 && lastError==123);
}
'''
 resourceSource=(root/'src/rr_part1_resource.h').read_text()
 resourceDecl=resourceSource.split('struct RRPart1ResourceObservation',1)[1].split('static RRPart1ResourceObservation RRPart1ReadResource',1)[0]
 harness=harness.replace('static control_rr_part1::Sample RRPart1Observe',
  'struct RRPart1ResourceObservation'+resourceDecl+'\nstatic RRPart1ResourceObservation RRPart1ReadResource(const control_rr_part1::Sample&){return {};}\nstatic control_rr_part1::Sample RRPart1Observe')
 harnessPath=Path(temp)/'adapter.cpp';harnessPath.write_text(harness)
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
  binary=Path(temp)/('adapter-'+mode)
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I',str(root/'src'),str(harnessPath),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  outputs['runtime-adapter-'+mode]='PASS: frame gating, log format, last-error preservation (host stubs)'
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
  binary=Path(temp)/('export-'+mode);out=Path(temp)/('export-output-'+mode);out.mkdir()
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I',str(here/'shim'),str(here/'export-test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,text=True,capture_output=True,env={**os.environ,'CONTROL_EXPORT_TEST_ROOT':str(out),'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  directories=sorted((out/'ControlFGProbe').iterdir())
  assert len(directories)==3
  completed=[d for d in directories if (d/'metadata.json').exists()]
  assert len(completed)==2
  with_table=[d for d in completed if (d/'material-part1.bin').exists()]
  assert len(with_table)==1
  d=with_table[0];m=json.loads((d/'metadata.json').read_text())
  assert (d/'material-part1.bin').read_bytes()==bytes((i*13)%256 for i in range(16))
  assert m['part1_bytes']==16 and m['part1_stride']==8 and m['part1_records']==2 and m['part1_source_frame']==22 and not m['part1_rr_validated']
  h=14695981039346656037
  for b in (d/'material-part1.bin').read_bytes():h=((h^b)*1099511628211)&((1<<64)-1)
  assert m['part1_fnv1a64']==f'{h:016x}'
  without=next(d for d in completed if d not in with_table);m=json.loads((without/'metadata.json').read_text())
  assert m['part1_file'] is None and m['part1_bytes']==0
  outputs['export-'+mode]='PASS: exact exporter, raw bytes, JSON, optional omission, invalid sizes, failed-write commit suppression (POSIX shim)'
 tested=[root/'src/rr_part1_metadata.h',root/'src/rr_part1_diagnostic.h',root/'src/rr_part1_resource.h',root/'src/rr_part1_readback.h',root/'src/rr_part1_copy_policy.h',root/'src/rr_albedo_capture.h',root/'src/rr_guide_render.h',root/'src/rr_guide_export.h',here/'metadata-test.cpp',here/'diagnostic-test.cpp',here/'resource-test.cpp',here/'copy-policy-test.cpp',here/'copy-runtime-test.cpp',here/'export-test.cpp',here/'shim/windows.h',Path(__file__).resolve()]
report={'status':'PASS','results':outputs,'tested_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in tested},
 'windows_build':'NOT_RUN','gpu_resource_lifetime':'NOT_PROVEN','rr_evaluation_enabled':False}
(here/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: Part1 metadata and diagnostic tests, normal and ASan/UBSan')
