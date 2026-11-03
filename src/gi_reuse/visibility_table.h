#ifndef CONTROL_GI_VISIBILITY_TABLE_H
#define CONTROL_GI_VISIBILITY_TABLE_H
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
namespace gi_reuse {
struct VisibilityIdentifierPair {
    std::array<unsigned char,32> native{}, replacement{};
};
// Only identifiers change. Row numbers, stride, local-root arguments and inactive
// records retain their exact bytes. Active rows must be known in the SAME state.
inline bool RemapVisibilityTable(const unsigned char* source,std::size_t bytes,
    unsigned stride,const std::vector<unsigned char>& active,
    const std::vector<VisibilityIdentifierPair>& identifiers,
    std::vector<unsigned char>& output) {
    if(!source||!bytes||bytes>32u*1024u*1024u||stride<32||stride>4096||stride%32||bytes%stride||
       bytes/stride>131072||active.size()!=bytes/stride||
       identifiers.empty()||identifiers.size()>4096) return false;
    const std::array<unsigned char,32> zero{};
    std::vector<const VisibilityIdentifierPair*> byNative,byReplacement;
    byNative.reserve(identifiers.size());byReplacement.reserve(identifiers.size());
    for(const auto& pair:identifiers) {
        if(pair.native==zero||pair.replacement==zero) return false;
        byNative.push_back(&pair);byReplacement.push_back(&pair);
    }
    std::sort(byNative.begin(),byNative.end(),[](const auto* a,const auto* b){return a->native<b->native;});
    std::sort(byReplacement.begin(),byReplacement.end(),[](const auto* a,const auto* b){return a->replacement<b->replacement;});
    for(std::size_t i=1;i<identifiers.size();++i)
        if(byNative[i-1]->native==byNative[i]->native||byReplacement[i-1]->replacement==byReplacement[i]->replacement) return false;
    // Make a private copy before publication, including when source aliases output.
    std::vector<unsigned char> next(source,source+bytes);
    bool any=false;
    for(std::size_t i=0;i<active.size();++i) {
        if(active[i]>1) return false;
        if(!active[i]) continue;
        any=true;const auto* from=source+i*stride;
        const auto match=std::lower_bound(byNative.begin(),byNative.end(),from,
            [](const auto* pair,const unsigned char* id){return std::memcmp(pair->native.data(),id,32)<0;});
        if(match==byNative.end()||std::memcmp((*match)->native.data(),from,32)) return false;
        std::memcpy(next.data()+i*stride,(*match)->replacement.data(),32);
    }
    if(!any) return false;
    output.swap(next);return true;
}
}
#endif
