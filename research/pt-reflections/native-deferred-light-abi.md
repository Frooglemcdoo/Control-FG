# Native deferred-reflection light ABI — PT bounce-2 integration

**Status:** Verified shader-reflection metadata, not yet validated GPU lighting transport.  
**Source:** Control renderer `rt_deferredReflection` native compute PSO, shader SHA-256 `4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9`, DXBC size **42,408 bytes**, `cs_5_1`, `[numthreads(8,16,1)]`.  
**Method:** Reconstructed the exact native DXBC from `PT_REFLECTION_P2_DEFERRED_DXBC_CHUNK` probe logs (111 chunks) and decoded the 38-entry RDEF resource-binding table plus struct-member-offset descriptors. Reconstructed SHA matches the known native capture. This is not a guess based on HLSL names.

## Relevant native bindings (existing native compute root signature)

| Shader resource | HLSL binding | ABI / meaning |
| --- | --- | --- |
| `g_tRaytracingLightCellPointLights` | `t5, space0` | Point-light cell/index texture; indexing/packing must be verified |
| `g_tRaytracingLightCellSpotLights` | `t6, space0` | Spotlight cell/index texture; indexing/packing must be verified |
| `g_tAmbientSHTerms` | `t7, space0` | Ambient SH input (sampling layout not yet verified) |
| `g_sEnvironmentCube` | `t8, space0` | Environment cube (currently used by P2) |
| `g_tPointLightShadowAtlas` | `t9, space0` | Point-shadow atlas |
| `g_sbDeferredLightPointData` | `t10, space0` | StructuredBuffer element stride **160 bytes** |
| `g_tSpotLightPCFShadowAtlas` | `t11, space0` | Spot PCF shadow atlas |
| `g_tSpotLightVSMShadowAtlas` | `t12, space0` | Spot VSM shadow atlas |
| `g_tSpotLightProjectionAtlas` | `t13, space0` | Spotlight projection |
| `g_sbDeferredLightSpotData` | `t14, space0` | StructuredBuffer element stride **320 bytes** |
| `g_sbActiveVolumeSamplingInfoArray` | `t16, space0` | Illumination volume data |
| `g_tGIBrickTextures` | `t19..t24, space0` | Six GI brick textures |
| `g_tMaterialId` | `t26, space0` | Traced hit material IDs |
| `g_tNormal_TexcoordX` | `t27, space0` | Traced hit normal |
| `g_tPosition_TexcoordY` | `t28, space0` | Traced hit position |
| `g_rwtReflectionTarget` | `u0, space0` | Native reflection output |
| `deferredlight_constants` | `b1, space0` | Includes point/spot-light counts, per-tile maxima |
| `rtreflection` | `b6, space0` | Native reflection constants |

## `DeferredLightPoint` — exactly 160 bytes

RDEF field offsets in **bytes** (not inferred padding):

| Offset | Native field |
| ---: | --- |
| 0 | `vPositionInView` (float3) |
| 12 | `fShadowMapShrink` |
| 16 | `vColor` (float3) |
| 28 | `fClipRange` |
| 32 | `vFalloff` (float4) |
| 48 | `vDirectionInView` (float3) |
| 60 | `fInvShadowMapRange` |
| 64 | `mViewToShadowClip` (float4x4) |
| 128 | `vShadowAtlasOffsetScale` (float4) |
| 144 | `fRadius` |
| 148 | `fBoundsRadius` |
| 152 | `fScatterIntensityMul` |
| 156 | `uTechniqueProperties` (uint) |

## `DeferredLightSpot` — exactly 320 bytes

| Offset | Native field |
| ---: | --- |
| 0 | `vPositionInView` (float3) |
| 12 | `fNearDistance` |
| 16 | `vWedPositionInView` (float3; name from RDEF) |
| 28 | `fFarDistance` |
| 32 | `vDirectionInView` (float3) |
| 44 | `fFarWidth` |
| 48 | `vColor` (float3) |
| 60 | `fClipRange` |
| 64 | `vFalloff` |
| 72 | `fRadius` |
| 76 | `fSinConeAnglePerTwo_sq` |
| 80 | `mViewToProjectionClip` (float4x4) |
| 144 | `mCameraPositionInCone` (float3) |
| 156 | `fInvShadowMapRange` |
| 160 | `mViewToCone` |
| 196 | `fShadowPCFSize` |
| 200 | `fShadowLOD` |
| 204 | `fScatterIntensityMul` |
| 208 | `mConeToView` |
| 256 | `vProjectionAtlasOffsetScale` (float4) |
| 272 | `vShadowAtlasOffsetScale` (float4) |
| 288 | `vDynamicShadowAtlasOffsetScale` (float4) |
| 304 | `uTechniqueProperties` (uint) |
| 308 | `padTo16bytes` |

## Confirmed end-to-end test results (October 10)

- P2 ray shader and its sibling native-root compute PSO execute.
- The quadrant test shows secondary hit coverage, but the actual bounce-2 radiance is weak.
- The copied cyan/magenta checker in raw diffuse GI was **visible in the final image**, with no new compute PSO or descriptor heap binding.
- The previous GI custom compute pass froze immediately after first successful dispatch: **do not restore that path**.
- The current P2 composite chiefly evaluates environment cubemap and emission, not native point/spot/SH/GI inputs. This is a leading explanation for weak interior lighting.

## Implementation requirements / fail-closed policy

1. Prototype a **diagnostic-only** secondary-hit local-light evaluator against the native P2 composite root, starting from the verified point/spot structs. Verify tile/light-index texture packing, light-count bounds, and positions in *view space* before using them for lighting. A valid RDEF binding is not proof of correct indexing or photometry.
2. Use attenuation and, critically, native point/spot shadow visibility; **never** blindly increase light strength or add lights through occluded walls.
3. Measure second-bounce **radiance only**, not the mixed native first-bounce reflection target. Keep a separate P2-only signal for GI routing to avoid double-counting.
4. Preserve Control's native command-list descriptor heaps, root signature, pipeline state, root arguments and native resource tracking. The established **native-copy GI marker** is only a proof, not the final transport algorithm.
5. Verify each step with P2 quadrant radiance / hit coverage and final image, and keep the normal mod branch untouched until stable.

**Do not claim PT local lighting or RR accumulation works based solely on PSO/dispatch markers.** A visible, unclamped scene-local contribution and a confirmed GI result are required.
