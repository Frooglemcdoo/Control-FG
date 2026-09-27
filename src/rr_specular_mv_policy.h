#pragma once
namespace control_rr_specmv {
inline bool RequestedForPreset(bool requested,unsigned activePreset,unsigned presetF) noexcept {
 return requested&&activePreset==presetF;
}
inline bool BindingChanged(bool previous,bool current) noexcept {return previous!=current;}
}
