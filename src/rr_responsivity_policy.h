#pragma once
namespace control_rr_responsivity {
inline constexpr int Min=-100;
inline constexpr int Max=100;
inline constexpr int Default=-50;
inline int Normalize(int value) noexcept {return value<Min?Min:(value>Max?Max:value);}
inline float MaskValue(int value) noexcept {return static_cast<float>(Normalize(value))/100.0f;}
inline bool Enabled(bool presetF,int value) noexcept {return presetF&&Normalize(value)!=0;}
inline float ClampFrameTimeMs(float value) noexcept {
 if(!(value>0.0f))return 16.666667f;
 if(value<1.0f)return 1.0f;
 if(value>100.0f)return 100.0f;
 return value;
}
}
