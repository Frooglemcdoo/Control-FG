#pragma once
namespace gi_reuse {
// A producer has already replaced the full native ray pass with half its rays.
// Only a recorded uniform complement permits ordinary full GI6 shading.
// Rejection is NOT native fallback: the hit textures are incomplete.
enum class FrozenCompletionResult {RejectedBeforeCommands,Recorded,NativeShadeRequired,FailedAfterCommands};
constexpr bool FrozenSuppressNativeShade(FrozenCompletionResult result)noexcept {
 return result!=FrozenCompletionResult::NativeShadeRequired;
}
}
