#ifndef CONTROL_PT_MATERIAL_ADMISSION
#define CONTROL_PT_MATERIAL_ADMISSION
// Secondary hits use the reviewed common Part3/texture approximation. Full PT
// also converts the observed opaque primary modes (0, 1, 3) to that closure,
// using raster GBuffer normals/F0/roughness and compositor material color.
// Scattering-kernel indices describe native screen-space lighting, not opacity.
// This is an explicit diffuse/GGX approximation, not native skin/cloth/SSS
// equivalence. Unknown mode 2 and other encodings remain unadmitted. Transparent
// composition is separate and cannot acquire ownership through this predicate.
bool PTMaterialScatteringSupported(uint packed,uint scattering,bool primary){
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
    uint mode=packed&255u;
    return !primary||mode==0u||mode==1u||mode==3u;
#else
    return !primary||((packed&255u)==0u&&(scattering&65535u)==0u);
#endif
}
#endif
