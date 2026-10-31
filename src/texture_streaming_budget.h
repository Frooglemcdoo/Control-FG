#pragma once
namespace texture_streaming_budget {
constexpr unsigned Normalize(unsigned value) noexcept {
 return value==4||value==6||value==8||value==10||value==12?value:0;
}
// Exact process-local overrides; reject truncated strings and trailing junk.
inline bool Override(const char* text,unsigned length,float& budget) noexcept {
 unsigned value=0;
 if(text&&length==1&&text[0]>='0'&&text[0]<='9')value=unsigned(text[0]-'0');
 else if(text&&length==2&&text[0]=='1'&&(text[1]=='0'||text[1]=='2'))value=10+unsigned(text[1]-'0');
 if(value!=1&&!Normalize(value))return false;
 budget=static_cast<float>(value);return true;
}
}
