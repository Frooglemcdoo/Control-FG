# R50: preserve native auxiliary lighting filters

Experimental candidate, not an in-game verified fix. Based on R49.

The screenshots show bright stippling on illuminated concrete, a dark cabinet,
and scattered papers. The user reports RR amplifies it while stationary.
Screenshots alone do not identify which lighting component produces the noise.

Code evidence: full RR forced contact-shadow temporal filtering off and spatial
size to zero. It also skipped two broad DeferredLightFiltering calls whenever
sun-shadow or AO bits were present. Those broad bits do not prove the contents
of each filtered surface. R50 preserves the original arguments/calls, giving a
bounded A/B test of these three bypasses as a group. The dedicated GI bypass
and reflection signal path remain unchanged.

This may cost GPU time or introduce double-filtering softness/history trails.
The existing world diffuse guide using material F0 is a separate unresolved
concern; this test does not claim to correct material-guide accuracy.

Validation: exhaustive native argument preservation and all 128 RT-effect bit
combinations, exact baseline exceptions, Windows build and existing CI gates.
In-game acceptance requires matching camera/settings: R49 RR on, R50 RR on,
and RR off, 10-second stationary views followed by slow movement. Inspect stair
lighting, papers, cabinet, lettering, skin and glass. Reject if no improvement
or if motion trails/softness outweigh the reduction. No new public controls.
