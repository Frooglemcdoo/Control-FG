#pragma once
#include "../build/rr_hair_opacity_compiled.h"
static constexpr int kRRHairOpacityStorageId=0x6F475034;
static std::atomic<bool> rrHairOpacityStorageReady{false};
static std::uintptr_t RRHairOpacityReplacement(std::uintptr_t original,std::uintptr_t selected,
 const RRAlbedoShader::CapturedPixelShader& albedo) noexcept;
