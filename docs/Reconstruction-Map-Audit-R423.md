# Reconstruction-map capture audit

R422 is rejected. R423 restores character filter/test files byte-for-byte from
R421 commit 84d6df15c5f5b90854f31174baf418e9576807ef. No smoothing candidate is added.

Existing renderer_rmdwin10_f.dll contains g_sGBufferReconstruction at RVA
0x6295E0. Function RVA 0xC3C60 registers that name, constructs ShaderTexture at
0x1292EE0 (LEA at 0xC3C96), and calls its setNativeTexture export at 0xC3D62.
The resource owner is GBufferProcessor field +0x20, assigned at 0xC3C07 after
creation bearing name GBufferProcessor::m_pGBufferReconstruction (0x629590).
Creation passes native PixelFormat 9 at instruction 0xC3BCF.

The supplied d3d_rmdwin10_f.dll export nativeFormat at RVA 0x23990 indexes
20-byte records at RVA 0x62420. Entry 9 is [42,42,42,0,0], confirming the
native resource/view mapping to DXGI_FORMAT_R32_UINT. Imports at renderer
IAT RVAs 0x5DFC70 and 0x5DFC90 identify the ShaderTexture constructor and
setNativeTexture; this is not an inferred address from proximity to another
texture global.

R423 queries that exact object with the existing read-only getNativeTexture
export, only while capture is pending. Existing tracked-state, native flags,
resource lifetime, same-device and GPU retirement checks apply. The format
must be R32_UINT, with a single mip/layer/sample and enough texels to address
the input pixels shifted right by one. Allocation may include padding, so
actual dimensions are recorded. Every raw 32-bit map word is copied directly,
without float conversion or decoding in the game. The worst-case capture
capacity increases from 17 to 18 resources; existing lighting indices stay
fixed. The map uses native index 8. No runtime sampling or rendering is changed.

Limitations: this observes the named resource at the RR boundary, not at each
character draw. Coverage selectors are not captured. The file must not be
presented as proof of per-fragment source correspondence before checking those
conditions. It does make the packed selectors available for offline examination,
unlike R421/R422's identity-map approximation. No claim of a visual fix or
performance gain is made.
