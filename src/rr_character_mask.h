#pragma once
#ifdef CONTROL_CHARACTER_GI_BLEND
inline std::atomic<bool> rrCharacterBlendRequested{true};
inline std::atomic<const wchar_t*> rrCharacterBlendStatus{L"Waiting for character GI"};
#include "../build/character_mask_opaque.h"
#include "../build/character_mask_hair.h"
#include "../build/character_mask_ambient.h"
static constexpr int kCharacterMaskOpaqueId=0x6F475036, kCharacterMaskHairId=0x6F475037, kCharacterMaskAmbientId=0x6F475038;
static std::uintptr_t RRCharacterMaskShader(std::uintptr_t original, std::uintptr_t selected,
                                           unsigned family) noexcept;
#endif
