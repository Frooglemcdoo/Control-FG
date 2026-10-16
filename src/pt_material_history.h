#pragma once
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdint>
#include <cstring>
namespace PTMaterialHistory {
constexpr unsigned Count=244000u/35u;
using Keys=std::array<std::uint64_t,Count>;
// Canonicalize the complete set of geometry bindings using each material slot.
// Ignore ray export and the transient slot itself; include all geometry addresses
// and local-root bounds. Membership/address changes conservatively reset history.
// These fingerprints are not object-motion vectors or texture-content versions.
template<class Records> Keys GeometryKeys(const Records& records){
    using Geometry=std::array<unsigned char,72>;
    std::map<unsigned,std::vector<Geometry>> groups;
    for(const auto& record:records){
        if(record.size()<104)continue;
        unsigned material=0;std::memcpy(&material,record.data()+68,4);
        if(material>=Count)continue;
        Geometry g{};std::memcpy(g.data(),record.data()+32,g.size());
        std::fill(g.begin()+36,g.begin()+40,static_cast<unsigned char>(0));groups[material].push_back(g);
    }
    Keys keys{};
    for(auto& entry:groups){auto& group=entry.second;
        std::sort(group.begin(),group.end());group.erase(std::unique(group.begin(),group.end()),group.end());
        std::uint64_t hash=14695981039346656037ull;
        for(const auto& geometry:group)for(auto byte:geometry){hash^=byte;hash*=1099511628211ull;}
        keys[entry.first]=hash?hash:1;
    }
    // Two simultaneous slots with indistinguishable geometry sets may bind
    // different textures. Do not merge them merely because constants match.
    std::map<std::uint64_t,unsigned> owners;
    for(unsigned m=0;m<Count;++m)if(keys[m]){
        const auto inserted=owners.emplace(keys[m],m);
        if(!inserted.second){keys[inserted.first->second]=0;keys[m]=0;}
    }
    return keys;
}
}
