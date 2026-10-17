#ifndef CONTROL_PT_MATERIAL_ADMISSION
#define CONTROL_PT_MATERIAL_ADMISSION
// Primary BRDF/translucency encodings are not secondary-shading exclusions.
// Reviewed native reflected-hit shader 4fc0a0e9... reads Part1.x RGB at
// DWORD 3037..3086, ignores its BRDF byte and does not read Part1.y.
// Secondary shading uses the common Part3/texture approximation. This does
// not claim specialized primary hair/skin/translucency BRDF support.
bool PTMaterialScatteringSupported(uint packed,uint scattering,bool primary){
    return !primary||((packed&255u)==0u&&(scattering&65535u)==0u);
}
#endif
