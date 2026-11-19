// R475 capture-only observations of the unchanged native secondary calculation.
#ifndef GI_SECONDARY_TERMS_LAYOUT
#define GI_SECONDARY_TERMS_LAYOUT
struct GISecondaryTerms {
 float4 texture; // RGB sample; w branch: 1 miss, 2 surface, 3 negligible material
 float4 tint; // material coefficient; w count of volume sentinel branches
 float4 volume; // normalized volume + ambient; w volume denominator
 float4 surface; // volume after distance/BRDF and 1/pi; w distance fade
 float4 pointLight; // point lighting before material; w visited point count
 float4 spot; // spot delta before material; w visited spot count
 float4 fallback; // weighted sentinel RGB before normalization; w weight sum
 float4 brdf; // volume BRDF multiplier; w miss multiplier
};
#endif
