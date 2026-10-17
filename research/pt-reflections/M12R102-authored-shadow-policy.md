# M12R102 authored direct-light shadow policy

The confirmed screenshot order is PT first four, native RT last four. PT loses
reception wall cones, much of the planter floor light/shadow contrast, and
reflection strength. R101 was loaded correctly; latest 1996/2040 sparse samples
covered, no light-binding failures; HitGI 1764 requests, 1763 positive, zero
unavailable. Healthy binding does not establish correct light visibility.

The adapter unconditionally traced a shadow ray for every point/spot source.
Native source a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f
loads point flags at byte156 (DWORD11323), tests flag2 at DWORD11476, and enters
the shadow evaluation only at DWORD11536. Spot byte304 is loaded at DWORD5961;
DWORD5986 tests flag2 and mask0x300. Mode0x100 enables PCF and mode0x200 VSM;
other modes default dynamic visibility to one at DWORD7242. Static disabled
also defaults to one. Thus no shadow bits means intentionally unshadowed.

R102 carries that policy with the selected light through RIS. No-shadow sources
return visible only after hemisphere rejection. Enabled point/spot lights and
sun continue using the existing visibility ray; unknown/reserved spot mode
0x300 follows the captured native default unless static flag2 is set. No radius
bias, geometry skipping, intensity scaling, or native atlas binding is added.

This is authored-light compatibility rather than universally physical shadowing.
It can restore contributions that fixture geometry previously blocked, but the
screenshots alone do not prove which individual light flags are present. Native
shadow-map caster sets and light near clipping are still distinct from our rays.

GPU fixtures run six flag sets: 0,2,0x100,0x200,0x300,0x302, checking actual native
record decoding for point/spot/sun and retaining attenuation/GGX numeric checks.
A source audit verifies the exact captured branch constants and caller ordering.
In-game visual confirmation remains pending.

Related primary reference: NVIDIA, Fixing Ray-traced Shadow Content Issues with
NVIDIA RTX-Dev, https://developer.nvidia.com/blog/fixing-ray-traced-shadow-content-issues-with-rtx-dev/
explains why raster-authored scene content and ray-traced shadow visibility can
differ. The flag interpretation above comes from the captured game shader.
