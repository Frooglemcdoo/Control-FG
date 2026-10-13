#pragma once
#include "pt_scene_dispatch_plan.h"
#include "pt_native_job_route.h"
// Hash-locked d3d writer 33a40: CPU identifiers at state+20 + 8*(2*type+ray),
// followed by the 56 local-root bytes from binding+8. Never map the native SBT.
namespace PTNativeShaderTable {
using U64=std::uint64_t;
using Identifier=PTShaderTableAdapter::Identifier;
struct Identifiers {U64 pipeline=0;std::array<Identifier,4> hit{};};
using Layout=PTNativeJobRoute::TableLayout;
inline bool HitRange(const Layout& l,std::size_t bindings,U64& address,U64& bytes){
    address=bytes=0;
    // Only the recovered two-ray reflection layout is admitted. The native
    // one-ray writer still reserves two rows; do not compact those row numbers.
    const U64 offset=U64(l.missOffset)+l.hitRelative;
    const U64 size=U64(l.hitRecords)*l.hitStride;
    if(!l.buffer||l.buffer%64||!bindings||bindings>65536||l.raygenBytes!=32||l.missOffset<32||l.missOffset%64||
       l.missStride!=32||l.hitRelative<64||offset%64||l.hitStride!=96||l.hitRecords!=bindings*2||
       offset>l.bytes||size>l.bytes-offset||offset>UINT64_MAX-l.buffer||size>UINT64_MAX-(l.buffer+offset))return false;
    address=l.buffer+offset;bytes=size;return true;
}
inline bool Build(const Layout& layout,const Identifiers& identifiers,U64 version,unsigned rays,
                  const std::vector<PTNativeBinding::Binding>& bindings,const PTOwnedScene::Index& owners,
                  PTSceneDispatch::Table& output,const char** failure=nullptr,std::size_t* failedRow=nullptr){
    output={};U64 address=0,bytes=0;
    if(failure)*failure="writer_layout";if(failedRow)*failedRow=0;
    if(rays!=2||!version||!identifiers.pipeline||!HitRange(layout,bindings.size(),address,bytes))return false;
    PTSceneDispatch::Table pending;pending.address=address;pending.version=version;pending.pipeline=identifiers.pipeline;pending.rays=rays;
    pending.bindings=bindings;pending.bytes.resize(static_cast<std::size_t>(bytes));pending.allocationTokens.reserve(bindings.size());
    for(std::size_t i=0;i<bindings.size();++i){
        if(failedRow)*failedRow=i;
        if(failure)*failure="writer_binding_type";
        const auto& b=bindings[i];if(b.type>1)return false;
        // Capture the logical native allocation; DWORD padding belongs to the private copy.
        const U64 index=owners.Find(b.indices,b.constants[8]),attributes=owners.Find(b.attributes,b.constants[9]);
        // Freeze missing identities as zero. The writer includes rows outside
        // the selected TLAS; only scene preflight can prove reachability. Never
        // replace a zero with a later allocation at the same GPU address.
        pending.allocationTokens.push_back({index,attributes});
        for(unsigned ray=0;ray<2;++ray){
            if(failure)*failure="writer_identifier_zero";
            const auto& id=identifiers.hit[2*b.type+ray];if(id==Identifier{})return false;
            auto* record=pending.bytes.data()+(i*2+ray)*96;
            std::memcpy(record,id.data(),32);std::memcpy(record+32,reinterpret_cast<const unsigned char*>(&b)+8,56);
        }
    }
    output=std::move(pending);if(failure)*failure="none";return true;
}
#ifdef _WIN32
// Call only while Control's native writer owns these pointers. Hash gating and
// native zero-identifier debug mode must be checked by the live host.
inline bool ReadIdentifiers(const void* state,Identifiers& output)noexcept {
    output={};Identifiers pending{};
    __try {
        if(!state)return false;
        auto* bytes=static_cast<const unsigned char*>(state);
        std::memcpy(&pending.pipeline,bytes,8);if(!pending.pipeline)return false;
        for(unsigned i=0;i<4;++i){const void* id=nullptr;std::memcpy(&id,bytes+0x20+i*8,8);
            if(!id)return false;std::memcpy(pending.hit[i].data(),id,32);}
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    output=pending;return true;
}
#endif
}
