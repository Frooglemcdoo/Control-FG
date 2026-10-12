PT multi-bounce transport GPU test M3

Extract all files together. Run Run-PT-Path-Test.cmd. Send Path-Test-Results.txt.
No Control, PIX, Python, or administrator permission required.
The launcher runs the M2 scene-query regression followed by M3 transport tests.

M2 passed all eight tests on the user's RTX 5090 on 2026-10-12.
M3 checks one/two/four/eight surface contributions with RGB throughput, a visible
point light, an occluded point light, an environment miss and invalid input.
CPU expected RGB is independent of shader output. Each case runs 64 seeded paths.
A pair of large facing triangles supplies a known geometric-series solution.
This is a numerical test, not a visual demo or playable Control PT build.

The reusable integrator currently admits opaque diffuse/GGX materials, coincident
geometric/shading normals, delta-light NEE and BSDF-only emission/environment.
The fixture uses diffuse materials; GGX and roulette have CPU component tests but
are not exercised in the M3 GPU path cases. Max depth is a deliberate truncation.
The .001 origin offset is for this controlled fixture only. Native integration
still requires validated geometric normals, robust offsets, material/light
adapters, emitter MIS, output composition and denoising. No native cached GI is
used as a secondary-light sample. No changes are made to installed game files.
