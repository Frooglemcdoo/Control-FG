// Shared integer operation; RGB bit patterns are unchanged, including NaNs.
uint4 ControlDiffuseAlphaOne(uint4 value) { value.w=0x3c00u;return value; }
