# Building Control FG v2.1.0

Extract the source to a fresh directory on Windows. Use Visual Studio 2026 C++ build tools and the Windows SDK.

1. Run `Build.cmd`. It stages the pinned dependencies, runs source/build checks, and produces `build/dxgi.dll`, `build/ControlFG.RTX40MFG.dll`, and `build/build-validation.json`.
2. Run `Make-DropIn.cmd` after a successful build. The public package is `release/Control-FG-v2.1.0.zip`.
3. Install and smoke-test that package in Control DX12. See INSTALL.md and KNOWN_ISSUES.md.
4. Publish the ZIP in a new GitHub release tagged `v2.1.0`, using RELEASE_NOTES.md. Retain the generated SHA256SUMS and build-validation.json with the build records.

Do not replace the v2.0.0 asset or write these notes into GITHUB_RELEASE.md on main: the existing notes workflow targets v2.0.0.

Public packaging uses v2.1.0; internal build identifiers retain the tested CS5 lineage for existing validation contracts. The unsuccessful GI1 experiment is excluded. This source package needs Windows compilation before it can be installed.

The retained RR_NATIVE_G12_R20P.md is an input to a historical validation script, not current build guidance. Other historical milestone Markdown has been removed; runtime source, tests, and license notices are retained.
