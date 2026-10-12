#ifndef CONTROL_PT_GEOMETRY_BOUNDS
#define CONTROL_PT_GEOMETRY_BOUNDS
#ifdef __cplusplus
#include <cstdint>
namespace control_pt_geometry {
using uint=uint32_t;
#define PT_GEOM_INLINE inline
#else
#define PT_GEOM_INLINE
#endif
struct PTByteAddress { uint address; uint valid; };
PT_GEOM_INLINE PTByteAddress PTCheckedField(uint vertex,uint stride,uint field,uint bytes,uint limit) {
    PTByteAddress r;r.address=0;r.valid=0;
    if(stride==0||bytes==0||field>stride||bytes>stride-field||field>limit||bytes>limit-field) return r;
    if(vertex>(limit-field-bytes)/stride) return r;
    r.address=vertex*stride+field;r.valid=(r.address&3u)==0u?1u:0u;return r;
}
PT_GEOM_INLINE PTByteAddress PTCheckedTriangle(uint primitive,uint offset,uint stride,uint logicalBytes) {
    PTByteAddress r;r.address=0;r.valid=0;
    if((stride!=2u&&stride!=4u)||(offset%stride)!=0u||offset>logicalBytes) return r;
    uint triangleBytes=3u*stride;
    if(triangleBytes>logicalBytes-offset||primitive>(logicalBytes-offset-triangleBytes)/triangleBytes) return r;
    r.address=offset+primitive*triangleBytes;r.valid=1;return r;
}
PT_GEOM_INLINE bool PTCanLoadWord(uint address,uint physicalBytes) {
    return (address&3u)==0u&&physicalBytes>=4u&&address<=physicalBytes-4u;
}
#ifdef __cplusplus
}
#endif
#undef PT_GEOM_INLINE
#endif
