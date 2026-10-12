#pragma once
#include "pt_scene_admission.h"
#include "pt_native_binding.h"
#include <algorithm>
#include <map>
#include <vector>
// CPU candidate index. The owner supplies retained resources and serializes calls.
// This is NOT a TLAS-to-SBT correspondence proof or a GPU submission lifetime.
namespace PTOwnedScene {
using U64=std::uint64_t;
struct Allocation {U64 token,address,bytes;};
struct Triangle {U64 indices,positions,positionStride,indexToken,positionToken;std::uint32_t indexStride,indexCount,vertexCount;};
enum class Status {candidate,missingAllocation,missingGeometry,ambiguousGeometry,invalidBinding,invalidGeometry};
class Index {
    std::map<U64,Allocation> allocations;
    std::map<U64,std::vector<Triangle>> triangles;
    std::size_t triangleCount=0;
    mutable std::map<std::pair<U64,U64>,U64> lookup;
public:
    static constexpr std::size_t MaximumTriangles=65536;
    void Clear(){allocations.clear();triangles.clear();triangleCount=0;lookup.clear();}
    void Insert(Allocation a){lookup.clear();allocations[a.token]=a;}
    void Remove(U64 token){lookup.clear();allocations.erase(token);}
    bool Contains(U64 token,U64 address,U64 bytes)const {
        auto it=allocations.find(token);if(it==allocations.end())return false;
        const auto& a=it->second;
        return bytes&&address>=a.address&&address-a.address<=a.bytes&&bytes<=a.bytes-(address-a.address);
    }
    U64 Find(U64 address,U64 bytes)const {
        const auto key=std::make_pair(address,bytes);const auto cached=lookup.find(key);
        if(cached!=lookup.end())return cached->second;
        U64 found=0;
        for(const auto& entry:allocations)if(Contains(entry.first,address,bytes)){
            if(found){found=0;break;} // Overlapping retained allocations are not guessed.
            found=entry.first;
        }
        if(lookup.size()<65536)lookup.emplace(key,found);
        return found;
    }
    bool Observe(Triangle t){
        if(!t.indices||!t.positions||(t.indexStride!=2&&t.indexStride!=4)||!t.indexCount||t.indexCount%3||!t.vertexCount||t.positionStride<12||t.positionStride>UINT32_MAX)return false;
        t.indexToken=Find(t.indices,U64(t.indexCount)*t.indexStride);
        t.positionToken=Find(t.positions,U64(t.vertexCount-1)*t.positionStride+12);
        if(!t.indexToken||!t.positionToken)return false;
        if(triangleCount>=MaximumTriangles&&!triangles.count(t.indices))return false;
        auto& bucket=triangles[t.indices];
        for(auto it=bucket.begin();it!=bucket.end();){
            if(!allocations.count(it->indexToken)||!allocations.count(it->positionToken)){it=bucket.erase(it);--triangleCount;}
            else ++it;
        }
        for(const auto& p:bucket)if(p.positions==t.positions&&p.positionStride==t.positionStride&&p.indexToken==t.indexToken&&p.positionToken==t.positionToken&&p.indexStride==t.indexStride&&p.indexCount==t.indexCount&&p.vertexCount==t.vertexCount)return true;
        if(triangleCount>=MaximumTriangles)return false;
        bucket.push_back(t);++triangleCount;return true;
    }
    Status Resolve(const PTNativeBinding::Binding& b,PTSceneAdmission::Record& out)const {
        out={};U64 address=0;if(!PTNativeBinding::IndexAddress(b,address)||b.constants[0]>=b.constants[8])return Status::invalidBinding;
        const auto& c=b.constants;
        const U64 indexToken=Find(b.indices,c[8]),attributeToken=Find(b.attributes,c[9]);
        if(!indexToken||!attributeToken)return Status::missingAllocation;
        auto it=triangles.find(address);if(it==triangles.end())return Status::missingGeometry;
        unsigned candidates=0;const Triangle* selected=nullptr;
        for(const auto& t:it->second){
            if(t.indexToken!=indexToken||t.indexStride!=c[1]||U64(t.indexCount)*t.indexStride>U64(c[8])-c[0]||
               !Contains(t.positionToken,t.positions,U64(t.vertexCount-1)*t.positionStride+12))continue;
            ++candidates;selected=&t;
        }
        if(!candidates)return Status::missingGeometry;
        if(candidates!=1)return Status::ambiguousGeometry;
        const auto& t=*selected;const auto& p=allocations.at(t.positionToken);
        const U64 padded=(U64(c[8])+3)&~U64(3);
        if(padded>UINT32_MAX||!Contains(indexToken,b.indices,padded))return Status::invalidGeometry;
        // One CPU resolution transaction uses epoch 1. Allocation identity is
        // separately checked through monotonically increasing owner tokens.
        PTSceneAdmission::Geometry g{{{b.indices,padded,1},address},{{p.address,p.bytes,1},t.positions},
            {{b.attributes,c[9],1},b.attributes},t.indexStride,t.indexCount,t.vertexCount,static_cast<std::uint32_t>(t.positionStride),c[2],c[5],c[6],c[3],244000u/35u,1,true,true,true};
        if(PTSceneAdmission::Admit(g,out)!=PTSceneAdmission::Error::none){out={};return Status::invalidGeometry;}
        // Preserve native tangent/color and declared attribute bounds. Admission
        // proves the needed accesses fit; it need not shrink the native view.
        out.constants[4]=c[4];out.constants[7]=c[7];out.constants[9]=c[9];
        return Status::candidate;
    }
};
}
