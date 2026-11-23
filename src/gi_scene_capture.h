#pragma once
#include <atomic>
#include "gi_reuse/motion_capture_layout.h"
namespace gi_scene_capture {
inline bool Enabled() noexcept {
 static const bool on=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_SCENE_CAPTURE",v,sizeof(v))==1&&v[0]=='1';}();return on;
}
inline bool Motion() noexcept {
 static const bool on=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_MOTION_CAPTURE",v,sizeof(v))==1&&v[0]=='1';}();return on&&Enabled();
}
inline std::atomic<unsigned> motionRecorded{0},motionExported{0};
inline std::atomic<bool> motionFailed{false};
inline bool TextureMipOne() noexcept {
 static const bool on=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_TEXTURE_MIP1",v,sizeof(v))==1&&v[0]=='1';}();return on;
}
inline unsigned Rays() noexcept {char v[8]{};GetEnvironmentVariableA("CONTROLTOH_GI_AUTO_RAYS",v,sizeof(v));return !v[0]||v[0]=='4'?4u:10u;}
inline std::atomic<bool(*)(void*)> rawTexture{nullptr};
inline std::atomic<unsigned long long> detailedFrame{0};
}
