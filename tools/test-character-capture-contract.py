"""Capture must not mutate RR; exercise its actual bounded log copier on Windows."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
s=(root/'src/ui/lighting_capture_suite.h').read_text()
assert not re.search(r'\b(?:RRUserSetMode|RRUserRequest|RRSetDiagnosticBypass)\s*\(|rrUserMode\s*\.\s*store',s),'Capture must not change RR mode'
x=s[s.index('static constexpr const wchar_t* names[]={L"r360-legacy"'):]
names=re.findall(r'L"([^"]+)"',x[:x.index(';')])
phases=[int(v) for v in re.search(r'phases\[\]=\{([^}]+)\}',x)[1].split(',')]
assert len(names)==len(phases)==4 and phases[0]==10 and phases[-1]==10
assert phases==[10,10,4,10] and 'stable-rr-off' not in names
assert 'control_rr_diffuse_source::legacyBypass.store(false,std::memory_order_release)' in s[s.index('inline void Finish('):s.index('inline bool Start(')]
assert 'source_a_calls=%llu source_b_calls=%llu native_filter_calls=%llu bypass_calls=%llu' in s
assert 'rr_runtime_stopped_restart_required'  in s and 'CopyLog(path,65536)' in s
start=s.index('inline bool CopyLog(');end=s.index('inline bool PackageCharacter(',start)
copier=s[start:end]
harness=r'''#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
struct { const wchar_t* Path(){return L"build/capture-log-test-input.tmp";} } report;
LARGE_INTEGER logOffset{};
void require(bool p){if(!p)throw std::runtime_error("bounded log copy contract failed");}
std::vector<char> read(const wchar_t* p){HANDLE h=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);require(h!=INVALID_HANDLE_VALUE);LARGE_INTEGER n{};require(GetFileSizeEx(h,&n));std::vector<char> b(size_t(n.QuadPart));if(b.empty()){CloseHandle(h);return b;}DWORD got=0;require(ReadFile(h,b.data(),DWORD(b.size()),&got,nullptr)&&got==b.size());CloseHandle(h);return b;}
'''+copier+r'''
int main(){try{
 const wchar_t* out=L"build/capture-log-test-output.tmp";
 std::vector<char> data(200000);for(size_t i=0;i<data.size();++i)data[i]=char(i%251);
 HANDLE h=CreateFileW(report.Path(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);require(h!=INVALID_HANDLE_VALUE);DWORD wrote=0;require(WriteFile(h,data.data(),DWORD(data.size()),&wrote,nullptr)&&wrote==data.size());CloseHandle(h);
 struct Case{long long offset;uint64_t limit;size_t begin;};
 for(auto c:{Case{100,65536,200000-65536},Case{199000,65536,199000},Case{100,128,200000-128},Case{100,0,100},Case{200000,65536,200000}}){
  DeleteFileW(out);logOffset.QuadPart=c.offset;require(CopyLog(out,c.limit));auto b=read(out);require(b==std::vector<char>(data.begin()+c.begin,data.end()));if(c.limit)require(b.size()<=c.limit);
  require(!CopyLog(out,c.limit));require(read(out)==b);
 }
 DeleteFileW(report.Path());DeleteFileW(out);require(!CopyLog(out,65536));
 std::cout<<"PASS actual Win32 log copier:64KiB cap,session start,short and empty sessions,legacy unbounded copy,existing output and missing input\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
(root/'build/test-character-log.cpp').write_text(harness)
print('PASS RR read-only capture invariant; four phases/eight images; generated actual Win32 copier tests')
