#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <array>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <conio.h>

// Standalone, exact-build exploration helper. No DLL injection or disk patches.
struct Site { size_t rva; const char* expected; };
constexpr Site sites[] = {
    {0x328bd0,"48895c24104889742418574883ec20488bd9410fb6f8488b"},
    {0x32a7f0,"48895c2410574883ec2083b964010000008bfa488bd97548"},
    {0x32a860,"40534883ec2083b96401000000488bd97552488b49288379"},
    {0x5313a0,"48895c24084889742410574883ec2080b9cb00000000410f"}
};
constexpr char gameHash[] = "9441db3ae75b267abd989846ad0895e3fe24abcc0f06e58f93c34fc8d4736506";
unsigned Nibble(char c) { return c <= '9' ? c-'0' : c-'a'+10; }
std::vector<unsigned char> Bytes(const char* text) {
    std::vector<unsigned char> result;
    for(size_t i=0;text[i];i+=2) result.push_back(static_cast<unsigned char>(Nibble(text[i])*16+Nibble(text[i+1])));
    return result;
}
bool Matches(HANDLE process, unsigned char* address, const Site& site, bool patched) {
    auto expected=Bytes(site.expected); if(patched) expected[0]=0xc3;
    std::vector<unsigned char> actual(expected.size()); SIZE_T read=0;
    return ReadProcessMemory(process,address,actual.data(),actual.size(),&read) && read==actual.size() && actual==expected;
}
bool WriteByte(HANDLE process, unsigned char* address, unsigned char value) {
    DWORD old=0,ignored=0; SIZE_T written=0;
    if(!VirtualProtectEx(process,address,1,PAGE_EXECUTE_READWRITE,&old))return false;
    const bool wrote=WriteProcessMemory(process,address,&value,1,&written) && written==1;
    const bool flushed=FlushInstructionCache(process,address,1)!=FALSE;
    const bool restored=VirtualProtectEx(process,address,1,old,&ignored)!=FALSE;
    return wrote && flushed && restored;
}
struct Session {
    HANDLE process=nullptr; unsigned char* base=nullptr; std::array<bool,4> owned{};
    bool Restore() {
        bool okay=true;
        for(size_t i=0;i<4;++i) if(owned[i]) {
            auto* address=base+sites[i].rva;
            if(Matches(process,address,sites[i],false)){owned[i]=false;continue;}
            // Do not overwrite another tool's changes.
            if(!Matches(process,address,sites[i],true) || !WriteByte(process,address,Bytes(sites[i].expected)[0])) okay=false;
            else owned[i]=false;
        }
        return okay;
    }
    bool Enable() {
        // Validate every site before the first write.
        for(const auto& site:sites) if(!Matches(process,base+site.rva,site,false))return false;
        for(size_t i=0;i<4;++i) {
            owned[i]=true; // Also attempt rollback if write succeeded but cache/protection failed.
            if(!WriteByte(process,base+sites[i].rva,0xc3) || !Matches(process,base+sites[i].rva,sites[i],true)) {
                if(!Restore()) std::puts("Rollback incomplete. Restart Control to restore its original code.");
                return false;
            }
        }
        return true;
    }
};
bool HashMatches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    bool okay=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    std::array<unsigned char,65536> buffer{}; DWORD count=0;
    while(okay) {
        if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&count,nullptr)){okay=false;break;}
        if(!count)break;
        okay=BCryptHashData(hash,buffer.data(),count,0)>=0;
    }
    std::array<unsigned char,32> digest{};
    if(okay)okay=BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash); if(alg)BCryptCloseAlgorithmProvider(alg,0); CloseHandle(file);
    const auto expected=Bytes(gameHash);
    return okay && std::memcmp(digest.data(),expected.data(),digest.size())==0;
}
bool Attach(Session& session) {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot==INVALID_HANDLE_VALUE)return false;
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);DWORD id=0;unsigned matches=0;
    if(Process32FirstW(snapshot,&entry))do {
        if(_wcsicmp(entry.szExeFile,L"Control_DX12.exe")==0){id=entry.th32ProcessID;++matches;}
    }while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot);
    if(matches!=1){std::puts("Start one instance of Control DX12 and leave it at the main menu first.");return false;}
    session.process=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,id);
    if(!session.process){std::puts("Cannot access Control. Try running this helper as administrator.");return false;}
    wchar_t path[32768]{};DWORD length=32768;
    if(!QueryFullProcessImageNameW(session.process,0,path,&length) || !HashMatches(path)) {
        std::puts("Unsupported game EXE. No changes made; this helper only accepts the audited DX12 build.");return false;
    }
    snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,id);
    if(snapshot==INVALID_HANDLE_VALUE)return false;
    MODULEENTRY32W module{};module.dwSize=sizeof(module);
    if(Module32FirstW(snapshot,&module))do {
        if(_wcsicmp(module.szExePath,path)==0){session.base=module.modBaseAddr;break;}
    }while(Module32NextW(snapshot,&module));
    CloseHandle(snapshot);return session.base!=nullptr;
}
bool SelfTest() {
    Session test;test.process=GetCurrentProcess();
    test.base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x540000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!test.base)return false;
    for(const auto& site:sites){auto b=Bytes(site.expected);std::memcpy(test.base+site.rva,b.data(),b.size());}
    bool okay=test.Enable();
    for(const auto& site:sites)okay=okay&&Matches(test.process,test.base+site.rva,site,true);
    okay=okay&&test.Restore();
    for(const auto& site:sites)okay=okay&&Matches(test.process,test.base+site.rva,site,false);
    test.base[sites[2].rva+5]^=1;
    okay=okay&&!test.Enable()&&Matches(test.process,test.base+sites[0].rva,sites[0],false);
    VirtualFree(test.base,0,MEM_RELEASE);return okay;
}
int main(int argc,char** argv) {
    if(argc==2 && std::strcmp(argv[1],"--self-test")==0){bool ok=SelfTest();std::puts(ok?"PASS patch, restore and mismatch rejection":"FAIL");return ok?0:1;}
    std::puts("Control - No Encounter Starts (exploration test 1)\nUse at the MAIN MENU before loading a test area.\nExisting enemies and story locks are not removed.\nUse a backed-up save: blocked encounters can prevent mission progression.\n");
    Session session;
    if(!Attach(session) || !session.Enable()) {
        std::puts("Not enabled. No supported encounter patch could be applied. Press any key to exit.");
        if(session.process)CloseHandle(session.process);_getch();return 1;
    }
    std::puts("ENABLED: new encounter starts blocked. Return to Control and load your area.\nKeep this window open. Press Enter here to restore encounters and exit.\nClosing/killing this helper may leave the patch active until Control restarts.");
    while(WaitForSingleObject(session.process,100)==WAIT_TIMEOUT) {
        if(_kbhit() && _getch()=='\r') {
            std::puts(session.Restore()?"Original encounter code restored.":"Restore failed: restart Control to restore original code.");break;
        }
    }
    CloseHandle(session.process);return 0;
}
