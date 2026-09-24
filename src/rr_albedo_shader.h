#pragma once
// Bounded inspection of Control's selected native albedo shaders.
// This validates a diagnostic MRT contract, not diffuse/specular color semantics.
// Token definitions: Microsoft's d3d12TokenizedProgramFormat.hpp.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>
#include "rr_vertex_evidence.h"
#if defined(_WIN32)
#include <windows.h>
#endif

namespace RRAlbedoShader {
inline constexpr size_t kMaximumBlobBytes = 256u * 1024u;
inline constexpr size_t kMaximumCopiedBytes = 16u * 1024u * 1024u;
inline constexpr size_t kMaximumPairs = 128u;
struct Contract {
    uint32_t targetCount = 0;
    std::array<uint8_t, 2> componentMasks{};
    std::array<uint8_t, 2> neverWritesMasks{};
    uint32_t instructionCount = 0;
    uint32_t resourceCount = 0;
    uint32_t shaderMajor = 0;
    uint32_t shaderMinor = 0;
};
inline bool Reject(const char** reason, const char* message) noexcept {
    if (reason) *reason = message;
    return false;
}
inline uint32_t Word(const uint8_t* p) noexcept {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8u) |
           (uint32_t(p[2]) << 16u) | (uint32_t(p[3]) << 24u);
}
inline constexpr uint32_t Tag(char a, char b, char c, char d) noexcept {
    return uint32_t(uint8_t(a)) | (uint32_t(uint8_t(b)) << 8u) |
           (uint32_t(uint8_t(c)) << 16u) | (uint32_t(uint8_t(d)) << 24u);
}
inline bool Range(size_t offset, size_t count, size_t extent) noexcept {
    return offset <= extent && count <= extent - offset;
}
inline bool Table(size_t offset, size_t count, size_t stride, size_t extent) noexcept {
    return offset <= extent && stride && count <= (extent - offset) / stride;
}
inline bool StringAt(const uint8_t* data, size_t size, size_t offset,
                     const char* expected = nullptr) noexcept {
    if (offset >= size) return false;
    const size_t available = (std::min)(size - offset, size_t(256));
    for (size_t i = 0; i < available; ++i) {
        const unsigned char c = data[offset + i];
        if (expected) {
            const unsigned char want = static_cast<unsigned char>(expected[i]);
            const unsigned char lower = c >= 'A' && c <= 'Z' ? static_cast<unsigned char>(c + 32) : c;
            const unsigned char lowerWant = want >= 'A' && want <= 'Z' ? static_cast<unsigned char>(want + 32) : want;
            if (lower != lowerWant) return false;
        }
        if (c == 0) return i != 0;
        if (c < 32 || c > 126) return false;
    }
    return false;
}
struct Chunk { uint32_t tag = 0; const uint8_t* data = nullptr; size_t size = 0; };
struct Container {
    Chunk code{}, output{}, resources{};
    uint32_t major = 0, minor = 0, stage = 0;
};
inline bool ReadContainer(const uint8_t* data, size_t size, Container& out,
                          const char** reason) noexcept {
    out = {};
    if (!data || size < 32 || size > kMaximumBlobBytes || (size & 3u))
        return Reject(reason, "dxbc_size");
    if (Word(data) != Tag('D','X','B','C') || Word(data + 20) != 1 || Word(data + 24) != size)
        return Reject(reason, "dxbc_header");
    const uint32_t count = Word(data + 28);
    if (count < 3 || count > 32 || !Table(32, count, 4, size))
        return Reject(reason, "dxbc_chunk_table");
    std::array<std::array<size_t, 2>, 32> spans{};
    bool inputSeen = false;
    for (uint32_t n = 0; n < count; ++n) {
        const size_t offset = Word(data + 32 + n * 4u);
        if ((offset & 3u) || offset < 32u + count * 4u || !Range(offset, 8, size))
            return Reject(reason, "dxbc_chunk_offset");
        const size_t bytes = Word(data + offset + 4);
        if (!Range(offset + 8, bytes, size)) return Reject(reason, "dxbc_chunk_size");
        spans[n] = {offset, offset + 8 + bytes};
        for (uint32_t j = 0; j < n; ++j)
            if (spans[n][0] < spans[j][1] && spans[j][0] < spans[n][1])
                return Reject(reason, "dxbc_chunk_overlap");
        const Chunk chunk{Word(data + offset), data + offset + 8, bytes};
        switch (chunk.tag) {
        case Tag('S','H','D','R'): case Tag('S','H','E','X'):
            if (out.code.data) return Reject(reason, "dxbc_duplicate_code");
            out.code = chunk; break;
        case Tag('O','S','G','N'): case Tag('O','S','G','5'): case Tag('O','S','G','1'):
            if (out.output.data) return Reject(reason, "dxbc_duplicate_output");
            out.output = chunk; break;
        case Tag('R','D','E','F'):
            if (out.resources.data) return Reject(reason, "dxbc_duplicate_rdef");
            out.resources = chunk; break;
        case Tag('I','S','G','N'): case Tag('I','S','G','1'):
            if (inputSeen) return Reject(reason, "dxbc_duplicate_input");
            inputSeen = true; break;
        case Tag('S','T','A','T'): case Tag('S','D','B','G'):
        case Tag('S','P','D','B'): case Tag('I','L','D','N'):
            break; // Bounded debug/statistics payloads do not execute.
        default: return Reject(reason, "dxbc_unsupported_chunk");
        }
    }
    if (!out.code.data || !out.output.data || !out.resources.data ||
        out.code.size < 12 || (out.code.size & 3u))
        return Reject(reason, "dxbc_required_chunks");
    const uint32_t version = Word(out.code.data);
    out.stage = version >> 16u;
    out.major = (version >> 4u) & 15u;
    out.minor = version & 15u;
    if ((version & 0xff00u) ||
        !((out.major == 4 && out.minor <= 1) || (out.major == 5 && out.minor <= 1)))
        return Reject(reason, "dxbc_unsupported_model");
    if (Word(out.code.data + 4) != out.code.size / 4)
        return Reject(reason, "dxbc_program_length");
    return true;
}
inline bool ReadOutputs(const Container& c, bool pixel, Contract& out,
                        const char** reason) noexcept {
    const Chunk& s = c.output;
    if (s.size < 8) return Reject(reason, "dxbc_signature_header");
    const size_t count = Word(s.data), offset = Word(s.data + 4);
    const bool stream = s.tag != Tag('O','S','G','N');
    const size_t stride = s.tag == Tag('O','S','G','1') ? 32u : (stream ? 28u : 24u);
    if (!count || count > (pixel ? 2u : 32u) || offset < 8 || (offset & 3u) ||
        !Table(offset, count, stride, s.size)) return Reject(reason, "dxbc_signature_table");
    uint32_t registers = 0;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t* record = s.data + offset + i * stride;
        if (stream && Word(record) != 0) return Reject(reason, "dxbc_signature_stream");
        const uint8_t* p = record + (stream ? 4 : 0);
        const uint32_t name = Word(p), index = Word(p + 4), system = Word(p + 8);
        const uint32_t component = Word(p + 12), reg = Word(p + 16);
        const uint8_t mask = p[20], neverWrites = p[21];
        if (name < offset + count * stride || !StringAt(s.data, s.size, name) ||
            (mask & 0xf0u) || !mask || (neverWrites & 0xf0u) || p[22] || p[23])
            return Reject(reason, "dxbc_signature_record");
        if (stride == 32 && Word(record + 28) != 0)
            return Reject(reason, "dxbc_signature_precision");
        if (pixel) {
            // Legacy FXC serializes SV_Target with system-value 0; reflection
            // classifies its semantic as TARGET=64. Accept both representations.
            if (!StringAt(s.data, s.size, name, "SV_Target") || (system != 0 && system != 64) ||
                component != 3 || index != reg || reg >= count ||
                (mask & 7u) != 7u || (neverWrites & 7u) != 0)
                return Reject(reason, "dxbc_target_contract");
            if (registers & (1u << reg)) return Reject(reason, "dxbc_duplicate_target");
            registers |= 1u << reg;
            out.componentMasks[reg] = mask; out.neverWritesMasks[reg] = neverWrites;
        } else {
            if (reg >= 32 || component < 1 || component > 3)
                return Reject(reason, "dxbc_vertex_output");
        }
    }
    if (pixel) {
        if (registers != ((1u << count) - 1u)) return Reject(reason, "dxbc_noncontiguous_targets");
        out.targetCount = static_cast<uint32_t>(count);
    }
    return true;
}
struct ResourceBinding {
    uint32_t kind = 0, operandType = 0, identifier = 0, space = 0;
    uint32_t first = 0, last = 0, vectorCount = 0, structureStride = 0;
    bool declared = false;
};
struct Bindings {
    std::array<ResourceBinding, 256> entries{};
    size_t count = 0;
    bool model51 = false;
    ResourceBinding* Find(uint32_t type, uint32_t identifier) noexcept {
        for (size_t i = 0; i < count; ++i)
            if (entries[i].operandType == type && entries[i].identifier == identifier) return &entries[i];
        return nullptr;
    }
    const ResourceBinding* Find(uint32_t type, uint32_t identifier) const noexcept {
        for (size_t i = 0; i < count; ++i)
            if (entries[i].operandType == type && entries[i].identifier == identifier) return &entries[i];
        return nullptr;
    }
};
inline bool ReadResources(const Container& c, Contract& out, Bindings& bindings,
                          const char** reason) noexcept {
    bindings = {}; bindings.model51 = c.major == 5 && c.minor == 1;
    const Chunk& s = c.resources;
    if (s.size < 28) return Reject(reason, "dxbc_rdef_header");
    if (bindings.model51 && Word(s.data + 16) != ((c.stage == 0 ? 0xffff0000u : 0xfffe0000u) | 0x0501u))
        return Reject(reason, "dxbc_rdef_profile");
    const size_t cbCount = Word(s.data), cbOffset = Word(s.data + 4);
    const size_t count = Word(s.data + 8), offset = Word(s.data + 12);
    const size_t bindingStride = bindings.model51 ? 40u : 32u;
    if (cbCount > 256 || count > 256 ||
        (cbCount && (cbOffset < 28 || !Table(cbOffset, cbCount, 24, s.size))) ||
        (count && (offset < 28 || !Table(offset, count, bindingStride, s.size))))
        return Reject(reason, "dxbc_rdef_table");
    for (size_t i = 0; i < cbCount; ++i) {
        const uint8_t* p = s.data + cbOffset + i * 24;
        const size_t variables = Word(p + 4), variableOffset = Word(p + 8);
        if (!StringAt(s.data, s.size, Word(p)) || variables > 4096 ||
            (variables && !Table(variableOffset, variables, c.major == 5 ? 40u : 24u, s.size)))
            return Reject(reason, "dxbc_rdef_constants");
    }
    for (size_t i = 0; i < count; ++i) {
        const uint8_t* p = s.data + offset + i * bindingStride;
        const uint32_t kind = Word(p + 4), slot = Word(p + 20), slots = Word(p + 24);
        if (!StringAt(s.data, s.size, Word(p))) return Reject(reason, "dxbc_rdef_binding");
        if ((!bindings.model51 && (!slots || slot >= 4096 || slots > 4096 - slot)) ||
            (bindings.model51 && slots && slots - 1u > UINT32_MAX - slot))
            return Reject(reason, "dxbc_rdef_binding");
        if (!(kind == 0 || kind == 1 || kind == 2 || kind == 3 || kind == 5 || kind == 7))
            return Reject(reason, "dxbc_writable_or_unknown_resource");
        if (bindings.model51) {
            ResourceBinding b{};
            b.kind = kind; b.operandType = kind == 0 ? 8u : (kind == 3 ? 6u : 7u);
            b.first = slot; b.last = slots ? slot + slots - 1u : UINT32_MAX;
            b.space = Word(p + 32); b.identifier = Word(p + 36);
            if (kind == 5) b.structureStride = Word(p + 16);
            for (size_t j = 0; j < bindings.count; ++j) {
                const auto& previous = bindings.entries[j];
                if (previous.operandType != b.operandType) continue;
                if (previous.identifier == b.identifier) return Reject(reason, "dxbc_duplicate_binding_identifier");
                if (previous.space == b.space && b.first <= previous.last && previous.first <= b.last)
                    return Reject(reason, "dxbc_overlapping_binding_range");
            }
            bindings.entries[bindings.count++] = b;
        }
    }
    out.resourceCount = static_cast<uint32_t>(count);
    return true;
}
struct Operand {
    uint32_t type = 0, index = 0, dimensions = 0, components = 0, selection = 0;
    uint8_t mask = 0;
    bool immediateIndex = false;
    std::array<uint32_t, 3> indices{};
    uint8_t immediateIndices = 0;
};
struct ProgramReader {
    const uint8_t* bytes;
    size_t words;
    bool pixel;
    const Contract& contract;
    const Bindings& bindings;
    const char** reason;
    bool operand(size_t& pos, size_t end, Operand& out, unsigned nesting = 0,
                 bool declaration = false) const noexcept {
        if (nesting > 4 || pos >= end) return Reject(reason, "dxbc_operand_bounds");
        const uint32_t token = Word(bytes + pos++ * 4);
        const uint32_t components = token & 3u, selection = (token >> 2u) & 3u;
        out = {}; out.type = (token >> 12u) & 255u; out.dimensions = (token >> 20u) & 3u;
        out.components = components; out.selection = selection;
        out.mask = components == 1 ? 1u : (components == 2 && selection == 0 ? uint8_t((token >> 4u) & 15u) : 15u);
        if (components == 3 || (components == 2 && selection == 3))
            return Reject(reason, "dxbc_operand_components");
        if (!(out.type <= 4 || out.type == 6 || out.type == 7 || out.type == 8 ||
              out.type == 9 || out.type == 11 || out.type == 13))
            return Reject(reason, "dxbc_forbidden_operand");
        uint32_t ext = token; unsigned extensionCount = 0;
        while (ext & 0x80000000u) {
            if (pos >= end || ++extensionCount > 2) return Reject(reason, "dxbc_operand_extension");
            ext = Word(bytes + pos++ * 4);
            // SM5.1 explicitly represents a non-uniform resource index here.
            // Minimum precision and unknown extension controls remain unsupported.
            const uint32_t permitted = bindings.model51 && !declaration &&
                (out.type == 6 || out.type == 7 || out.type == 8) ? 0x00020000u : 0u;
            if ((ext & 63u) != 1 || (ext & (0x7ffffc00u & ~permitted)) || ((ext >> 6u) & 255u) > 3)
                return Reject(reason, "dxbc_operand_modifier");
        }
        if (out.type == 4) {
            const size_t immediateWords = components == 1 ? 1u : (components == 2 ? 4u : 0u);
            if (out.dimensions || !immediateWords || immediateWords > end - pos)
                return Reject(reason, "dxbc_immediate_operand");
            pos += immediateWords; return true;
        }
        uint32_t expectedDimensions = out.type == 3 || out.type == 8 ? 2u :
            (out.type == 11 || out.type == 13 ? 0u : 1u);
        if (bindings.model51) {
            if (out.type == 6 || out.type == 7) expectedDimensions = declaration ? 3u : 2u;
            if (out.type == 8) expectedDimensions = 3;
        }
        if (out.dimensions != expectedDimensions)
            return Reject(reason, "dxbc_operand_dimensions");
        for (uint32_t i = 0; i < out.dimensions; ++i) {
            const uint32_t representation = (token >> (22u + 3u * i)) & 7u;
            if (representation == 0 || representation == 3) {
                if (pos >= end) return Reject(reason, "dxbc_index_bounds");
                const uint32_t index = Word(bytes + pos++ * 4);
                out.indices[i] = index;
                if (representation == 0) out.immediateIndices |= static_cast<uint8_t>(1u << i);
                if (i == 0) { out.index = index; out.immediateIndex = representation == 0; }
            }
            if (representation == 2 || representation == 3) {
                Operand relative{};
                if (!operand(pos, end, relative, nesting + 1)) return false;
                if (!(relative.type == 0 || relative.type == 1 || relative.type == 3))
                    return Reject(reason, "dxbc_relative_index_source");
            } else if (representation != 0) return Reject(reason, "dxbc_index_representation");
        }
        if (out.type == 2 && (out.dimensions != 1 || !out.immediateIndex ||
            out.index >= (pixel ? contract.targetCount : 32u)))
            return Reject(reason, "dxbc_output_register");
        if (bindings.model51 && (out.type == 6 || out.type == 7 || out.type == 8)) {
            if (!(out.immediateIndices & 1u)) return Reject(reason, "dxbc_dynamic_binding_identifier");
            if (declaration) {
                if (out.immediateIndices != 7 || out.indices[1] > out.indices[2])
                    return Reject(reason, "dxbc_declaration_binding_range");
            } else {
                const ResourceBinding* b = bindings.Find(out.type, out.indices[0]);
                if (!b || !b->declared) return Reject(reason, "dxbc_undeclared_binding_identifier");
                if ((out.immediateIndices & 2u) && (out.indices[1] < b->first || out.indices[1] > b->last))
                    return Reject(reason, "dxbc_resource_index_range");
                if (out.type == 8 && (out.immediateIndices & 4u) && b->vectorCount && out.indices[2] >= b->vectorCount)
                    return Reject(reason, "dxbc_constant_vector_range");
            }
        }
        return true;
    }
    bool destination(const Operand& o) const noexcept {
        if (o.type != 13 && !(o.components == 1 || (o.components == 2 && o.selection == 0)))
            return Reject(reason, "dxbc_destination_components");
        if (!(o.type == 0 || o.type == 2 || o.type == 3 || o.type == 13))
            return Reject(reason, "dxbc_invalid_destination");
        if (o.type == 2 && pixel && (o.mask & ~contract.componentMasks[o.index]))
            return Reject(reason, "dxbc_output_write_mask");
        return true;
    }
};
// {operand count, destination count}; -1 rejects an unsupported instruction.
inline std::array<int, 2> InstructionOperands(uint32_t op) noexcept {
    switch (op) {
    case 2: case 7: case 10: case 18: case 21: case 22: case 23: case 48: case 58: case 62: return {0,0};
    case 3: case 6: case 8: case 13: case 31: case 63: case 76: return {1,0};
    case 11: case 12: case 25: case 26: case 27: case 28: case 40: case 43:
    case 47: case 54: case 59: case 64: case 65: case 66: case 67: case 68:
    case 75: case 86: case 111: case 121: case 122: case 123: case 124: case 125:
    case 129: case 130: case 131: case 134: case 135: case 136: case 137: case 141: case 205: return {2,1};
    case 0: case 1: case 14: case 15: case 16: case 17: case 24: case 29: case 30:
    case 32: case 33: case 34: case 36: case 37: case 39: case 41: case 42: case 45:
    case 49: case 51: case 52: case 56: case 57: case 60: case 61: case 79:
    case 80: case 83: case 84: case 85: case 87: case 110: case 165: case 203: case 204: return {3,1};
    case 35: case 46: case 50: case 55: case 69: case 82: case 108: case 109:
    case 138: case 139: case 167: return {4,1};
    case 70: case 71: case 72: case 74: case 126: case 127: case 140: return {5,1};
    case 73: case 128: return {6,1};
    case 77: return {3,2};
    case 38: case 78: case 81: case 132: case 133: return {4,2};
    case 142: return {6,2};
    default: return {-1,0};
    }
}
inline bool ReadProgram(const Container& c, bool pixel, Contract& out, Bindings& bindings,
                        const char** reason) noexcept {
    ProgramReader reader{c.code.data, c.code.size / 4, pixel, out, bindings, reason};
    size_t pos = 2; uint32_t declarations = 0, returns = 0;
    std::array<uint32_t, 64> blocks{}; size_t depth = 0;
    bool executableSeen = false;
    while (pos < reader.words) {
        const size_t start = pos;
        const uint32_t token = Word(reader.bytes + pos++ * 4), op = token & 0x7ffu;
        if (++out.instructionCount > 65536) return Reject(reason, "dxbc_instruction_cap");
        if (op == 53) {
            if (pos >= reader.words) return Reject(reason, "dxbc_custom_header");
            const uint32_t count = Word(reader.bytes + pos * 4), kind = token >> 11u;
            if (count < 2 || count > reader.words - start ||
                !(kind == 0 || kind == 1 || kind == 3) || (kind == 3 && ((count - 2) & 3u)))
                return Reject(reason, "dxbc_custom_data");
            if (kind == 3 && executableSeen) return Reject(reason, "dxbc_declaration_order");
            pos = start + count; continue;
        }
        const size_t length = (token >> 24u) & 127u;
        if (!length || length > reader.words - start) return Reject(reason, "dxbc_instruction_length");
        const size_t end = start + length;
        uint32_t extended = token; unsigned extensionCount = 0;
        while (extended & 0x80000000u) {
            if (pos >= end || ++extensionCount > 3) return Reject(reason, "dxbc_instruction_extension");
            extended = Word(reader.bytes + pos++ * 4);
            const uint32_t kind = extended & 63u;
            if (kind < 1 || kind > 3) return Reject(reason, "dxbc_unknown_instruction_extension");
        }
        if ((op >= 156 && op <= 160) || op == 163 || op == 164 || op == 166 ||
            (op >= 168 && op <= 190) || op == 225)
            return Reject(reason, "dxbc_writable_instruction");
        const bool declaration = (op >= 88 && op <= 106) || op == 161 || op == 162;
        if (declaration) {
            if (executableSeen || extensionCount) return Reject(reason, "dxbc_declaration_order");
            if (op == 104) { if (end - pos != 1 || Word(reader.bytes + pos * 4) > 4096) return Reject(reason, "dxbc_temp_count"); pos = end; }
            else if (op == 105) { if (end - pos != 3 || Word(reader.bytes + (pos + 1) * 4) > 4096 || Word(reader.bytes + (pos + 2) * 4) > 4) return Reject(reason, "dxbc_indexable_temps"); pos = end; }
            else if (op == 106) {
                const uint32_t permitted = 0x0000e800u | (bindings.model51 ? 0x00080000u : 0u);
                if (pos != end || ((token & 0x00fff800u) & ~permitted)) return Reject(reason, "dxbc_global_flags");
            }
            else {
                if (op == 92 || op == 93 || op == 94) return Reject(reason, "dxbc_geometry_declaration");
                Operand operand{};
                if (!reader.operand(pos, end, operand, 0, true)) return false;
                size_t trailing = 0;
                if (op == 88 || op == 91 || op == 96 || op == 97 || op == 99 ||
                    op == 100 || op == 102 || op == 103 || op == 162) trailing = 1;
                const bool resourceDeclaration = op == 88 || op == 89 || op == 90 || op == 161 || op == 162;
                if (bindings.model51 && resourceDeclaration) trailing = op == 90 || op == 161 ? 1u : 2u;
                if (end - pos != trailing) return Reject(reason, "dxbc_declaration_length");
                if ((op == 88 || op == 161 || op == 162) && operand.type != 7)
                    return Reject(reason, "dxbc_resource_declaration");
                if (op == 89 && operand.type != 8) return Reject(reason, "dxbc_cbuffer_declaration");
                if (op == 90 && operand.type != 6) return Reject(reason, "dxbc_sampler_declaration");
                if (op >= 95 && op <= 100 && !(operand.type == 1 || operand.type == 11))
                    return Reject(reason, "dxbc_input_declaration");
                if (bindings.model51 && resourceDeclaration) {
                    ResourceBinding* binding = bindings.Find(operand.type, operand.indices[0]);
                    const uint32_t space = Word(reader.bytes + (end - 1) * 4);
                    if (!binding || binding->declared || binding->space != space ||
                        binding->first != operand.indices[1] || binding->last != operand.indices[2])
                        return Reject(reason, "dxbc_rdef_declaration_mismatch");
                    if ((op == 88 && !(binding->kind == 1 || binding->kind == 2)) ||
                        (op == 161 && binding->kind != 7) || (op == 162 && binding->kind != 5))
                        return Reject(reason, "dxbc_rdef_declaration_type");
                    if (op == 89) {
                        binding->vectorCount = Word(reader.bytes + pos * 4);
                        if (binding->vectorCount > 4096) return Reject(reason, "dxbc_constant_buffer_size");
                    }
                    if (op == 162 && (Word(reader.bytes + pos * 4) == 0 ||
                        (Word(reader.bytes + pos * 4) & 3u) ||
                        Word(reader.bytes + pos * 4) != binding->structureStride))
                        return Reject(reason, "dxbc_structured_stride");
                    binding->declared = true;
                }
                if (op >= 101 && op <= 103) {
                    if (operand.type != 2 || !reader.destination(operand)) return Reject(reason, "dxbc_output_declaration");
                    if (pixel) {
                        if (declarations & (1u << operand.index)) return Reject(reason, "dxbc_duplicate_output_declaration");
                        declarations |= 1u << operand.index;
                        if (op != 101 || operand.mask != out.componentMasks[operand.index])
                            return Reject(reason, "dxbc_signature_declaration_mismatch");
                    }
                }
                pos = end;
            }
            continue;
        }
        const auto operands = InstructionOperands(op);
        if (operands[0] < 0) return Reject(reason, "dxbc_unsupported_instruction");
        executableSeen = true;
        if (op == 31 || op == 48 || op == 76) {
            if (depth == blocks.size()) return Reject(reason, "dxbc_control_depth");
            blocks[depth++] = op;
        } else if (op == 21 || op == 22 || op == 23) {
            const uint32_t expected = op == 21 ? 31u : (op == 22 ? 48u : 76u);
            if (!depth || (blocks[depth - 1] & 0xffu) != expected) return Reject(reason, "dxbc_control_close");
            --depth;
        } else if (op == 18) {
            if (!depth || blocks[depth - 1] != 31) return Reject(reason, "dxbc_control_else");
            blocks[depth - 1] |= 0x100u;
        } else if (op == 6 || op == 10) {
            if (!depth || blocks[depth - 1] != 76) return Reject(reason, "dxbc_control_case");
        } else if (op == 2 || op == 3 || op == 7 || op == 8) {
            bool permitted = false;
            for (size_t i = 0; i < depth; ++i)
                permitted = permitted || blocks[i] == 48 || ((op == 2 || op == 3) && blocks[i] == 76);
            if (!permitted) return Reject(reason, "dxbc_control_break");
        }
        if (op == 62 || op == 63) ++returns;
        for (int i = 0; i < operands[0]; ++i) {
            Operand operand{};
            if (!reader.operand(pos, end, operand)) return false;
            if (i < operands[1] && !reader.destination(operand)) return false;
            if (i >= operands[1] && operand.type == 13)
                return Reject(reason, "dxbc_null_source");
        }
        if (pos != end) return Reject(reason, "dxbc_operand_count");
    }
    if (depth || !returns || (pixel && declarations != ((1u << out.targetCount) - 1u)))
        return Reject(reason, "dxbc_incomplete_program");
    for (size_t i = 0; i < bindings.count; ++i)
        if (!bindings.entries[i].declared) return Reject(reason, "dxbc_missing_resource_declaration");
    return true;
}
inline bool Parse(const uint8_t* data, size_t size, bool pixel, Contract& out,
                  const char** reason = nullptr) noexcept {
    out = {}; if (reason) *reason = nullptr;
    Container c{};
    if (!ReadContainer(data, size, c, reason)) return false;
    if (c.stage != (pixel ? 0u : 1u)) return Reject(reason, "dxbc_stage");
    Contract checked{}; checked.shaderMajor = c.major; checked.shaderMinor = c.minor;
    Bindings bindings{};
    if (!ReadOutputs(c, pixel, checked, reason) || !ReadResources(c, checked, bindings, reason) ||
        !ReadProgram(c, pixel, checked, bindings, reason)) return false;
    out = checked; return true;
}
inline bool ParsePixelContract(const uint8_t* data, size_t size, Contract& out,
                               const char** reason = nullptr) noexcept {
    return Parse(data, size, true, out, reason);
}
inline bool ParseVertex(const uint8_t* data, size_t size, const char** reason = nullptr) noexcept {
    Contract ignored{}; return Parse(data, size, false, ignored, reason);
}
// Noncryptographic diagnostic identifier. Deduplication also compares every byte.
inline uint64_t HashFNV1a(const uint8_t* data, size_t size) noexcept {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) { hash ^= data[i]; hash *= UINT64_C(1099511628211); }
    return hash;
}
struct CapturedPixelShader {
    std::vector<uint8_t> bytes;
    uint64_t hashFNV1a = 0;
    Contract contract{};
};
struct Result { uint32_t targetCount = 0; size_t selectedIndex = 0; size_t vertexEvidenceIndex = RRVertexEvidence::Missing; };

