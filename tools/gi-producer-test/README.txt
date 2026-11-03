Control owned GI producer hardware test

Extract the entire ZIP to a folder, then double-click Run-GI-Producer-Test.cmd.
No game launch, installation, administrator access, or capture is required.
Send GI-Producer-Result.txt back whether it reports PASS or FAIL.

This exercises the new private DXR shader table and accepted-hit payload,
R16 alpha-tested and R32 opaque geometry, materials, normals, UVs, true hit
position, finite-range sampling density, misses, invalid input and output bounds.
It runs the actual producer shader and table/pipeline implementations.

The earlier visibility/receiver test did not execute this producer.
This is a standalone hardware check, not a full GI replacement game build.
Live scene integration and spatial/temporal reuse are not validated here.
