#pragma once
#include "rr_user_control.h"
namespace control_rr {
inline constexpr const char* RRPresetKeys[]={
 "RayReconstruction.Hint.Render.Preset.DLAA",
 "RayReconstruction.Hint.Render.Preset.Quality",
 "RayReconstruction.Hint.Render.Preset.Balanced",
 "RayReconstruction.Hint.Render.Preset.Performance",
 "RayReconstruction.Hint.Render.Preset.UltraPerformance",
 "RayReconstruction.Hint.Render.Preset.UltraQuality"};
template<class Api> bool SetRRPreset(const Api& api,unsigned preset) {
 if(!RRUserPresetSupported(preset))return false;
 for(const char* key:RRPresetKeys)api.Set(key,preset);
 for(const char* key:RRPresetKeys)if(!api.Equals(key,preset))return false;
 return true;
}
}