#if defined(_WIN32)
using ResidentGetter = const char* (__cdecl*)(int);
inline ResidentGetter residentGetter = nullptr;
inline bool Initialize(HMODULE d3d) noexcept {
    residentGetter = nullptr;
    if (!d3d) return false;
    const auto address = GetProcAddress(d3d, "?get@ShaderCodeStorage@d3d@@YAPEBDH@Z");
    if (reinterpret_cast<uintptr_t>(address) != reinterpret_cast<uintptr_t>(d3d) + 0x40820u) return false;
    residentGetter = reinterpret_cast<ResidentGetter>(address); return true;
}
struct NativeDescriptor { uintptr_t shaderBase; uint32_t stage; int32_t identifier; uint64_t size; };
inline bool ReadDescriptor(uintptr_t shader, size_t slot, NativeDescriptor* out) noexcept {
    if (!shader || !out || shader > (std::numeric_limits<uintptr_t>::max)() - slot - 8) return false;
    __try {
        const uintptr_t base = *reinterpret_cast<const uintptr_t*>(shader + slot);
        if (!base || base > (std::numeric_limits<uintptr_t>::max)() - 24) return false;
        out->shaderBase = base;
        out->stage = *reinterpret_cast<const uint32_t*>(base + 8);
        out->identifier = *reinterpret_cast<const int32_t*>(base + 12);
        out->size = *reinterpret_cast<const uint64_t*>(base + 16);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool CopyResident(const NativeDescriptor* descriptor, uint8_t* output, size_t size) noexcept {
    if (!descriptor || !output || !residentGetter || descriptor->identifier < 0 ||
        descriptor->size != size || size < 32 || size > kMaximumBlobBytes) return false;
    __try {
        const char* p = residentGetter(descriptor->identifier);
        if (!p || reinterpret_cast<uintptr_t>(p) > (std::numeric_limits<uintptr_t>::max)() - size) return false;
        std::memcpy(output, p, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
class Session {
    struct CachedPair { uintptr_t original, albedo; Result result; bool valid; const char* reason; };
    std::vector<CachedPair> pairs_;
    std::vector<CapturedPixelShader> shaders_;
    size_t copiedBytes_ = 0;
    bool captureVertexEvidence_ = false;
    RRVertexEvidence::Store vertexEvidence_;
    bool Read(uintptr_t shader, size_t slot, uint32_t stage, std::vector<uint8_t>& bytes,
              const char** reason) {
        NativeDescriptor before{}, after{};
        if (!ReadDescriptor(shader, slot, &before) || before.stage != stage ||
            before.identifier < 0 || before.size < 32 || before.size > kMaximumBlobBytes)
            return Reject(reason, "native_shader_descriptor");
        const size_t size = static_cast<size_t>(before.size);
        if (size > kMaximumCopiedBytes - copiedBytes_) return Reject(reason, "native_shader_copy_budget");
        bytes.resize(size); copiedBytes_ += size;
        if (!CopyResident(&before, bytes.data(), size) || !ReadDescriptor(shader, slot, &after) ||
            before.shaderBase != after.shaderBase || before.stage != after.stage ||
            before.identifier != after.identifier || before.size != after.size)
            return Reject(reason, "native_shader_changed_or_faulted");
        return true;
    }
public:
    explicit Session(bool captureVertexEvidence=false) noexcept : captureVertexEvidence_(captureVertexEvidence) {}
    RRVertexEvidence::Store& VertexEvidence() noexcept { return vertexEvidence_; }
    const std::vector<CapturedPixelShader>& PixelShaders() const noexcept { return shaders_; }
    size_t CopiedBytes() const noexcept { return copiedBytes_; }
    size_t PairCount() const noexcept { return pairs_.size(); }
    bool ValidatePair(uintptr_t originalShader, uintptr_t albedoShader, Result& out,
                      const char** reason = nullptr) noexcept {
        out = {}; if (reason) *reason = nullptr;
        if (!residentGetter) return Reject(reason, "native_shader_getter_unavailable");
        for (const auto& p : pairs_) if (p.original == originalShader && p.albedo == albedoShader) {
            out = p.result;
            return p.valid ? true : Reject(reason, p.reason);
        }
        if (pairs_.size() >= kMaximumPairs) return Reject(reason, "native_shader_pair_cap");
        const size_t pairIndex = pairs_.size();
        try {
            pairs_.push_back({originalShader, albedoShader, {}, false, "native_shader_validation_incomplete"});
            const auto fail = [&](const char* why) {
                pairs_[pairIndex].reason = why;
                pairs_[pairIndex].result = out;
                return Reject(reason, why);
            };
            const char* why = nullptr;
            std::vector<uint8_t> originalVertex, albedoVertex, pixel;
            if (!Read(originalShader, 0x10, 0, originalVertex, &why) ||
                !Read(albedoShader, 0x10, 0, albedoVertex, &why) ||
                !Read(albedoShader, 0x50, 4, pixel, &why)) return fail(why);
            // Retain the selected PS even if strict replay validation rejects it.
            // Its default targetCount=0 explicitly means an unverified contract.
            const uint64_t hash = HashFNV1a(pixel.data(), pixel.size());
            size_t selected = shaders_.size();
            for (size_t i = 0; i < shaders_.size(); ++i)
                if (shaders_[i].hashFNV1a == hash && shaders_[i].bytes == pixel) { selected = i; break; }
            if (selected == shaders_.size()) shaders_.push_back({std::move(pixel), hash, {}});
            out.selectedIndex = selected;
            if (originalVertex != albedoVertex) {
                if (captureVertexEvidence_) out.vertexEvidenceIndex=vertexEvidence_.Add(originalVertex,albedoVertex,shaders_[selected].bytes);
                return fail("native_vertex_bytecode_differs");
            }
            if (!ParseVertex(originalVertex.data(), originalVertex.size(), &why)) return fail(why);
            Contract contract{};
            const auto& captured = shaders_[selected].bytes;
            if (!ParsePixelContract(captured.data(), captured.size(), contract, &why)) return fail(why);
            shaders_[selected].contract = contract;
            out.targetCount = contract.targetCount;
            pairs_[pairIndex].result = out;
            pairs_[pairIndex].valid = true;
            pairs_[pairIndex].reason = nullptr;
            return true;
        } catch (...) {
            if (pairIndex < pairs_.size()) pairs_[pairIndex].reason = "native_shader_allocation_failed";
            return Reject(reason, "native_shader_allocation_failed");
        }
    }
};
#endif
} // namespace RRAlbedoShader
