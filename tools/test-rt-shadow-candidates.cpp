#include "../src/rt_shadow_capture_candidates.h"
#include <cassert>
#include <cstdio>
#include <string>
int main() {
    using namespace native_lighting::rt_shadow_capture;
    unsigned mask = 0;
    for (unsigned i = 0; i < kComputeCandidateCount; ++i) {
        const auto& candidate = kComputeCandidates[i];
        assert(IsCandidateSize(candidate.bytes));
        assert(ComputeCandidateBit(candidate.bytes, candidate.sha) == (1u << i));
        assert(ComputeCandidateBit(candidate.bytes + 1, candidate.sha) == 0);
        std::string changed(candidate.sha);
        changed[0] = changed[0] == '0' ? '1' : '0';
        assert(ComputeCandidateBit(candidate.bytes, changed.c_str()) == 0);
        assert(ComputeCandidateBit(candidate.bytes, nullptr) == 0);
        mask |= ComputeCandidateBit(candidate.bytes, candidate.sha);
    }
    assert(mask == kExpectedComputeMask);
    assert(!IsCandidateSize(0));
    // These two shaders have equal lengths but distinct hashes and mask bits.
    assert(kComputeCandidates[7].bytes == kComputeCandidates[8].bytes);
    assert(ComputeCandidateBit(kComputeCandidates[7].bytes, kComputeCandidates[7].sha) !=
           ComputeCandidateBit(kComputeCandidates[8].bytes, kComputeCandidates[8].sha));
    puts("PASS: exact shader identity, equal-size variants, rejection and complete mask");
}
