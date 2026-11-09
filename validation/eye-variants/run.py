from pathlib import Path
import hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent;results={}
with tempfile.TemporaryDirectory() as temp:
 t=Path(temp)
 # Compile the actual audit formatting block, independently parse its JSON.
 source=(root/'src/rr_albedo_capture.h').read_text();start=source.index('            std::ostringstream audit;');end=source.index('        std::array<unsigned int,7> admitted',start)
 block=source[start:end]
 prefix='''#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include "rr_albedo_prepare.h"
#include "rr_vertex_evidence.h"
struct Shaders {RRVertexEvidence::Store store;auto& VertexEvidence(){return store;}};
struct Capture {control_rr_albedo::PreparedReplay replay;std::array<unsigned,5> shaderRejectCounts{};std::vector<std::pair<unsigned,std::string>> shaderRejectExamples;Shaders shaders;std::string rejectionAuditJson;};
int main(){Capture c;auto* capture=&c;unsigned long long frame=77;c.replay.coverage={3,2,1,false};c.replay.familyKinds={1,6};c.replay.rejectionAudit.total=1;c.replay.rejectionAudit.counts[static_cast<size_t>(control_rr_albedo::RejectReason::EyeVariantNotUnique)]=1;
control_rr_albedo::RejectionExample e;e.reason=control_rr_albedo::RejectReason::EyeVariantNotUnique;e.eye.tableValid=true;e.eye.matchCount=2;e.eye.keys={1,2,3,4};e.eye.matches={true,false,true,false};c.replay.rejectionAudit.examples.push_back(e);
{
'''
 (t/'format.cpp').write_text(prefix+block+'std::cout<<c.rejectionAuditJson;}')
 for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
  for name,src,extra,args in [
   ('selector',here/'test.cpp',[],[]),
   ('integration',here/'integration.cpp',['-D_WIN32','-I',str(root/'validation/vertex-evidence/shim')],[str(root/('validation/foliage-variant/evidence/pair-000-'+n+'.dxbc')) for n in ('original_vs','replacement_vs','replacement_ps')]),
   ('format',t/'format.cpp',['-I',str(root/'src')],[])]:
   exe=t/(mode+'-'+name)
   subprocess.run(['g++','-std=c++17',*flags,*extra,str(src),'-o',str(exe)],check=True)
   result=subprocess.check_output([str(exe),*args],env=env,text=True)
   if name=='format':
    d=json.loads(result);assert d['accepted_families']['eye']==1 and sum(d['accepted_families'].values())==d['accepted_batches'] and d['rejection_total_reconciled']
    assert d['examples'][0]['eye_variants']=={'table_valid':True,'match_count':2,'keys':[1,2,3,4],'matches':[True,False,True,False]}
   results[mode+'-'+name]='PASS'
files=['src/rr_eye_variants.h','src/rr_albedo_prepare.h','src/rr_albedo_capture.h','src/rr_diffuse_replay.h','src/rr_albedo_shader.h','tools/diagnostics/Collect-ControlFG-Logs.ps1','validation/eye-variants/test.cpp','validation/eye-variants/integration.cpp','validation/eye-variants/run.py']
(here/'results.json').write_text(json.dumps({'status':'LOCAL_PASS_WINDOWS_REQUIRED','results':results,'sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in files},'limits':'Captured foliage programs exercise the actual validator as fixtures; no eye bytecode or Windows gameplay execution is claimed.'},indent=2)+'\n');print(json.dumps(results,indent=2))
