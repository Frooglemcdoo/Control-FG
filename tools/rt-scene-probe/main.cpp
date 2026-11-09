#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// External, read-only identification of the native scene-query implementation.
// This is NOT an atomic scene snapshot or a cache-validity/lifetime oracle.
struct Handle {
 HANDLE value=INVALID_HANDLE_VALUE;
 explicit Handle(HANDLE h):value(h){}
 ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
 Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
struct Module {std::wstring name,path;std::uint64_t base=0,size=0;};
std::string Narrow(const std::wstring& s){
 std::string out;for(wchar_t c:s)out+=c>=32&&c<127?static_cast<char>(c):'?';return out;
}
std::string HashFile(const std::wstring& path){
 std::vector<unsigned char> object;
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 struct Cleanup {BCRYPT_ALG_HANDLE& a;BCRYPT_HASH_HANDLE& h;~Cleanup(){if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);}} cleanup{alg,hash};
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 provider");
 DWORD n=0,bytes=0;
 if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&bytes),sizeof(bytes),&n,0)<0)throw std::runtime_error("SHA256 size");
 object.resize(bytes);
 if(BCryptCreateHash(alg,&hash,object.data(),bytes,nullptr,0,0)<0)throw std::runtime_error("SHA256 create");
 std::ifstream in(std::filesystem::path(path),std::ios::binary);if(!in)throw std::runtime_error("Cannot read module file");
 std::array<unsigned char,65536> block{};
 while(in){in.read(reinterpret_cast<char*>(block.data()),block.size());const auto got=in.gcount();if(got>0&&BCryptHashData(hash,block.data(),static_cast<ULONG>(got),0)<0)throw std::runtime_error("SHA256 update");}
 if(!in.eof())throw std::runtime_error("Module file read failed");
 std::array<unsigned char,32> digest{};if(BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)<0)throw std::runtime_error("SHA256 finish");
 std::ostringstream out;for(auto c:digest)out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);return out.str();
}
template<class T> bool Read(HANDLE p,std::uint64_t address,T& value){
 SIZE_T got=0;T temp{};if(!address||!ReadProcessMemory(p,reinterpret_cast<const void*>(address),&temp,sizeof(temp),&got)||got!=sizeof(temp))return false;value=temp;return true;
}
std::vector<Module> Modules(DWORD pid){
 Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
 if(snapshot.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Module enumeration failed");
 MODULEENTRY32W m{};m.dwSize=sizeof(m);std::vector<Module> out;
 if(!Module32FirstW(snapshot.value,&m))throw std::runtime_error("Module list empty");
 do{out.push_back({m.szModule,m.szExePath,reinterpret_cast<std::uint64_t>(m.modBaseAddr),m.modBaseSize});}while(Module32NextW(snapshot.value,&m));return out;
}
const Module* Find(const std::vector<Module>& modules,std::uint64_t address){
 for(const auto& m:modules)if(address>=m.base&&address-m.base<m.size)return &m;return nullptr;
}
DWORD Process(){
 Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
 PROCESSENTRY32W p{};p.dwSize=sizeof(p);DWORD found=0;
 if(snapshot.value==INVALID_HANDLE_VALUE||!Process32FirstW(snapshot.value,&p))throw std::runtime_error("Process enumeration failed");
 do{if(_wcsicmp(p.szExeFile,L"Control_DX12.exe")==0){if(found)throw std::runtime_error("More than one Control DX12 process: close the extra instance");found=p.th32ProcessID;}}while(Process32NextW(snapshot.value,&p));
 if(!found)throw std::runtime_error("Start Control DX12 and load into gameplay first");return found;
}
struct Sample {std::uint64_t job=0,context=0,query=0,vtable=0,target=0,frame=0;std::array<std::uint32_t,4> counts{};};
bool SampleJob(HANDLE p,std::uint64_t renderer,Sample& s){
 // Verified renderer: preparation job handle at RVA 90E9D8; job+18 context,
 // job+28 query object, vtable+50 query function. Only read bounded scalars.
 std::array<std::uint64_t,6> first{},last{};std::uint64_t endJob=0,endFrame=0,endVtable=0,endTarget=0;
 if(!Read(p,renderer+0x90e9d8,s.job)||!Read(p,s.job,first)||first[0]!=renderer+0x6281e8)return false;
 s.context=first[3];s.query=first[5];
 if(!Read(p,s.context+0xd8,s.frame)||!Read(p,s.query,s.vtable)||!Read(p,s.vtable+0x50,s.target))return false;
 constexpr unsigned offsets[]={0x18,0x78,0x88,0x98};
 for(unsigned i=0;i<4;++i)if(!Read(p,s.context+offsets[i],s.counts[i]))return false;
 if(!Read(p,s.context+0xd8,endFrame)||!Read(p,s.query,endVtable)||!Read(p,s.vtable+0x50,endTarget)||!Read(p,s.job,last)||!Read(p,renderer+0x90e9d8,endJob))return false;
 // Reject obvious races. Matching reads cannot rule out ABA or partial arrays;
 // counts are observations only and no scene decisions use them.
 return s.frame==endFrame&&s.vtable==endVtable&&s.target==endTarget&&s.job==endJob&&first==last;
}
int main(){
 std::ofstream log("RT-Scene-Probe.txt",std::ios::trunc);
 if(!log){std::cerr<<"Cannot create RT-Scene-Probe.txt in this folder\n";return 1;}
 const auto say=[&](const std::string& s){std::cout<<s<<std::endl;log<<s<<std::endl;};
 try{
  say("CONTROL RT SCENE PROBE v1; read_only=1; injected_code=0; game_settings_changed=0");
  const DWORD pid=Process();Handle process(OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid));
  if(!process.value)throw std::runtime_error("Cannot read Control process; run game and probe at the same permission level");
  const auto modules=Modules(pid);const Module* renderer=nullptr;
  for(const auto& m:modules)if(_wcsicmp(m.name.c_str(),L"renderer_rmdwin10_f.dll")==0)renderer=&m;
  if(!renderer)throw std::runtime_error("Renderer module not loaded");
  const auto hash=HashFile(renderer->path);say("renderer_sha256="+hash);
  if(hash!="ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433")throw std::runtime_error("Unsupported renderer: no scene pointers were read");
  const std::array<unsigned char,4> expectedQuery{0x41,0xff,0x52,0x50};std::array<unsigned char,4> queryCall{};
  if(!Read(process.value,renderer->base+0xa83d9,queryCall)||queryCall!=expectedQuery)throw std::runtime_error("Live query call differs from verified renderer");
  say("Capturing for 30 seconds. Return to Control, walk around and destroy a few objects. Keep R392 settings unchanged.");
  std::map<std::pair<std::uint64_t,std::uint64_t>,unsigned> targets;
  std::array<std::uint32_t,4> low{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX},high{};
  unsigned accepted=0,rejected=0;const auto start=GetTickCount64();
  while(GetTickCount64()-start<30000){
   DWORD code=0;if(!GetExitCodeProcess(process.value,&code)||code!=STILL_ACTIVE)throw std::runtime_error("Control exited during capture");
   Sample s;
   if(SampleJob(process.value,renderer->base,s)){
    const auto* target=Find(modules,s.target);const auto* table=Find(modules,s.vtable);
    if(target&&table){
     ++accepted;for(unsigned i=0;i<4;++i){low[i]=(std::min)(low[i],s.counts[i]);high[i]=(std::max)(high[i],s.counts[i]);}
     if(++targets[{s.vtable,s.target}]==1){
      std::ostringstream row;row<<"query_module="<<Narrow(target->name)<<" query_rva=0x"<<std::hex<<(s.target-target->base)<<" vtable_module="<<Narrow(table->name)<<" vtable_rva=0x"<<(s.vtable-table->base);say(row.str());
      say("query_module_sha256="+HashFile(target->path));
     }
    }else ++rejected;
   }else ++rejected;
   Sleep(20);
  }
  say("accepted_samples="+std::to_string(accepted)+" rejected_samples="+std::to_string(rejected)+" unique_query_targets="+std::to_string(targets.size()));
  if(!accepted)throw std::runtime_error("No stable preparation job observed. Return this log; do not change game threading settings");
  const char* labels[]={"objects","instances","geometry","material_refs"};
  for(unsigned i=0;i<4;++i)say(std::string(labels[i])+" observed_min="+std::to_string(low[i])+" observed_max="+std::to_string(high[i]));
  say("PASS query target identified; metadata_only=1; atomic_scene_snapshot=0; native_retirement=0; performance_claim=0");return 0;
 }catch(const std::exception& e){say(std::string("FAIL ")+e.what());return 1;}
}
