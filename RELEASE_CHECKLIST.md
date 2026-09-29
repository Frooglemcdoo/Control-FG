# Release checklist — Control FG v2.1.2

- Build the exact release commit using `Build.cmd` on Windows; source verification, C++ checks, ABI/exports, proxy smoke test and package verification must pass.
- Confirm the source ZIP and deployment ZIP come from that same commit; retain SHA256SUMS.txt and build-validation.json.
- Confirm README, installation instructions and release notes identify Steam, Epic and GOG support and the Galaxy overlay workaround.
- Preserve the known RR indirect diffuse noise issue; v2.1.2 fixes RR VRAM lifetime behavior, not the remaining indirect-diffuse noise artifact.
- Publish the reviewed commit as tag `v2.1.2`, with `RELEASE_NOTES.md`, `Control-FG-v2.1.2.zip`, `Control-FG-v2.1.2-Source.zip`, and `SHA256SUMS.txt`.
- The preparation workflow does not publish or replace existing release assets. Historical release-note workflows must not be used for v2.1.2.

The v2.1.2 candidate carries forward validated Steam/Epic/GOG support and the user-tested September 29 RR/texture changes. Record RTX 40-series 617.14 verification separately when available.
