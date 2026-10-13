#pragma once
#include <cstddef>
#include <cstring>
namespace PTPrimaryProfiles {
struct Profile { std::size_t bytes; const char* sha256; const char* name; };
// Both captures share all consumed SRVs, samplers and constant layouts.
// The older variant also writes g_rwtSingleSpot (u2); PT never consumes it.
inline constexpr Profile profiles[]={
    {71144,"a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f","single_spot_output"},
    {69704,"bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff","standard_output"}
};
inline bool CandidateSize(std::size_t bytes){
    for(const auto& p:profiles)if(p.bytes==bytes)return true;
    return false;
}
inline const Profile* Find(std::size_t bytes,const char* sha){
    if(!sha)return nullptr;
    for(const auto& p:profiles)if(p.bytes==bytes&&!std::strcmp(p.sha256,sha))return &p;
    return nullptr;
}
}
