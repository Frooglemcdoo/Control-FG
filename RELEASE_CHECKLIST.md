# Release checklist — Control FG v2.1.1

- Build the exact release commit using `Build.cmd` on Windows; source verification, C++ checks, ABI/exports, proxy smoke test and package verification must pass.
- Confirm the source ZIP and deployment ZIP come from that same commit; retain SHA256SUMS.txt and build-validation.json.
- Confirm README, installation instructions and release notes identify Steam, Epic and GOG support and the Galaxy overlay workaround.
- Preserve the known RR indirect diffuse noise issue; no noise fix is claimed for v2.1.1.
- Publish the reviewed commit as tag `v2.1.1`, with `RELEASE_NOTES.md`, `Control-FG-v2.1.1.zip`, `Control-FG-v2.1.1-Source.zip`, and `SHA256SUMS.txt`.
- The preparation workflow does not publish or replace existing release assets. Historical release-note workflows must not be used for v2.1.1.

Unified R3 was user-tested on Steam, Epic and GOG. Fresh in-game execution of the version-labelled v2.1.1 build is distinct from inherited R3 runtime validation; record it separately if performed.
