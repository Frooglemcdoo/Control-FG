// Exercise the production capture helper with the same bytecode/length inputs
// used by CreateComputePipelineState, including shaders with no storage ID.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
static std::vector<std::string> records;
static void Log(const char* fmt, ...) {
    char line[2048];va_list args;va_start(args,fmt);
    vsnprintf_s(line,sizeof(line),_TRUNCATE,fmt,args);va_end(args);
    records.emplace_back(line);
}
static bool PTReflectionSha256(const unsigned char* bytes,size_t size,char* hex) {
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    unsigned char hash[32]{};
    const auto result=BCryptHash(algorithm,nullptr,0,const_cast<PUCHAR>(bytes),
        static_cast<ULONG>(size),hash,sizeof(hash));
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(result<0)return false;
    for(size_t i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02x",hash[i]);
    return true;
}
#include "../src/pt_glass_capture.h"
static void Put(std::vector<unsigned char>& b,size_t offset,std::uint32_t value) {
    memcpy(b.data()+offset,&value,4);
}
static std::vector<unsigned char> Fixture() {
    std::vector<unsigned char> b(200,0);
    memcpy(b.data(),"DXBC",4);Put(b,24,200);Put(b,28,2);
    Put(b,32,40);Put(b,36,52);
    memcpy(b.data()+40,"SHEX",4);Put(b,44,4);Put(b,48,0x00050050);
    memcpy(b.data()+52,"RDEF",4);Put(b,56,140);
    memcpy(b.data()+60,"g_tMaterialId",14);
    memcpy(b.data()+80,"g_tNormal_TexcoordX",20);
    memcpy(b.data()+110,"g_tPosition_TexcoordY",22);
    return b;
}
int main() {
    auto first=Fixture(),second=first;second[190]=1;
    PTGlassCaptureCompute(nullptr,first.size());
    PTGlassCaptureCompute(first.data(),0);
    PTGlassCaptureCompute(first.data(),31);
    PTGlassCaptureCompute(first.data(),first.size()-1);
    PTGlassCaptureCompute(first.data(),PTGlassCapture::kMaximumBytes+1);
    assert(records.empty());
    PTGlassCaptureCompute(first.data(),first.size());
    assert(ptGlassCaptureCount==1 && records.size()==4);
    assert(records[1].find("kind=hit_lighting_candidate source=compute_pipeline")!=std::string::npos);
    char expectedHex[401]{};
    for(size_t i=0;i<first.size();++i)sprintf_s(expectedHex+i*2,3,"%02X",first[i]);
    assert(records[2].substr(records[2].find("hex=")+4)==expectedHex);
    PTGlassCaptureCompute(first.data(),first.size());
    assert(ptGlassCaptureCount==1 && records.size()==4);
    PTGlassCaptureCompute(second.data(),second.size());
    assert(ptGlassCaptureCount==2 && records.size()==8);
    assert(ptGlassCaptureSeenCount==0); // -1 does not suppress later candidates.
    PTGlassCaptureResident(123,reinterpret_cast<const char*>(second.data()));
    assert(ptGlassCaptureCount==2 && records.size()==8); // cross-source dedup
    assert(first==Fixture()); // bytes supplied by game remain untouched
    puts("PASS: production pipeline capture emits exact bytes, captures distinct ID-less shaders, deduplicates and enforces supplied bounds.");
}
