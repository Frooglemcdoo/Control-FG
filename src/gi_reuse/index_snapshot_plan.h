#pragma once
#include <cstdint>
namespace gi_reuse {
// Exact uint16 input reads. Only the owned destination is DWORD padded.
// 2-D dispatch removes the old single-axis 16 MiB dispatch ceiling. A separate
// bounded allocation policy still limits total retained snapshot memory.
struct IndexSnapshotPlan {
    std::uint64_t sourceOffset=0;
    std::uint32_t prefixBytes=0,elements=0,copyBytes=0,words=0,groupsX=0,groupsY=0;
    static constexpr std::uint64_t MaxBytes=256ull*1024*1024;
    static bool Make(std::uint64_t ownerBytes,std::uint64_t offset,std::uint64_t bytes,IndexSnapshotPlan& out) noexcept {
        out={};
        if(!bytes || (offset&1) || (bytes&1) || offset>ownerBytes || bytes>ownerBytes-offset || bytes>MaxBytes)return false;
        const auto words=static_cast<std::uint32_t>((bytes+3)/4);
        const auto groups=(words+63)/64;
        const auto x=groups<1024?groups:1024u;
        out={offset&~std::uint64_t(3),static_cast<std::uint32_t>(offset&3),
             static_cast<std::uint32_t>(bytes/2),words*4,words,x,(groups+x-1)/x};
        return true;
    }
};
}
