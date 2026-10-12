#pragma once
#include "pt_dxbc_checksum.h"
#include <vector>
#include <algorithm>
// Bytecode-only adapter for the nine reviewed general-surface variants.
// Host must SHA256-match the original shader to Profile::sha, preserve the native
// graphics root and provide a private table with the PT float4 texture at t35.
// Alpha is a per-pixel 0/1 admission mask; RGB is reflected surface radiance.
// No original shader, heap, root, render target, or game file is modified here.
namespace PTSurfaceComposition {
using U32=std::uint32_t;
struct Profile {const char* sha;std::array<unsigned char,16> checksum;U32 bytes,load,combine,temps,resource;};
#include "pt_surface_profiles.in.h"
inline bool Patch(const unsigned char* input,std::size_t bytes,const char* verifiedSha,std::vector<unsigned char>& output){
    output.clear();if(!input||!verifiedSha||bytes<32||bytes>256*1024)return false;
    const Profile* profile=nullptr;for(const auto& p:Profiles)if(!std::strcmp(p.sha,verifiedSha)){profile=&p;break;}
    if(!profile||bytes!=profile->bytes||std::memcmp(input,"DXBC",4)||PTDxbc::Word(input+20)!=1||PTDxbc::Word(input+24)!=bytes||
       std::memcmp(input+4,profile->checksum.data(),16)||PTDxbc::Checksum(input+20,static_cast<U32>(bytes-20))!=profile->checksum)return false;
    struct Chunk {U32 tag;std::vector<unsigned char> data;};std::vector<Chunk> chunks;
    const U32 count=PTDxbc::Word(input+28);if(!count||count>32||32+count*4>bytes)return false;
    bool patched=false;
    for(U32 n=0;n<count;++n){
        const U32 at=PTDxbc::Word(input+32+n*4);if(at>bytes-8)return false;
        const U32 tag=PTDxbc::Word(input+at),size=PTDxbc::Word(input+at+4);if(size>bytes-at-8)return false;
        // Reflection/statistics describe the old program. Runtime binding comes
        // from declarations; retain input/output signatures and feature flags.
        if(tag==0x46454452u||tag==0x54415453u)continue;
        Chunk chunk{tag,std::vector<unsigned char>(input+at+8,input+at+8+size)};
        if(tag==0x58454853u){
            if(patched||size%4||size<8)return false;
            std::vector<U32> words(size/4);for(std::size_t i=0;i<words.size();++i)words[i]=PTDxbc::Word(chunk.data.data()+4*i);
            if(words[0]!=0x51||words[1]!=words.size()||profile->load+8>words.size()||profile->combine+9>words.size()||profile->temps+2>words.size())return false;
            const auto load=profile->load,combine=profile->combine,temps=profile->temps;
            if(words[load]!=0x0800002du||words[load+5]>>12!=0x207u||words[load+6]||words[load+7]||
               words[combine]!=0x09000032u||words[combine+1]!=0x00100072u||words[combine+2]||
               words[temps]!=0x02000068u||!words[temps+1]||words[temps+1]>=4096)return false;
            const U32 temporary=words[temps+1];
            const U32 declaration[]={0x07001858u,0x00307e46u,profile->resource,35,35,0x5555,0};
            const U32 fetch[]={0x0800002du,0x001000f2u,temporary,words[load+3],words[load+4],0x00207e46u,profile->resource,35};
            const U32 select[]={0x09000037u,0x00100072u,0,0x00100ff6u,temporary,0x00100e46u,temporary,0x00100e46u,0};
            std::vector<U32> revised;revised.reserve(words.size()+24);revised.push_back(words[0]);revised.push_back(words[1]+24);
            bool sawLoad=false,sawCombine=false,sawTemps=false;
            for(std::size_t i=2;i<words.size();){
                U32 length=(words[i]>>24)&127;
                if((words[i]&2047)==53){if(i+1>=words.size())return false;length=words[i+1];}
                if(!length||length>words.size()-i)return false;
                if(i==temps){revised.insert(revised.end(),std::begin(declaration),std::end(declaration));sawTemps=true;}
                if(i==load){revised.insert(revised.end(),std::begin(fetch),std::end(fetch));sawLoad=true;}
                const auto start=revised.size();revised.insert(revised.end(),words.begin()+i,words.begin()+i+length);
                if(i==temps)revised[start+1]=temporary+1;
                if(i==combine){revised.insert(revised.end(),std::begin(select),std::end(select));sawCombine=true;}
                i+=length;
            }
            if(!sawLoad||!sawCombine||!sawTemps||revised.size()!=words.size()+24)return false;
            chunk.data.resize(revised.size()*4);for(std::size_t i=0;i<revised.size();++i)PTDxbc::Put(chunk.data.data()+i*4,revised[i]);patched=true;
        }
        chunks.push_back(std::move(chunk));
    }
    if(!patched)return false;
    std::size_t total=32+chunks.size()*4;for(const auto& chunk:chunks)total+=8+chunk.data.size();
    std::vector<unsigned char> result(total);std::memcpy(result.data(),"DXBC",4);PTDxbc::Put(result.data()+20,1);
    PTDxbc::Put(result.data()+24,static_cast<U32>(total));PTDxbc::Put(result.data()+28,static_cast<U32>(chunks.size()));
    std::size_t cursor=32+chunks.size()*4;
    for(std::size_t n=0;n<chunks.size();++n){const auto& chunk=chunks[n];PTDxbc::Put(result.data()+32+n*4,static_cast<U32>(cursor));
        PTDxbc::Put(result.data()+cursor,chunk.tag);PTDxbc::Put(result.data()+cursor+4,static_cast<U32>(chunk.data.size()));
        std::memcpy(result.data()+cursor+8,chunk.data.data(),chunk.data.size());cursor+=8+chunk.data.size();}
    const auto hash=PTDxbc::Checksum(result.data()+20,static_cast<U32>(total-20));std::memcpy(result.data()+4,hash.data(),16);
    output=std::move(result);return true;
}
}
