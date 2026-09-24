#pragma once
// Native draw ABI revalidated against the exact supplied DLLs for G2.
// See g2-review/native-draw/NATIVE-DRAW-REVIEW.md for byte-span evidence.
// Renderer SHA256 ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433.
// CPU preparation only. Installs no hooks, creates no GPU resources, enables no RR.
// Execute only in the same native draw callback/frame as the captured batches.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace control_rr_albedo {
struct OpaqueBatch {
    std::uintptr_t shader;
    std::uintptr_t mesh;
    std::uintptr_t material;
    std::int32_t instanceByteOffset;
    std::int32_t instanceCount;
    std::uint8_t flags;
    std::uint8_t tessellated;
    std::uint16_t opaqueMetadata;
    std::uint32_t instanceBufferIndex;
};
static_assert(sizeof(std::uintptr_t) == 8);
static_assert(sizeof(OpaqueBatch) == 0x28);
static_assert(offsetof(OpaqueBatch, material) == 0x10);
static_assert(offsetof(OpaqueBatch, instanceBufferIndex) == 0x24);

// Catch native access violations in separate POD-only SEH adapters. __try in
// the STL-owning preparation function would reproduce MSVC error C2712.
struct Access {
    bool (*read)(std::uintptr_t address, void* out, std::size_t bytes);
    bool (*getShader)(std::uintptr_t technique, std::uint32_t key,
                      std::uintptr_t* shader);
    // Optional extra candidate gate, invoked only during CPU preparation.
    // G2 uses resident bytecode comparison/reflection here before publication.
    bool (*validateShader)(std::uintptr_t original, std::uintptr_t replacement) = nullptr;
    void* candidateContext = nullptr;
    bool (*validateEyeCandidate)(void*, std::uintptr_t, std::uintptr_t) = nullptr;
};
struct Coverage {
    std::uint32_t sourceBatches = 0;
    std::uint32_t replayBatches = 0;
    std::uint32_t rejectedBatches = 0;
    // Covers the selected opaque batch range, NOT every contributor in the scene.
    bool complete = false;
};
enum class RejectReason : std::uint32_t {
    InvalidInstance = 0, MissingCorePointers, Tessellated, MaterialTechniqueUnreadable,
    AlbedoTechniqueUnreadable, ShaderKeyUnreadable, MaterialNameUnreadable,
    UnsupportedMaterialFamily, TechniqueContractMismatch, OriginalShaderMismatch,
    AlbedoShaderLookupFailed, AlbedoShaderKeyMismatch, StageContractMismatch,
    ShaderValidatorRejected, EyeVariantNotUnique, Count
};
inline const char* RejectReasonName(RejectReason reason) noexcept {
    static const char* names[] = {
        "invalid_instance", "missing_core_pointers", "tessellated", "material_technique_unreadable",
        "albedo_technique_unreadable", "shader_key_unreadable", "material_name_unreadable",
        "unsupported_material_family", "technique_contract_mismatch", "original_shader_mismatch",
        "albedo_shader_lookup_failed", "albedo_shader_key_mismatch", "stage_contract_mismatch",
        "shader_validator_rejected", "eye_variant_not_unique"
    };
    const auto i=static_cast<std::uint32_t>(reason);
    return i<static_cast<std::uint32_t>(RejectReason::Count) ? names[i] : "unknown";
}
struct EyeVariantEvidence {
    std::array<std::uint32_t,4> keys{};
    std::array<bool,4> matches{};
    std::uint32_t matchCount=0; bool tableValid=false;
};
struct RejectionExample {
    EyeVariantEvidence eye{};
    std::uint32_t sourceIndex = 0;
    RejectReason reason = RejectReason::MissingCorePointers;
    std::uintptr_t material = 0, materialTechnique = 0, albedoTechnique = 0;
    std::uintptr_t originalShader = 0, replacementShader = 0;
    std::uint32_t shaderKey = 0;
    std::int32_t instanceCount = 0;
    std::uint32_t instanceBufferIndex = 0;
    std::uint8_t tessellated = 0, flags = 0;
    char materialFamily[48]{};
    // G8 retains the read-only family contract audit. Character and cloth are
    // now admitted only when they match their exact G7-observed contracts;
    // unsupported families continue to use these fields for later review.
    std::uint32_t mainOr = 0, mainClear = 0, mainCount = 0;
    std::uint32_t albedoOr = 0, albedoClear = 0, albedoCount = 0;
    std::uint32_t predictedAlbedoKey = 0, effectiveAlbedoKey = 0, actualReplacementKey = 0;
    std::uint8_t techniqueContractReadable = 0;
    std::uint8_t originalLookupSucceeded = 0, originalMatchesBatch = 0;
    std::uint8_t albedoLookupSucceeded = 0, replacementKeyMatches = 0, effectiveKeyMatches = 0;
    std::uint8_t stageContractReadable = 0;
    std::uint8_t hasVertex = 0, hasPixel = 0, hasHull = 0, hasDomain = 0;
    std::uint8_t hasGeometry = 0, hasCompute = 0, hasRay = 0;
};
struct RejectionAudit {
    std::array<std::uint32_t, static_cast<std::size_t>(RejectReason::Count)> counts{};
    std::vector<RejectionExample> examples;
    std::uint32_t total = 0;
};
struct PreparedReplay {
    alignas(8) std::array<std::byte, 0x50> manager{};
    std::vector<OpaqueBatch> batches;
    // The original index of every accepted batch. Keep paired with batches
    // when further shader-contract filtering removes unsupported candidates.
    std::vector<std::uint32_t> sourceIndices;
    // Parallel to batches/sourceIndices: 1=StandardMaterial, 2=character, 3=cloth, 4=foliage, 5=hair.
    // G11 preserves G10 family admission and uses this for replay timing/warm-repeat attribution.
    std::vector<std::uint8_t> familyKinds;
    std::uint32_t sourceFirst = 0, sourceEnd = 0;
    Coverage coverage{};
    RejectionAudit rejectionAudit{};
    // Refresh immediately before native DrawOpaque(view,0,-1): moving this C++
    // object can invalidate a previously embedded batches.data() pointer.
    void* bindManagerView() {
        const auto p = reinterpret_cast<std::uintptr_t>(batches.data());
        const auto n = static_cast<std::uint32_t>(batches.size());
        std::memcpy(manager.data() + 0x30, &p, sizeof(p));
        std::memcpy(manager.data() + 0x38, &n, sizeof(n));
        return manager.data();
    }
};
template<class T> inline bool readAt(const Access& a, std::uintptr_t p,
                                     std::size_t offset, T& value) {
    if (!p || offset > (std::numeric_limits<std::uintptr_t>::max)() - p)
        return false;
    const auto address = p + offset;
    if (sizeof(value)-1 > (std::numeric_limits<std::uintptr_t>::max)() - address)
        return false;
    return a.read(address, &value, sizeof(value));
}
inline bool isStandardMaterialName(const Access& a, std::uintptr_t p) {
    char name[256]{};
    if (!p) return false;
    std::size_t end = 0, begin = 0;
    for (; end < sizeof(name)-1; ++end) {
        if (!readAt(a,p,end,name[end])) return false;
        if (!name[end]) break;
        if (name[end] == '/' || name[end] == '\\') begin=end+1;
        if (name[end] >= 'A' && name[end] <= 'Z') name[end] += 'a'-'A';
    }
    if (end == sizeof(name)-1) return false;
    const char* b=name+begin;
    return std::strcmp(b,"standardmaterial")==0 ||
           std::strcmp(b,"standardmaterial.rfx")==0 ||
           std::strcmp(b,"standardmaterial.obj")==0;
}
enum class MaterialAdmission : std::uint8_t { Unsupported=0, Standard, Character, Cloth, Foliage, Hair, Eye };
struct MaterialContract {
    std::uint32_t mainOr=0, mainClear=0, mainCount=0;
    std::uint32_t albedoOr=0, albedoClear=0, albedoCount=0;
};
inline std::uint32_t CanonicalTechniqueKey(std::uint32_t techniqueOr, std::uint32_t techniqueClear,
                                                  std::uint32_t requestedKey) noexcept {
    // Exact rend::Technique::getShader behavior at renderer RVA 0x1DD3F0:
    // effective = (technique.or | requested) & ~technique.clear, then binary-search by key.
    return (techniqueOr | requestedKey) & ~techniqueClear;
}
inline MaterialAdmission classifyMaterialFamily(const char* family, MaterialContract& c) noexcept {
    if (!family || !*family) return MaterialAdmission::Unsupported;
    if (std::strcmp(family,"standardmaterial")==0 || std::strcmp(family,"standardmaterial.rfx")==0 ||
        std::strcmp(family,"standardmaterial.obj")==0) {
        c={0x02000000u,0u,1024u,0x02000002u,0u,128u}; return MaterialAdmission::Standard;
    }
    if (std::strcmp(family,"character")==0 || std::strcmp(family,"character.rfx")==0 ||
        std::strcmp(family,"character.obj")==0) {
        c={0x02000000u,0u,128u,0x02000000u,64u,16u}; return MaterialAdmission::Character;
    }
    if (std::strcmp(family,"cloth")==0 || std::strcmp(family,"cloth.rfx")==0 ||
        std::strcmp(family,"cloth.obj")==0) {
        c={0x02000000u,1u,64u,0x02000002u,1u,8u}; return MaterialAdmission::Cloth;
    }
    if (std::strcmp(family,"foliage")==0 || std::strcmp(family,"foliage.rfx")==0 ||
        std::strcmp(family,"foliage.obj")==0) {
        c={0x02000000u,0x0040001Cu,16u,0x02000000u,0x3Cu,4u}; return MaterialAdmission::Foliage;
    }
    if (std::strcmp(family,"hair")==0 || std::strcmp(family,"hair.rfx")==0 ||
        std::strcmp(family,"hair.obj")==0) {
        c={0x02000000u,0x0Eu,32u,0x02000000u,0x0Fu,4u}; return MaterialAdmission::Hair;
    }
    if (std::strcmp(family,"eye")==0 || std::strcmp(family,"eye.rfx")==0 || std::strcmp(family,"eye.obj")==0) {
        c={0x02000000u,0u,16u,0x02000000u,0u,4u}; return MaterialAdmission::Eye;
    }
    return MaterialAdmission::Unsupported;
}
// r16 captured foliage originals omit leaf-wind deformation, while forcing
// bit 1 selected a replacement that adds it. Preserve the foliage request's
// own bit instead; the existing byte-exact VS validator still decides admission.
inline std::uint32_t RequestedAlbedoKey(std::uint32_t key, MaterialAdmission family) noexcept {
    return (key & ~0x80400000u) | (family==MaterialAdmission::Foliage ? 0x02000000u : 0x02000002u);
}
inline bool readMaterialFamily(const Access& a, std::uintptr_t p, char* out, std::size_t capacity) {
    if (!out || !capacity) return false; out[0]=0;
    char name[256]{}; if (!p) return false;
    std::size_t end=0,begin=0;
    for (; end<sizeof(name)-1; ++end) {
        if (!readAt(a,p,end,name[end])) return false;
        if (!name[end]) break;
        if (name[end]=='/' || name[end]=='\\') begin=end+1;
    }
    if (end==sizeof(name)-1) return false;
    std::size_t n=0;
    for (const char* q=name+begin; *q && n+1<capacity; ++q) {
        char c=*q; if (c>='A' && c<='Z') c+=char('a'-'A'); if (!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')) c='_'; out[n++]=c;
    }
    out[n]=0; return n!=0;
}
#include "rr_eye_variants.h"
inline void AuditFamilyContract(const Access& a, RejectionExample& e) {
    if (!e.materialTechnique || !e.albedoTechnique) return;
    bool techniqueReadable =
        readAt(a,e.materialTechnique,4,e.mainOr) &&
        readAt(a,e.materialTechnique,8,e.mainClear) &&
        readAt(a,e.materialTechnique,0x240,e.mainCount) &&
        readAt(a,e.albedoTechnique,4,e.albedoOr) &&
        readAt(a,e.albedoTechnique,8,e.albedoClear) &&
        readAt(a,e.albedoTechnique,0x240,e.albedoCount);
    e.techniqueContractReadable = techniqueReadable ? 1 : 0;
    if (!techniqueReadable) return;
    std::uintptr_t original=0;
    if (a.getShader(e.materialTechnique,e.shaderKey,&original) && original) {
        e.originalLookupSucceeded=1;
        e.originalMatchesBatch = original==e.originalShader ? 1 : 0;
    }
    MaterialContract ignored{};
    e.predictedAlbedoKey=RequestedAlbedoKey(e.shaderKey,classifyMaterialFamily(e.materialFamily,ignored));
    e.effectiveAlbedoKey=CanonicalTechniqueKey(e.albedoOr,e.albedoClear,e.predictedAlbedoKey);
    if (!a.getShader(e.albedoTechnique,e.predictedAlbedoKey,&e.replacementShader) || !e.replacementShader) return;
    e.albedoLookupSucceeded=1;
    if (!readAt(a,e.replacementShader,4,e.actualReplacementKey)) return;
    // Keep the G7/G8 raw-request comparison for historical continuity; G11
    // also records the engine-canonical comparison proven from getShader.
    e.replacementKeyMatches = e.actualReplacementKey==e.predictedAlbedoKey ? 1 : 0;
    e.effectiveKeyMatches = e.actualReplacementKey==e.effectiveAlbedoKey ? 1 : 0;
    std::uintptr_t vertex=0,pixel=0,hull=0,domain=0,geometry=0,compute=0,ray=0;
    if (!readAt(a,e.replacementShader,0x10,vertex) || !readAt(a,e.replacementShader,0x50,pixel) ||
        !readAt(a,e.replacementShader,0x20,hull) || !readAt(a,e.replacementShader,0x30,domain) ||
        !readAt(a,e.replacementShader,0x40,geometry) || !readAt(a,e.replacementShader,0x60,compute) ||
        !readAt(a,e.replacementShader,0x70,ray)) return;
    e.stageContractReadable=1;
    e.hasVertex=vertex?1:0; e.hasPixel=pixel?1:0; e.hasHull=hull?1:0; e.hasDomain=domain?1:0;
    e.hasGeometry=geometry?1:0; e.hasCompute=compute?1:0; e.hasRay=ray?1:0;
}
inline RejectReason DiagnoseReject(const Access& a, const OpaqueBatch& b, RejectionExample& e) {
    e.material=b.material; e.originalShader=b.shader; e.instanceCount=b.instanceCount;
    e.instanceBufferIndex=b.instanceBufferIndex; e.tessellated=b.tessellated; e.flags=b.flags;
    if (b.instanceCount<=0 || b.instanceByteOffset<0) return RejectReason::InvalidInstance;
    if (!b.shader || !b.mesh || !b.material) return RejectReason::MissingCorePointers;
    if (b.tessellated) return RejectReason::Tessellated;
    if (!readAt(a,b.material,0x9A0,e.materialTechnique) || !e.materialTechnique) return RejectReason::MaterialTechniqueUnreadable;
    if (!readAt(a,b.shader,4,e.shaderKey)) return RejectReason::ShaderKeyUnreadable;
    std::uintptr_t name=0;
    if (!readAt(a,e.materialTechnique,0x100,name) || !name || !readMaterialFamily(a,name,e.materialFamily,sizeof(e.materialFamily))) return RejectReason::MaterialNameUnreadable;
    if (!readAt(a,b.material,0x9D8,e.albedoTechnique) || !e.albedoTechnique) return RejectReason::AlbedoTechniqueUnreadable;
    MaterialContract expected{};
    const auto admission=classifyMaterialFamily(e.materialFamily,expected);
    if (admission==MaterialAdmission::Unsupported) {
        AuditFamilyContract(a,e);
        return RejectReason::UnsupportedMaterialFamily;
    }
    std::uint32_t mainOr=0,mainClear=0,mainCount=0,albedoOr=0,albedoClear=0,albedoCount=0;
    if (!readAt(a,e.materialTechnique,4,mainOr) || !readAt(a,e.materialTechnique,8,mainClear) ||
        !readAt(a,e.materialTechnique,0x240,mainCount) || !readAt(a,e.albedoTechnique,4,albedoOr) ||
        !readAt(a,e.albedoTechnique,8,albedoClear) || !readAt(a,e.albedoTechnique,0x240,albedoCount) ||
        mainOr!=expected.mainOr || mainClear!=expected.mainClear || mainCount!=expected.mainCount ||
        albedoOr!=expected.albedoOr || albedoClear!=expected.albedoClear || albedoCount!=expected.albedoCount)
        return RejectReason::TechniqueContractMismatch;
    std::uintptr_t original=0;
    if (!a.getShader(e.materialTechnique,e.shaderKey,&original) || original!=b.shader) return RejectReason::OriginalShaderMismatch;
    if (admission==MaterialAdmission::Eye) {
        const bool unique=SelectEyeVariant(a,e.albedoTechnique,b.shader,e.replacementShader,&e.eye);
        return unique ? RejectReason::ShaderValidatorRejected : RejectReason::EyeVariantNotUnique;
    }
    const std::uint32_t albedoKey=RequestedAlbedoKey(e.shaderKey,admission);
    const std::uint32_t effectiveAlbedoKey=CanonicalTechniqueKey(albedoOr,albedoClear,albedoKey);
    if (!a.getShader(e.albedoTechnique,albedoKey,&e.replacementShader) || !e.replacementShader) return RejectReason::AlbedoShaderLookupFailed;
    std::uint32_t actualKey=0;
    if (!readAt(a,e.replacementShader,4,actualKey) || actualKey!=effectiveAlbedoKey) return RejectReason::AlbedoShaderKeyMismatch;
    std::uintptr_t vertex=0,pixel=0,hull=0,domain=0,geometry=0,compute=0,ray=0;
    if (!readAt(a,e.replacementShader,0x10,vertex) || !readAt(a,e.replacementShader,0x50,pixel) ||
        !readAt(a,e.replacementShader,0x20,hull) || !readAt(a,e.replacementShader,0x30,domain) ||
        !readAt(a,e.replacementShader,0x40,geometry) || !readAt(a,e.replacementShader,0x60,compute) ||
        !readAt(a,e.replacementShader,0x70,ray) || !vertex || !pixel || hull || domain || geometry || compute || ray)
        return RejectReason::StageContractMismatch;
    return RejectReason::ShaderValidatorRejected;
}
inline void RecordReject(RejectionAudit& audit, const Access& a, const OpaqueBatch& b, std::uint32_t sourceIndex, RejectReason forced=RejectReason::Count) {
    RejectionExample e{}; e.sourceIndex=sourceIndex;
    e.reason = forced==RejectReason::Count ? DiagnoseReject(a,b,e) : forced;
    const auto i=static_cast<std::size_t>(e.reason);
    if (i<audit.counts.size()) ++audit.counts[i];
    ++audit.total;
    if (audit.examples.size()<64) audit.examples.push_back(e);
}
inline bool selectStandardAlbedo(const Access& a, const OpaqueBatch& b,
                                 std::uintptr_t& shader, std::uint8_t* familyKind=nullptr) {
    if (!b.shader || !b.mesh || !b.material || b.tessellated) return false;
    std::uintptr_t materialTechnique=0, albedoTechnique=0, name=0, original=0;
    std::uint32_t key=0, mainOr=0, mainClear=0, mainCount=0,
                  albedoOr=0, albedoClear=0, albedoCount=0;
    char family[48]{};
    if (!readAt(a,b.material,0x9A0,materialTechnique) ||
        !readAt(a,b.material,0x9D8,albedoTechnique) ||
        !readAt(a,b.shader,4,key) ||
        !readAt(a,materialTechnique,0x100,name) || !name ||
        !readMaterialFamily(a,name,family,sizeof(family)) ||
        !readAt(a,materialTechnique,4,mainOr) ||
        !readAt(a,materialTechnique,8,mainClear) ||
        !readAt(a,materialTechnique,0x240,mainCount) ||
        !readAt(a,albedoTechnique,4,albedoOr) ||
        !readAt(a,albedoTechnique,8,albedoClear) ||
        !readAt(a,albedoTechnique,0x240,albedoCount)) return false;
    MaterialContract expected{};
    const auto admission=classifyMaterialFamily(family,expected);
    if (admission==MaterialAdmission::Unsupported) return false;
    if (familyKind) *familyKind=static_cast<std::uint8_t>(admission);
    if (mainOr!=expected.mainOr || mainClear!=expected.mainClear || mainCount!=expected.mainCount ||
        albedoOr!=expected.albedoOr || albedoClear!=expected.albedoClear || albedoCount!=expected.albedoCount) return false;
    if (!a.getShader(materialTechnique,key,&original) || original!=b.shader)
        return false;
    if (admission==MaterialAdmission::Eye) return SelectEyeVariant(a,albedoTechnique,b.shader,shader);
    const std::uint32_t albedoKey=RequestedAlbedoKey(key,admission);
    const std::uint32_t effectiveAlbedoKey=CanonicalTechniqueKey(albedoOr,albedoClear,albedoKey);
    if (!a.getShader(albedoTechnique,albedoKey,&shader) || !shader) return false;
    std::uint32_t actualKey=0;
    std::uintptr_t vertex=0,pixel=0,hull=0,domain=0,geometry=0,compute=0,ray=0;
    if (!readAt(a,shader,4,actualKey) || actualKey!=effectiveAlbedoKey ||
        !readAt(a,shader,0x10,vertex) || !readAt(a,shader,0x50,pixel) ||
        !readAt(a,shader,0x20,hull) || !readAt(a,shader,0x30,domain) ||
        !readAt(a,shader,0x40,geometry) || !readAt(a,shader,0x60,compute) ||
        !readAt(a,shader,0x70,ray)) return false;
    return vertex && pixel && !hull && !domain && !geometry && !compute && !ray;
}
inline bool prepare(const Access& a, std::uintptr_t nativeManager,
                    PreparedReplay& out, bool allowPartial=false,
                    std::uint32_t maximumBatches=32768,
                    int first=0, int endExclusive=-1) {
    out.coverage={}; out.rejectionAudit={}; out.batches.clear(); out.sourceIndices.clear(); out.familyKinds.clear();
    out.sourceFirst=out.sourceEnd=0; out.manager.fill(std::byte{0});
    if (!a.read || !a.getShader || !nativeManager ||
        !a.read(nativeManager,out.manager.data(),out.manager.size())) return false;
    std::uintptr_t batches=0,instances=0;
    std::uint32_t count=0,instanceBuffers=0;
    std::memcpy(&batches,out.manager.data()+0x30,8);
    std::memcpy(&count,out.manager.data()+0x38,4);
    std::memcpy(&instances,out.manager.data()+0x40,8);
    std::memcpy(&instanceBuffers,out.manager.data()+0x48,4);
    if (!count || count>maximumBatches || !batches || !instances || !instanceBuffers || first<0)
        return false;
    const auto begin=static_cast<std::uint32_t>(first);
    const auto end=endExclusive<0 ? count : static_cast<std::uint32_t>(endExclusive);
    // Never silently clamp invalid native ranges: fail before submitting work.
    if (begin>=end || end>count) return false;
    out.sourceFirst=begin; out.sourceEnd=end;
    out.coverage.sourceBatches=end-begin;
    out.batches.reserve(end-begin); out.sourceIndices.reserve(end-begin); out.familyKinds.reserve(end-begin);
    for (std::uint32_t i=begin;i<end;++i) {
        OpaqueBatch b{}; std::uintptr_t replacement=0; std::uint8_t familyKind=0;
        if (!readAt(a,batches,std::size_t(i)*sizeof(b),b)) {
            out.batches.clear(); out.sourceIndices.clear(); out.familyKinds.clear(); return false;
        }
        const bool baseValid=b.instanceCount>0 && b.instanceByteOffset>=0 &&
            b.instanceBufferIndex<instanceBuffers && selectStandardAlbedo(a,b,replacement,&familyKind);
        const bool shaderValid=baseValid && (!a.validateShader || a.validateShader(b.shader,replacement));
        if (!shaderValid) {
            ++out.coverage.rejectedBatches;
            if (baseValid) RecordReject(out.rejectionAudit,a,b,i,RejectReason::ShaderValidatorRejected);
            else if (b.instanceBufferIndex>=instanceBuffers) RecordReject(out.rejectionAudit,a,b,i,RejectReason::InvalidInstance);
            else RecordReject(out.rejectionAudit,a,b,i);
            continue;
        }
        b.shader=replacement;
        out.batches.push_back(b); out.sourceIndices.push_back(i); out.familyKinds.push_back(familyKind);
    }
    out.coverage.replayBatches=static_cast<std::uint32_t>(out.batches.size());
    out.coverage.complete=out.coverage.rejectedBatches==0;
    if (!out.coverage.complete && !allowPartial) {
        out.batches.clear(); out.sourceIndices.clear(); out.familyKinds.clear(); return false;
    }
    return !out.batches.empty();
}
// One stack-local view per draw callback. The full immutable batch array stays
// owned by the capture until all primary workers have joined. This view is
// accepted only by audited DrawOpaque; call it with (managerView(), 0, -1).
struct DrawRange {
    alignas(8) std::array<std::byte, 0x50> manager{};
    Coverage coverage{};
    std::uint32_t sourceFirst=0, sourceEnd=0;
    std::array<std::uint32_t,7> familyCounts{}; // [1]=standard [2]=character [3]=cloth [4]=foliage [5]=hair
    void* managerView() noexcept { return manager.data(); }
};
inline bool ShouldWarmRepeatHairRange(const DrawRange& range) noexcept {
    return range.familyCounts[5] != 0;
}
inline bool prepareDrawRange(const PreparedReplay& full, int first,
                             int endExclusive, DrawRange& out) noexcept {
    out=DrawRange{};
    if (first<0 || full.batches.size()!=full.sourceIndices.size() || full.batches.size()!=full.familyKinds.size() ||
        full.sourceFirst>=full.sourceEnd) return false;
    const auto begin=static_cast<std::uint32_t>(first);
    const auto end=endExclusive<0 ? full.sourceEnd : static_cast<std::uint32_t>(endExclusive);
    if (begin<full.sourceFirst || begin>=end || end>full.sourceEnd) return false;
    out.sourceFirst=begin; out.sourceEnd=end;
    out.coverage.sourceBatches=end-begin;
    // prepare() produces sorted unique indices; further validation may erase
    // pairs but must never reorder/add entries after publication.
    const auto lo=std::lower_bound(full.sourceIndices.begin(),full.sourceIndices.end(),begin);
    const auto hi=std::lower_bound(lo,full.sourceIndices.end(),end);
    const auto count=static_cast<std::uint32_t>(hi-lo);
    out.coverage.replayBatches=count;
    out.coverage.rejectedBatches=end-begin-count;
    out.coverage.complete=out.coverage.rejectedBatches==0;
    if (!count) return false;
    const auto offset=static_cast<std::size_t>(lo-full.sourceIndices.begin());
    for (std::size_t j=offset;j<offset+count;++j) {
        const auto k=full.familyKinds[j]; if (k<out.familyCounts.size()) ++out.familyCounts[k];
    }
    out.manager=full.manager;
    const auto pointer=reinterpret_cast<std::uintptr_t>(full.batches.data()+offset);
    std::memcpy(out.manager.data()+0x30,&pointer,sizeof(pointer));
    std::memcpy(out.manager.data()+0x38,&count,sizeof(count));
    return true;
}
// This copied receiver view is accepted only by audited renderer+0x1C1AE0.
// Never pass it to engine constructors/destructors/update or another build.
using DrawOpaque = void(*)(void* auditedManagerView, int first, int endExclusive);
}
