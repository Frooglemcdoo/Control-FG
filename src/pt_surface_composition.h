#pragma once
#include "pt_dxbc_checksum.h"
#include <vector>
#include <algorithm>
// Bytecode-only adapter for the nine reviewed general-surface variants.
// Host must SHA256-match the original shader to Profile::sha, preserve the native
// graphics root and provide PT radiance through the appended raw root SRV t0,space2.
// 64-byte packet: summary+coverage, geometry metadata, unit-color diffuse, specular.
// Native material color multiplies diffuse only; emission and fog remain native.
// No original shader, heap, root, render target, or game file is modified here.
namespace PTSurfaceComposition {
using U32=std::uint32_t;
struct Profile {const char* sha;std::array<unsigned char,16> checksum;U32 bytes,load,combine,temps,resource;std::uint64_t required;};
#include "pt_surface_profiles.in.h"
inline bool PatchProfile(const unsigned char* input,std::size_t bytes,const Profile* profile,std::vector<unsigned char>& output){
    output.clear();if(!input||bytes<32||bytes>256*1024)return false;
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
            U32 cbId=0;
            for(std::size_t i=2;i<words.size();){
                U32 length=(words[i]>>24)&127;
                if((words[i]&2047)==53){if(i+1>=words.size())return false;length=words[i+1];}
                if(!length||length>words.size()-i)return false;
                if((words[i]&2047)==89){if(length!=7)return false;cbId=std::max(cbId,words[i+2]+1);}
                i+=length;
            }
            // Raw root SRV t0,space2. Width is b0,space2; the first native
            // lighting load already holds exact integer internal pixel coords.
            const U32 declaration[]={0x060000a1u,0x00307e46u,profile->resource,0,0,2,
                0x060000a1u,0x00307e46u,profile->resource+1,1,1,2,
                0x07000059u,0x00308e46u,cbId,0,0,1,2};
            const U32 coordinate=words[load+4];
            if(((words[load+3]>>12)&255)!=0||((words[load+3]>>20)&3)!=1||((words[load+3]>>4)&15)!=4)return false;
            const U32 scratch=temporary+1,diffuse=temporary+2,specular=temporary+3;
            const U32 fetch[]={
                // Root descriptors have no bounds metadata. Reject negative or
                // out-of-extent native coordinates BEFORE issuing any raw load.
                0x0900004fu,0x00100032u,scratch,0x00100046u,coordinate,0x00308046u,cbId,0,0,
                0x07000001u,0x00100012u,scratch,0x0010000au,scratch,0x0010001au,scratch,
                // Consumer-local guard. Reject before touching the shared PT buffer.
                0x080000a5u,0x00100022u,scratch,0x00004001u,0,0x00207e46u,profile->resource+1,1,
                0x07000001u,0x00100012u,scratch,0x0010000au,scratch,0x0010001au,scratch,
                0x08000036u,0x001000f2u,temporary,0x00004002u,0,0,0,0,
                0x0304001fu,0x0010000au,scratch,
                0x0b000023u,0x00100012u,scratch,0x0010001au,coordinate,0x0030800au,cbId,0,0,0x0010000au,coordinate,
                0x07000029u,0x00100012u,scratch,0x0010000au,scratch,0x00004001u,6,
                0x080000a5u,0x001000f2u,temporary,0x0010000au,scratch,0x00207e46u,profile->resource,0,
                // Keep each primary lobe separate through native material evaluation.
                0x0700001eu,0x00100012u,scratch,0x0010000au,scratch,0x00004001u,32,
                0x080000a5u,0x00100072u,diffuse,0x0010000au,scratch,0x00207e46u,profile->resource,0,
                0x0700001eu,0x00100012u,scratch,0x0010000au,scratch,0x00004001u,16,
                0x080000a5u,0x00100072u,specular,0x0010000au,scratch,0x00207e46u,profile->resource,0,
                0x01000015u};
            const U32 select[]={0x09000037u,0x00100072u,0,0x00100ff6u,temporary,0x00100e46u,temporary,0x00100e46u,0};
            const U32 materialize[]={
                0x0304001fu,0x00100ff6u,temporary,
                0x09000032u,0x00100072u,temporary,0x00100e46u,diffuse,
                words[combine+5],words[combine+6],0x00100e46u,specular,
                0x01000015u};
            constexpr U32 added=140;
            std::vector<U32> revised;revised.reserve(words.size()+added);revised.push_back(words[0]);revised.push_back(words[1]+added);
            bool sawLoad=false,sawCombine=false,sawTemps=false;
            for(std::size_t i=2;i<words.size();){
                U32 length=(words[i]>>24)&127;
                if((words[i]&2047)==53){if(i+1>=words.size())return false;length=words[i+1];}
                if(!length||length>words.size()-i)return false;
                if(i==temps){revised.insert(revised.end(),std::begin(declaration),std::end(declaration));sawTemps=true;}
                if(i==load){revised.insert(revised.end(),std::begin(fetch),std::end(fetch));sawLoad=true;}
                if(i==combine)revised.insert(revised.end(),std::begin(materialize),std::end(materialize));
                const auto start=revised.size();revised.insert(revised.end(),words.begin()+i,words.begin()+i+length);
                if(i==temps)revised[start+1]=temporary+4;
                if(i==combine){revised.insert(revised.end(),std::begin(select),std::end(select));sawCombine=true;}
                i+=length;
            }
            if(!sawLoad||!sawCombine||!sawTemps||revised.size()!=words.size()+added)return false;
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
// Runtime entry point admits ONLY the SHA-reviewed profiles. PatchProfile is
// also exercised with compiler-generated fixtures by the GPU validation test.
inline bool Patch(const unsigned char* input,std::size_t bytes,const char* verifiedSha,std::vector<unsigned char>& output){
    const Profile* profile=nullptr;
    if(verifiedSha)for(const auto& p:Profiles)if(!std::strcmp(p.sha,verifiedSha)){profile=&p;break;}
    return PatchProfile(input,bytes,profile,output);
}
}
