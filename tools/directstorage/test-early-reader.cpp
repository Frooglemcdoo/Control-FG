#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cassert>
#include "../../src/directstorage_early_reader.h"
static void Check(bool value,const char* label){if(!value){printf("FAIL R237 %s error=%lu\n",label,GetLastError());fflush(stdout);ExitProcess(1);}}
template<class F> static void Await(F condition,const char* label){const auto end=GetTickCount64()+10000;while(GetTickCount64()<end){if(condition())return;Sleep(1);}Check(false,label);}
static void Seek(HANDLE h,std::uint64_t offset){LARGE_INTEGER p{};p.QuadPart=offset;Check(SetFilePointerEx(h,p,nullptr,FILE_BEGIN)!=FALSE,"seek");}
static std::uint64_t Position(HANDLE h){LARGE_INTEGER z{},p{};Check(SetFilePointerEx(h,z,&p,FILE_CURRENT)!=FALSE,"position");return p.QuadPart;}
int wmain(){
 using namespace control_storage;using loader_candidate::Candidate;
 ComPtr<IDStorageFactory> factory;Check(SUCCEEDED(DStorageGetFactory(IID_PPV_ARGS(&factory))),"factory");
 EarlyReader reader;Check(reader.Initialize(factory.Get()),"initialize");
 wchar_t temp[MAX_PATH]{},base[MAX_PATH]{};Check(GetTempPathW(MAX_PATH,temp)!=0,"temp");Check(GetTempFileNameW(temp,L"cfe",0,base)!=0,"tempname");DeleteFileW(base);
 const std::wstring path=std::wstring(base)+L".rmdp",other=std::wstring(base)+L"-other.rmdp";
 std::vector<unsigned char> source(BlockBytes+1024);for(unsigned i=0;i<source.size();++i)source[i]=static_cast<unsigned char>((i*73)^(i>>8));
 for(const auto& name:{path,other}){HANDLE h=CreateFileW(name.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);Check(h!=INVALID_HANDLE_VALUE,"create");DWORD bytes=0;Check(WriteFile(h,source.data(),static_cast<DWORD>(source.size()),&bytes,nullptr)&&bytes==source.size(),"write");CloseHandle(h);}
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 HANDLE different=CreateFileW(other.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 Check(file!=INVALID_HANDLE_VALUE&&different!=INVALID_HANDLE_VALUE,"native files");
 unsigned char output[1024]{};DWORD received=0;Candidate request{1,10,20,0,{713,1024}};
 SetLastError(123);Check(!reader.TryConsume(file,request,output,1024,&received)&&GetLastError()==123,"cold native fallback");
 Await([&]{if(reader.counters.registered==0)reader.TryConsume(file,request,output,1024,&received);return reader.counters.registered>0;},"registration handoff");
 Check(reader.counters.submitted==0,"demand does not submit");
 // Registration's duplicate is owned independently and never seeks the native handle.
 CloseHandle(file);file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 Await([&]{return reader.counters.filesReady==1;},"background registration");
 Await([&]{reader.Offer(&request,1,1,10);return reader.counters.offered>0;},"selected offer inspected");Check(reader.counters.admitted==0,"exclude selected request");
 request.id=2;const auto completed=reader.counters.completed.load();Await([&]{reader.Offer(&request,1,1,10);return reader.counters.admitted>0;},"offer ready request");
 Await([&]{return reader.counters.completed>completed;},"early completion");
 const auto submissions=reader.counters.submitted.load();Await([&]{reader.Offer(&request,1,1,10);return reader.counters.duplicates>0;},"duplicate suppression");Check(reader.counters.submitted==submissions,"duplicates submit no work");
 Candidate wrongOwner=request;wrongOwner.owner=11;Seek(file,713);Check(!reader.TryConsume(file,wrongOwner,output,1024,&received)&&Position(file)==713,"owner isolation");
 Await([&]{SetLastError(456);const bool hit=reader.TryConsume(file,request,output,1024,&received);Check(GetLastError()==456,"last error");return hit;},"ready hit");
 Check(received==1024&&memcmp(output,source.data()+713,1024)==0&&Position(file)==1737,"exact bytes and native position");
 Seek(file,713);Check(!reader.TryConsume(file,request,output,1024,&received)&&reader.counters.submitted==submissions,"consume once, no demand retry");
 // A path with equal length and contents is still a distinct identity.
 request.id=3;auto before=reader.counters.completed.load();auto admitted=reader.counters.admitted.load();Await([&]{reader.Offer(&request,1,1,10);return reader.counters.admitted>admitted;},"offer identity fixture");Await([&]{return reader.counters.completed>before;},"identity fixture ready");
 memset(output,0xcc,sizeof(output));Seek(different,713);Await([&]{Check(!reader.TryConsume(different,request,output,1024,&received)&&output[0]==0xcc&&Position(different)==713,"different file rejects without mutation");return reader.counters.identityRejected==1;},"identity rejection observed");
 Await([&]{return reader.counters.filesReady==2;},"second identity registered");
 // Invalidating native archives forbids old ready data and safely retires sources.
 Await([&]{Check(!reader.TryConsume(file,request,output,1024,&received),"restore original mapping");return reader.counters.registered>=3;},"restore registration");
 Await([&]{return reader.counters.filesReady>=3;},"restored mapping ready");
 Candidate stale{30,10,20,0,{713,1024}};before=reader.counters.completed.load();admitted=reader.counters.admitted.load();
 Await([&]{reader.Offer(&stale,1,1,10);return reader.counters.admitted>admitted;},"offer stale fixture");
 Await([&]{return reader.counters.completed>before;},"stale fixture was ready");
 Seek(file,713);const auto servedBeforeInvalidation=reader.counters.served.load();reader.Invalidate();
 Await([&]{Check(!reader.TryConsume(file,stale,output,1024,&received)&&Position(file)==713&&reader.counters.served==servedBeforeInvalidation,"ready old generation must not serve");return reader.counters.registered>=4;},"epoch registration handoff");
 Await([&]{return reader.counters.filesReady>=4;},"new epoch registration");
 request.id=4;before=reader.counters.completed.load();admitted=reader.counters.admitted.load();Await([&]{reader.Offer(&request,1,1,10);return reader.counters.admitted>admitted;},"offer position fixture");Await([&]{return reader.counters.completed>before;},"position fixture ready");
 Seek(file,714);Await([&]{Check(!reader.TryConsume(file,request,output,1024,&received)&&Position(file)==714,"wrong position falls back unchanged");return reader.counters.positionRejected==1;},"position rejection observed");
 request.id=5;before=reader.counters.completed.load();admitted=reader.counters.admitted.load();Await([&]{reader.Offer(&request,1,1,10);return reader.counters.admitted>admitted;},"offer expiry fixture");Await([&]{return reader.counters.completed>before;},"expiry fixture ready");
 Sleep(280);Seek(file,713);Check(!reader.TryConsume(file,request,output,1024,&received)&&reader.counters.expired>0,"unused results expire");
 // In-flight cancellation/unmount never frees a destination before status completes.
 Candidate pending[16]{};for(unsigned i=0;i<16;++i)pending[i]={100+i,10,20,0,{i*16,1024}};
 admitted=reader.counters.admitted.load();const auto submittedBefore=reader.counters.submitted.load();Await([&]{reader.Offer(pending,16,1,10);return reader.counters.admitted>=admitted+16;},"bounded batch admission");Await([&]{return reader.counters.submitted>=submittedBefore+16;},"bounded batch submitted");reader.Invalidate();Check(reader.Stop(),"shutdown waits for retirement");
 CloseHandle(file);CloseHandle(different);Check(DeleteFileW(path.c_str())&&DeleteFileW(other.c_str()),"owned handles released");
 puts("PASS R237 early reader: real DirectStorage bytes, no demand submission, owned registration, owner/identity/position checks, epoch invalidation, expiry and in-flight retirement");
}
