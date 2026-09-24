#pragma once
#include "rr_reflection_binding.h"
namespace control_rr_specular {
using namespace control_rr_reflection;
inline constexpr Address TemporalDispatchRva=0x16786;
inline constexpr Address TemporalSourceRva=0x128f540;
inline constexpr Address TemporalTargetRva=0x128f518;
struct Pair {
 Address source=0,target=0,sourceResource=0,targetResource=0,sourceDevice=0,targetDevice=0;
 Shape shape{},targetShape{};
 unsigned sourceFlags=0,targetFlags=0;
 unsigned char sourceManual=0,targetManual=0;
};
struct Scope {
 Context frame{},dispatch{};
 Address sourceNative=0;
 bool rrCommitted=false;
 unsigned spatialPasses=0;
 bool attempted=false,recorded=false,referenceDispatched=false,nativeClampPrepared=false,nativeClampDispatched=false,nativeClampRestoreFailed=false,knownReplacementContamination=false,failed=false;
 const char* reason="not_attempted";
 Pair pair{};
};
inline const char* ReadPair(const Access& a,Address renderer,Pair& p) {
 p={};
 if(!renderer||renderer>UINTPTR_MAX-TemporalSourceRva||!a.read||!a.shaderNative||!a.describe)return "pair_access";
 p.source=a.shaderNative(a.context,renderer+TemporalSourceRva);
 p.target=a.shaderNative(a.context,renderer+TemporalTargetRva);
 if(!p.source||!p.target||p.source==p.target)return "pair_native_identity";
 if(!Read(a,p.source,0x38,p.sourceFlags)||!Read(a,p.target,0x38,p.targetFlags)||
    !Read(a,p.source,0x68,p.sourceManual)||!Read(a,p.target,0x68,p.targetManual))return "pair_native_fields";
 if((p.sourceFlags&0x30)||(p.targetFlags&0x30)||p.sourceManual||p.targetManual)return "pair_manual_or_special_state";
 if(!Read(a,p.source,0x88,p.sourceResource)||!Read(a,p.target,0x88,p.targetResource)||
    !p.sourceResource||!p.targetResource||p.sourceResource==p.targetResource)return "pair_resource_identity";
 if(!a.describe(a.context,p.sourceResource,p.shape,p.sourceDevice)||
    !a.describe(a.context,p.targetResource,p.targetShape,p.targetDevice))return "pair_resource_description";
 if(!p.sourceDevice||p.sourceDevice!=p.targetDevice)return "pair_device_mismatch";
 const auto& s=p.shape;const auto& t=p.targetShape;
 // Native copyRegion selects mip=0, layer=0 at both ends. Total mip counts
 // need not match: the temporal source has a mip chain used by the firefly clamp.
 if(s.dimension!=3||t.dimension!=3||!s.format||s.format!=t.format)return "pair_format_or_dimension";
 if(s.width<64||s.width>8192||s.height<64||s.height>8192||s.width!=t.width||s.height!=t.height)return "pair_mip_zero_extent";
 if(s.layers!=1||t.layers!=1||s.samples!=1||t.samples!=1||s.quality||t.quality||(s.flags&2)||(t.flags&2))return "pair_layers_samples_or_depth";
 unsigned maxMips=1;auto maxDimension=s.width>s.height?s.width:s.height;
 while(maxDimension>1){maxDimension>>=1;++maxMips;}
 if(!s.mips||!t.mips||s.mips>maxMips||t.mips>maxMips)return "pair_mip_count";
 return nullptr;
}
// Scope entry has not recorded any replacement commands. Native setup between
// filter entry and temporal dispatch may change the primary command list. It
// must preserve the frame/view/device/queue; all reads at dispatch use its list.
inline bool SameFrameOwner(const Context& a,const Context& b) {
 return a.frame==b.frame&&a.view==b.view&&a.device==b.device&&a.queue==b.queue;
}
// Validate that the exact temporal dispatch still belongs to the committed frame and
// source/target pair. This performs no GPU work, so a rejected reference arm can safely
// fall back to the proven raw-copy substitution.
template<class Api> bool ValidateReferenceDispatch(Scope& scope,const Access& a,Api& api,
 Address renderer,Address site,Address deviceState) {
 scope.reason="reference_scope_not_eligible";
 if(!scope.rrCommitted||scope.spatialPasses||scope.attempted||scope.failed||site!=TemporalDispatchRva)return false;
 scope.reason="reference_frame_missing";
 if(!a.currentContext||!scope.frame.frame||!scope.frame.device||!scope.frame.queue||!scope.frame.list||!scope.frame.view)return false;
 scope.reason="reference_native_device_state";if(!api.StateMatches(deviceState))return false;
 scope.reason="reference_dispatch_frame_owner";
 if(!a.currentContext(a.context,scope.dispatch)||!scope.dispatch.list||!SameFrameOwner(scope.dispatch,scope.frame))return false;
 auto& p=scope.pair;
 if(const auto reason=ReadPair(a,renderer,p)){scope.reason=reason;return false;}
 scope.reason="reference_source_changed_since_filter_entry";if(p.source!=scope.sourceNative)return false;
 scope.reason="reference_dispatch_resource_device";if(p.sourceDevice!=scope.dispatch.device)return false;
 Pair again{};
 if(const auto reason=ReadPair(a,renderer,again)){scope.reason=reason;return false;}
 scope.reason="reference_pair_changed_before_dispatch";
 if(p.source!=again.source||p.target!=again.target||p.sourceResource!=again.sourceResource||p.targetResource!=again.targetResource||
    p.sourceDevice!=again.sourceDevice||p.targetDevice!=again.targetDevice||!Same(p.shape,again.shape)||!Same(p.targetShape,again.targetShape)||
    p.sourceFlags!=again.sourceFlags||p.targetFlags!=again.targetFlags||p.sourceManual!=again.sourceManual||p.targetManual!=again.targetManual)return false;
 Context current{};scope.reason="reference_context_changed_before_dispatch";
 if(!api.StateMatches(deviceState)||!a.currentContext(a.context,current)||!Same(current,scope.dispatch))return false;
 scope.reason="reference_dispatch_admitted";
 return true;
}

// Call only at the exact temporal-dispatch site inside a committed native filter.
// Native copyRegion records transitions/counter updates and restores states.
template<class Api> bool Substitute(Scope& scope,const Access& a,Api& api,
 Address renderer,Address site,Address deviceState) {
 scope.reason="scope_not_eligible";
 if(!scope.rrCommitted||scope.spatialPasses||scope.attempted||scope.failed||site!=TemporalDispatchRva)return false;
 // Mark rejection before borrowed reads; clear it only after a complete copy.
 scope.failed=true;
 scope.reason="scope_frame_missing";
 if(!a.currentContext||!scope.frame.frame||!scope.frame.device||!scope.frame.queue||!scope.frame.list||!scope.frame.view)return false;
 scope.reason="native_device_state";if(!api.StateMatches(deviceState))return false;
 scope.reason="dispatch_frame_owner";
 if(!a.currentContext(a.context,scope.dispatch)||!scope.dispatch.list||!SameFrameOwner(scope.dispatch,scope.frame))return false;
 auto& p=scope.pair;
 if(const auto reason=ReadPair(a,renderer,p)){scope.reason=reason;return false;}
 scope.reason="source_changed_since_filter_entry";if(p.source!=scope.sourceNative)return false;
 scope.reason="dispatch_resource_device";if(p.sourceDevice!=scope.dispatch.device)return false;
 Pair again{};
 if(const auto reason=ReadPair(a,renderer,again)){scope.reason=reason;return false;}
 scope.reason="pair_changed_before_copy";
 if(p.source!=again.source||p.target!=again.target||p.sourceResource!=again.sourceResource||p.targetResource!=again.targetResource||
    p.sourceDevice!=again.sourceDevice||p.targetDevice!=again.targetDevice||!Same(p.shape,again.shape)||!Same(p.targetShape,again.targetShape)||
    p.sourceFlags!=again.sourceFlags||p.targetFlags!=again.targetFlags||p.sourceManual!=again.sourceManual||p.targetManual!=again.targetManual)return false;
 Context current{};scope.reason="dispatch_context_changed_before_copy";
 if(!api.StateMatches(deviceState)||!a.currentContext(a.context,current)||!Same(current,scope.dispatch))return false;
 scope.reason="native_copy_fault";scope.attempted=true;
 if(!api.CopyNative(p.target,p.source))return false;
 scope.recorded=true;scope.failed=false;scope.reason="copied_mip_zero";return true;
}
}
