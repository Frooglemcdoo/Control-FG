#pragma once
// Exact R22-captured standard material pair. A new cache is used each frame.
static bool RRWorldAlbedoPairMatches(std::uintptr_t original,
 const RRAlbedoShader::CapturedPixelShader& albedo) noexcept;
struct RRWorldAlbedoAdmission {
 struct Entry {std::uintptr_t original,selected;bool accepted;};
 std::array<Entry,128> entries{};size_t count=0;
 bool Allow(std::uintptr_t original,std::uintptr_t selected,
            const RRAlbedoShader::CapturedPixelShader& albedo) noexcept {
  for(size_t i=0;i<count;++i)if(entries[i].original==original&&entries[i].selected==selected)return entries[i].accepted;
  if(count>=entries.size())return false;
  const bool accepted=RRWorldAlbedoPairMatches(original,albedo);
  entries[count++]={original,selected,accepted};return accepted;
 }
};

#include "../build/rr_world_albedo_compiled.h"
static constexpr int kRRWorldAlbedoStorageId=0x6F475036;
static std::atomic<bool> rrWorldAlbedoStorageReady{false};
static std::uintptr_t RRWorldAlbedoReplacement(std::uintptr_t original,std::uintptr_t selected);
