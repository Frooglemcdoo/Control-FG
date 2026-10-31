# Control The Oldest House: Visual and Performance Overhaul — R258

This is a focused foliage-contract probe, not a visual fix. Installation layout and rendering/storage behavior remain R257. Copy the complete package beside Control_DX12.exe, preserving ControlTOH subfolders.

## One small capture

1. Launch normally and return to the affected plant/cabinet. Use RR On, Native Blend, Clamp Only, Smooth On, GI View Normal.
2. Open F10 and click **CAPTURE MATERIALS** (the diagnostic SAVE LOG slot in this build). This also saves the normal status report.
3. Close F10 and remain at the plant for 10 seconds. No CAPTURE RR or F9 image-buffer capture is required.
4. Run `ControlTOH/Tools/Collect-ControlTOH-Materials.cmd` and send the resulting Desktop ZIP.

The collector copies complete foliage shader pairs, metadata with per-file hashes and the session probe log. It excludes the existing multi-gigabyte RR image buffers. If it reports that no shader pair was captured, send the ordinary compact log package; the new selection report records whether foliage failed before shader selection.

## Why this probe

The R257 buffers show missing textured foliage albedo (approximately 31,000 leaf pixels per frame in the sampled region), with gray F0 fallback. The recurring replay explicitly excludes the foliage family before validating its shader pair. Older evidence contains a real vertex-deformation mismatch, so blindly admitting foliage is not a justified correction. Current native and candidate VS/PS bytecode are needed to preserve geometry, texture selection and alpha/cutout behavior.

The existing material exporter was disabled by the diagnostic image-capture build. The new action requests a foliage-only export independently. No automatic startup capture, no image readbacks, no rendering admission changes. At most one pair copy per preparation frame, 24 unique pairs across 120 preparation frames, four immutable exporter slots. Busy slots never block or overwrite. Disk writes remain on workers. A complete marker follows all four shader files and metadata. Maximum copied shader payload is 24 MiB per request; existing engine-resource lifetime and descriptor-recheck guards remain in place.

Windows checks cover production exporter ownership/bounds/expiry/failures and foliage-only selection, the collector's real ZIP contents, path spaces, incomplete/truncated rejection, and unchanged baseline/render/storage/menu tests. The actual plant contract still requires the user's runtime shader capture.
