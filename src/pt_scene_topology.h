#pragma once
#include "pt_native_binding.h"
#include <map>
#include <vector>
#include <cstdint>
#include <memory>
namespace PTSceneTopology {
using U64=std::uint64_t;
struct Instance {float transform[12];std::uint32_t idMask,contributionFlags;U64 blas;};
static_assert(sizeof(Instance)==64,"DXR instance ABI");
struct FrozenGeometry {
    U64 indexBase=0,indexBytes=0,positionBase=0,positionBytes=0;
    U64 privateIndex=0,privatePosition=0;
    U64 indexCapacity=0; // Private raw-load capacity; zero means legacy indexBytes.
    U64 indexCopyBase=0,indexCopyBytes=0; // Nonzero base: bounded index slice, original bounds remain above.
    std::shared_ptr<void> owner;
};
struct Geometry {U64 indices,positions,stride;std::uint32_t indexStride,indexCount,vertexCount;U64 indexToken=0,positionToken=0;std::shared_ptr<const FrozenGeometry> frozen{};};
struct Result {U64 instances=0,matched=0,missingBlas=0,outOfRange=0,mismatch=0,conflict=0,masked=0;};
struct Mapping {Geometry geometry{};bool referenced=false;};
// CPU relation checks only. A caller must separately prove submission ordering,
// allocation identity, resource states and vertex/attribute correspondence.
class Index {
    std::map<U64,std::shared_ptr<const std::vector<Geometry>>> blas;std::size_t rows=0;
public:
    bool References(U64 address,const std::vector<Geometry>* geometry)const {
        const auto found=blas.find(address);
        return found!=blas.end()&&found->second.get()==geometry;
    }
    std::size_t BottomCount()const{return blas.size();}
    bool Same(const Index& other)const {
        if(rows!=other.rows||blas.size()!=other.blas.size())return false;
        auto b=other.blas.begin();
        for(const auto& a:blas){
            if(a.first!=b->first||a.second->size()!=b->second->size())return false;
            if(a.second==b->second){++b;continue;}
            for(std::size_t n=0;n<a.second->size();++n){const auto& x=(*a.second)[n];const auto& y=(*b->second)[n];
                if(x.indices!=y.indices||x.positions!=y.positions||x.stride!=y.stride||x.indexStride!=y.indexStride||
                   x.indexCount!=y.indexCount||x.vertexCount!=y.vertexCount||x.indexToken!=y.indexToken||
                   x.positionToken!=y.positionToken||x.frozen!=y.frozen)return false;
            }
            ++b;
        }
        return true;
    }
    // Shared input must be immutable for its entire lifetime. Catalog snapshots
    // meet this contract; Build retains the value-copy API for mutable callers.
    bool BuildShared(U64 address,std::shared_ptr<const std::vector<Geometry>> geometry){
        auto old=blas.find(address);const auto previous=old==blas.end()?0:old->second->size();
        if(!address||!geometry||geometry->empty()||geometry->size()>4096||(blas.size()>=16384&&old==blas.end())||geometry->size()>131072-(rows-previous)){
            if(old!=blas.end()){rows-=previous;blas.erase(old);}return false;
        }
        rows=rows-previous+geometry->size();blas[address]=std::move(geometry);return true;
    }
    bool Build(U64 address,std::vector<Geometry> geometry){
        return BuildShared(address,std::make_shared<const std::vector<Geometry>>(std::move(geometry)));
    }
    bool Compact(U64 destination,U64 source){
        auto i=blas.find(source);if(i==blas.end()){Invalidate(destination,1);return false;}
        return BuildShared(destination,i->second);
    }
    void Invalidate(U64 address,U64 bytes){
        for(auto i=blas.lower_bound(address);i!=blas.end()&&i->first-address<bytes;){rows-=i->second->size();i=blas.erase(i);}
    }
    void Clear(){blas.clear();rows=0;}
    // Borrowed geometry remains valid while this immutable index is retained.
    // Preparation consumes these pointers synchronously and never stores them.
    Result CheckReferences(const std::vector<Instance>& instances,const std::vector<PTNativeBinding::Binding>& bindings,unsigned rays,
                 std::vector<const Geometry*>* mapping=nullptr)const {
        if(mapping)mapping->clear();
        Result out{};std::vector<const Geometry*> assigned(bindings.size(),nullptr);
        for(const auto& instance:instances){
            ++out.instances;if(!(instance.idMask>>24)){++out.masked;continue;}
            auto i=blas.find(instance.blas);if(i==blas.end()){++out.missingBlas;continue;}
            const U64 contribution=instance.contributionFlags&0xffffffu;
            if(!rays||contribution%rays){++out.outOfRange;continue;}
            for(std::size_t g=0;g<i->second->size();++g){
                const U64 row=contribution/rays+g; // TraceRay multiplier is rays.
                if(row>=bindings.size()){++out.outOfRange;continue;}
                const auto& geometry=(*i->second)[g];const auto& b=bindings[static_cast<std::size_t>(row)];U64 address=0;
                if(!PTNativeBinding::IndexAddress(b,address)||address!=geometry.indices||b.constants[1]!=geometry.indexStride||
                   b.constants[0]>=b.constants[8]||U64(geometry.indexCount)*geometry.indexStride>U64(b.constants[8])-b.constants[0]){++out.mismatch;continue;}
                const auto& old=assigned[static_cast<std::size_t>(row)];
                if(old&&(old->positions!=geometry.positions||old->stride!=geometry.stride||old->indexCount!=geometry.indexCount||old->vertexCount!=geometry.vertexCount||old->indexToken!=geometry.indexToken||old->positionToken!=geometry.positionToken||old->frozen!=geometry.frozen)){++out.conflict;continue;}
                assigned[static_cast<std::size_t>(row)]=&geometry;++out.matched;
            }
        }
        if(mapping&&out.matched&&!out.missingBlas&&!out.outOfRange&&!out.mismatch&&!out.conflict){
            *mapping=std::move(assigned);
        }
        return out;
    }
    Result Check(const std::vector<Instance>& instances,const std::vector<PTNativeBinding::Binding>& bindings,unsigned rays,
                 std::vector<Mapping>* mapping=nullptr)const {
        if(mapping)mapping->clear();
        std::vector<const Geometry*> references;
        const auto result=CheckReferences(instances,bindings,rays,mapping?&references:nullptr);
        if(mapping&&!references.empty()){
            mapping->resize(references.size());
            for(std::size_t n=0;n<references.size();++n)if(references[n])(*mapping)[n]={*references[n],true};
        }
        return result;
    }
};
}
