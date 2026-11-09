#pragma once
// Diagnostic bytes only. This store never changes shader admission.
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <limits>

namespace RRVertexEvidence {
inline constexpr size_t Missing = (std::numeric_limits<size_t>::max)();
inline constexpr size_t MaxPairs = 32, MaxBytes = 2u*1024u*1024u, MaxReferences = 256;
struct Pair { std::vector<uint8_t> original, replacement, pixel; };
struct Reference {
    uint32_t sourceIndex, familyKind, originalKey, replacementKey;
    bool keysReadable;
    size_t pairIndex;
};
inline void Hex(std::ostream& out, const std::vector<uint8_t>& bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    out << '"';
    for (auto b:bytes) { out.put(digits[b>>4u]); out.put(digits[b&15u]); }
    out << '"';
}
class Store {
    std::vector<Pair> pairs_;
    std::vector<Reference> references_;
    size_t bytes_=0, omittedPairs_=0, omittedReferences_=0;
public:
    size_t Add(const std::vector<uint8_t>& original, const std::vector<uint8_t>& replacement,
               const std::vector<uint8_t>& pixel) noexcept {
        for (size_t i=0;i<pairs_.size();++i) {
            const auto& p=pairs_[i];
            if(p.original==original && p.replacement==replacement && p.pixel==pixel) return i;
        }
        const bool bounded=!original.empty() && !replacement.empty() && !pixel.empty() &&
            original.size()<=262144 && replacement.size()<=262144 && pixel.size()<=262144;
        const size_t count=bounded?original.size()+replacement.size()+pixel.size():MaxBytes+1;
        if(!bounded || pairs_.size()>=MaxPairs || count>MaxBytes-bytes_) {++omittedPairs_;return Missing;}
        try {pairs_.push_back({original,replacement,pixel});bytes_+=count;return pairs_.size()-1;}
        catch(...) {++omittedPairs_;return Missing;}
    }
    void AddReference(Reference ref) noexcept {
        if(references_.size()>=MaxReferences) {++omittedReferences_;return;}
        if(ref.pairIndex>=pairs_.size()) ref.pairIndex=Missing;
        try {references_.push_back(ref);} catch(...) {++omittedReferences_;}
    }
    size_t PairCount() const noexcept {return pairs_.size();}
    std::string Json() const {
        std::ostringstream o;
        o << "{\"schema\":\"ControlFG.RejectedVertexPairs.v1\",\"encoding\":\"hex\",\"raw_bytes\":" << bytes_
          << ",\"omitted_pair_attempts\":" << omittedPairs_ << ",\"omitted_batch_references\":" << omittedReferences_
          << ",\"pairs\":[";
        for(size_t i=0;i<pairs_.size();++i) {
            if(i) o << ',';
            const auto& p=pairs_[i];
            o << "{\"index\":" << i << ",\"original_vs\":";Hex(o,p.original);
            o << ",\"replacement_vs\":";Hex(o,p.replacement);
            o << ",\"replacement_ps\":";Hex(o,p.pixel);o << '}';
        }
        o << "],\"batches\":[";
        for(size_t i=0;i<references_.size();++i) {
            if(i) o << ',';
            const auto& r=references_[i];
            o << "{\"source_index\":" << r.sourceIndex << ",\"family_kind\":" << r.familyKind
              << ",\"keys_readable\":" << (r.keysReadable?"true":"false")
              << ",\"original_key\":" << r.originalKey << ",\"replacement_key\":" << r.replacementKey
              << ",\"pair_index\":";
            if(r.pairIndex==Missing) o << "null";else o << r.pairIndex;
            o << '}';
        }
        o << "]}";return o.str();
    }
};
}
