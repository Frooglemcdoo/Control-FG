#pragma once
#include <cstddef>
#include <cstring>

namespace native_lighting::rt_shadow_capture {
// Compute shaders first observed after RT was enabled in the R64 session.
// Their roles remain unverified until the bytecode is decoded. In particular,
// size alone does not establish which shader composes direct RT lighting.
struct ComputeCandidate {
    size_t bytes;
    const char* sha;
};
inline constexpr ComputeCandidate kComputeCandidates[] = {
    {7932, "c89bf47249badf0a983c2ffcac675db1f52b41c8bd4bca9781fd0e3a3e4fd686"},
    {10036, "acdf0bc33c0c9b4e7b14ef8d2c1854cfb1d404fbbe4363e73c3b0d0e140e6dbd"},
    {3784, "9cb31a96edff2d5b54c3e10b2cebeb777c822eda42a1b0552f58caf4dae6d1fe"},
    {3848, "017bd2265499cb427b87867f411b6bbadb29d12b6e2c5da94a567b85e72dd255"},
    {6516, "e9105d227afac709e8252d12a831822ed641d857bfc7dc249ea5cb402605dfa4"},
    {71144, "a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f"},
    {4868, "feb37637481fed22a3cd4d256da613d1f61aec70fc11ff5e42aa2a585101fedb"},
    {12360, "584d4085443970868fe903d10bcdacb18c4c82490d08b742b5219648da437ab3"},
    {12360, "e1a78e01b31559354cd8d474c22d117c444dd008a0067881d44836a23b0a459b"},
    {50752, "6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621"},
    {87560, "32047fd063b5edd85ccf13f4afcffe62763c8f601bdef53c26074e64f939e6a6"},
};
inline constexpr unsigned kComputeCandidateCount =
    sizeof(kComputeCandidates) / sizeof(kComputeCandidates[0]);
static_assert(kComputeCandidateCount < 32, "Capture masks use 32 bits");
inline constexpr unsigned kExpectedComputeMask = (1u << kComputeCandidateCount) - 1;
inline bool IsCandidateSize(size_t bytes) noexcept {
    for (const auto& candidate : kComputeCandidates)
        if (candidate.bytes == bytes) return true;
    return false;
}
inline unsigned ComputeCandidateBit(size_t bytes, const char* sha) noexcept {
    if (!sha) return 0;
    for (unsigned i = 0; i < kComputeCandidateCount; ++i)
        if (kComputeCandidates[i].bytes == bytes &&
            std::strcmp(kComputeCandidates[i].sha, sha) == 0) return 1u << i;
    return 0;
}
} // namespace native_lighting::rt_shadow_capture
