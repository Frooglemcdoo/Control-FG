#pragma once
#include "pt_native_binding.h"
#include <map>
#include <vector>
#include <cstdint>
namespace PTSceneTopology {
using U64=std::uint64_t;
struct Instance {float transform[12];std::uint32_t idMask,contributionFlags;U64 blas;};
static_assert(sizeof(Instance)==64,"DXR instance ABI");
struct Geometry {U64 indices,positions,stride;std::uint32_t indexStride,indexCount,vertexCount;};
struct Result {U64 instances=0,matched=0,missingBlas=0,outOfRange=0,mismatch=0,conflict=0,masked=0;};
// CPU relation checks only. A caller must separately prove submission ordering,
// allocation identity, resource states and vertex/attribute correspondence.
class Index {
    std::map<U64,std::vector<Geometry>> blas;std::size_t rows=0;
public:
    bool Build(U64 address,std::vector<Geometry> geometry){
        auto old=blas.find(address);const auto previous=old==blas.end()?0:old->second.size();
        if(!address||geometry.empty()||geometry.size()>4096||(blas.size()>=16384&&old==blas.end())||geometry.size()>131072-(rows-previous)){
            if(old!=blas.end()){rows-=previous;blas.erase(old);}return false;
        }
        rows=rows-previous+geometry.size();blas[address]=std::move(geometry);return true;
    }
    bool Compact(U64 destination,U64 source){
        auto i=blas.find(source);if(i==blas.end()){Invalidate(destination,1);return false;}
        return Build(destination,i->second);
    }
    void Invalidate(U64 address,U64 bytes){
        for(auto i=blas.lower_bound(address);i!=blas.end()&&i->first-address<bytes;){rows-=i->second.size();i=blas.erase(i);}
    }
    void Clear(){blas.clear();rows=0;}
    Result Check(const std::vector<Instance>& instances,const std::vector<PTNativeBinding::Binding>& bindings,unsigned rays)const {
        Result out{};std::map<U64,Geometry> assigned;
        for(const auto& instance:instances){
            ++out.instances;if(!(instance.idMask>>24)){++out.masked;continue;}
            auto i=blas.find(instance.blas);if(i==blas.end()){++out.missingBlas;continue;}
            const U64 contribution=instance.contributionFlags&0xffffffu;
            if(!rays||contribution%rays){++out.outOfRange;continue;}
            for(std::size_t g=0;g<i->second.size();++g){
                const U64 row=contribution/rays+g; // TraceRay multiplier is rays.
                if(row>=bindings.size()){++out.outOfRange;continue;}
                const auto& geometry=i->second[g];const auto& b=bindings[static_cast<std::size_t>(row)];U64 address=0;
                if(!PTNativeBinding::IndexAddress(b,address)||address!=geometry.indices||b.constants[1]!=geometry.indexStride||
                   b.constants[0]>=b.constants[8]||U64(geometry.indexCount)*geometry.indexStride>U64(b.constants[8])-b.constants[0]){++out.mismatch;continue;}
                const auto old=assigned.find(row);
                if(old!=assigned.end()&&(old->second.positions!=geometry.positions||old->second.stride!=geometry.stride||old->second.indexCount!=geometry.indexCount||old->second.vertexCount!=geometry.vertexCount)){++out.conflict;continue;}
                assigned[row]=geometry;++out.matched;
            }
        }
        return out;
    }
};
}
