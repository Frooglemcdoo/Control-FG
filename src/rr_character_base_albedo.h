#pragma once
#include "../build/rr_character_base_albedo_compiled.h"
static constexpr int kRRCharacterBaseAlbedoStorageId=0x6F475035;
static std::atomic<bool> rrCharacterBaseAlbedoStorageReady{false};
static std::uintptr_t RRCharacterBaseAlbedoReplacement(std::uintptr_t original,std::uintptr_t selected,
 const RRAlbedoShader::CapturedPixelShader& albedo) noexcept;
