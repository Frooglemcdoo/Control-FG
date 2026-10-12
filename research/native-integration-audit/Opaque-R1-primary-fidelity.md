# Opaque R1: primary reflection fidelity

The opaque P2 raygen used the full GGX VNDF for the first reflection, including
an alpha floor of 0.02. It discarded below-horizon samples, leaving the first
material ID at the invalid sentinel. This differed from the captured native
raygen even before any secondary lighting replacement.

The native `reflection-000.ll` importance sampler multiplies its second random
coordinate by 0.4 and uses roughness^4 in the GGX inverse CDF. It retries rays
with normal dot direction below 0.005, up to 100 retries. Its origin is
`world + (normal - cameraToSurface) * 0.001 * linearDepth`, with TMin zero.

R1 restores that distribution, view-space tangent basis, retry limit, and origin
contract for the first ray. Retries only compute directions; they do not trace
additional rays. The second-bounce VNDF, its estimator, and its precision fix
remain unchanged. No brightness/color gain or new diffuse transport is added.
G3 glossy glass and the glass diffuse removal remain unchanged.

This is a concrete correction to native/PT ray-footprint and coverage mismatch.
It is not pixel-identical native tracing: P2 still uses its existing noise seed,
one primary sample with the second native slice used for the extra bounce,
extended roughness admission, and existing ray range. Those remaining differences
are not claimed fixed. Material shading/composition is unchanged in this build.

Validation: native-disassembly contract checks, independent angular-support and
origin checks, existing secondary precision/encoding and energy tests, DXC
compilation, and full Windows build. Target-game PT OFF/ON comparison at the
original opaque reflection scene is still needed to establish visual impact.
