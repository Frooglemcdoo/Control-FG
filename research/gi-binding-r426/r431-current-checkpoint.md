# R431 native mipmapped output correction

User R430 result: performance cost seems fixed; speckles persist. Exact new
session171852/PID61420 in compact bundle111958 has eight output_acquisition
failures then stops. Output2560x1440, mips12, samples1, UAV state8, flags5,
alias0. The source required MipLevels==1 both during Retain and completion.
See r430-session-summary.json; the older R429 session also bundled is not R430.

## Native binding proof

Exact SHA-verified binaries in r431-native-output-mip.asm:
- renderer12C621 prepares DiffuseGITarget at90E9F8 as UAV;12C68B binds its
  NativeTexture to g_rwtDiffuseGIColorTarget via ShaderTexture.
- D3D416C2 type4 ShaderTexture uses NativeTexture+A8 default UAV handle.
- D3D3A12E sets descriptor-factory mip argument to zero;39DAD loads it and
  39F1E writes Texture2D.MipSlice before CreateUnorderedAccessView at39F76.
- Production preserves the native descriptor tables; no replacement output
  UAV is manufactured. Existing exact-build scene/resource lease applies.

## Correction and checks

Allow a nonempty mip chain only for output retention and guided output
admission. Three hit textures retain strict single-mip validation. Keep array,
sample, dimensions, device, native alias, UAV state/flags and identity checks.
No algorithm, ray-count, RR/RELAX or budget change. Eight-failure stop remains.

GPU regression: native output wrapper now uses5 mip levels at18x3; acquisition
must pass while the same mipmapped resource as a ray-hit texture must fail.
Generated evaluator uses mipmapped color with native-equivalent mip0 UAV;
mip1 is initialized to a sentinel and checked unchanged after both phases.
This validates shader subresource writes, not live RTX quality/performance.

Source15329087433debee1b614b3e34ffcc65b040bea9.
Core43269235657/job131360980490 SUCCESS; full43269235727/job131360979456 SUCCESS. No live guided=1 proof yet.
When ready, user needs motion/FPS check at known GI-lit jaw/hair locations and
compact logs. No repeat R425-427 ownership capture, PIX or new capture harness.
Enabling successful guided work can have real cost beyond R430's stopped path;
do not promise unchanged FPS or fixed speckles before their test.

Verified artifact13966649999; package 6992245 bytes, 88 members.
ZIP SHA256: 54f486eef069215667822b28327124fd6a7797b4dd902ad80d30f2a219b9f35d
DLL SHA256: 5cef7a85cc21cea225741e8432ba270d9bdc46b5166110870fedcb477984e5e0
Launcher and runtime markers verified; mip1 sentinel GPU test passed.
