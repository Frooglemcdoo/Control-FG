#pragma once
// Included within control_rr_albedo, after Access/readAt and EyeVariantEvidence.
// Exact renderer getShader (RVA 0x1DD3F0): table +0x238, count +0x240,
// inline shader records of stride 0xA8, key +4. Never invent a lookup key.
inline bool SelectEyeVariant(const Access& a, std::uintptr_t technique,
                             std::uintptr_t original, std::uintptr_t& selected,
                             EyeVariantEvidence* evidence=nullptr) {
    selected=0;EyeVariantEvidence local{};
    auto& e=evidence?*evidence:local;e={};
    if(!a.read || !a.getShader || !a.validateEyeCandidate || !original) return false;
    std::uintptr_t table=0;std::uint32_t count=0,mask=0,clear=0;
    if(!readAt(a,technique,0x238,table) || !table || !readAt(a,technique,0x240,count) || count!=4 ||
       !readAt(a,technique,4,mask) || mask!=0x02000000u || !readAt(a,technique,8,clear) || clear!=0 ||
       table>(std::numeric_limits<std::uintptr_t>::max)()-4u*0xA8u) return false;
    for(std::uint32_t i=0;i<4;++i) {
        const auto candidate=table+i*0xA8u;std::uintptr_t lookup=0;
        if(!readAt(a,candidate,4,e.keys[i]) || (i && e.keys[i]<=e.keys[i-1]) ||
           CanonicalTechniqueKey(mask,clear,e.keys[i])!=e.keys[i] ||
           !a.getShader(technique,e.keys[i],&lookup) || lookup!=candidate) return false;
    }
    std::uintptr_t unique=0;
    std::array<std::array<std::uintptr_t,7>,4> stagePointers{};
    for(std::uint32_t i=0;i<4;++i) {
        const auto candidate=table+i*0xA8u;bool stages=true;
        for(std::uint32_t stage=0;stage<7;++stage) {
            auto& pointer=stagePointers[i][stage];
            if(!readAt(a,candidate,0x10u+stage*0x10u,pointer)) return false;
            if((stage==0 || stage==4) ? !pointer : pointer!=0) stages=false;
        }
        if(stages && a.validateEyeCandidate(a.candidateContext,original,candidate)) {
            e.matches[i]=true;++e.matchCount;unique=candidate;
        }
    }
    // A callback may invoke native code. Recheck every table key and header
    // before accepting the result; no partial inspection authorizes a draw.
    std::uintptr_t again=0;std::uint32_t n=0,againMask=0,againClear=0;
    if(!readAt(a,technique,0x238,again) || again!=table ||
       !readAt(a,technique,0x240,n) || n!=4 || !readAt(a,technique,4,againMask) || againMask!=mask ||
       !readAt(a,technique,8,againClear) || againClear!=clear) return false;
    for(std::uint32_t i=0;i<4;++i) {
        std::uint32_t key=0;
        if(!readAt(a,table+i*0xA8u,4,key) || key!=e.keys[i]) return false;
        for(std::uint32_t stage=0;stage<7;++stage) {
            std::uintptr_t pointer=0;
            if(!readAt(a,table+i*0xA8u,0x10u+stage*0x10u,pointer) || pointer!=stagePointers[i][stage]) return false;
        }
    }
    e.tableValid=true;
    if(e.matchCount!=1) return false;
    selected=unique;return true;
}
