# R49 RTX 40-series provider selection fix

The September 30 RTX 4080 / driver 617.14 logs report the bundled provider as
patched, but NGX loads DLSS-G 310.9.1 from its model cache. The runtime repeatedly
rejects a generated-frame count of three as unsupported. Successful SetOptions
and reported frame counts therefore do not prove successful NGX evaluation.

R49 resolves loaded DLSS-G implementations after device initialization and before
FG feature creation. Export identity excludes unrelated SR/RR cache modules.
A unique external implementation takes precedence over the bundled provider.
The device/architecture gates and temporal descriptor correction target the same
pinned implementation. Unknown versions, ambiguous candidates and enumeration
failures reject readiness. Existing temporal hash/layout checks remain intact.
No NVIDIA files on disk are modified. RTX 50-series does not load this sidecar.

The package includes a rebuilt ControlFGStreamline/ControlFG.RTX40MFG.dll;
replacing only dxgi.dll does not install this fix. Restart the game after updating.
The log now records CONTROL_MFG_PROVIDER_SELECTED with the module path.

Validation executes the production bridge with mocked Windows/provider calls,
including cache selection, SR/RR exclusion, ambiguity, unsupported providers,
lookup failures, temporal retries and repeat calls. Windows compilation is a
separate gate. RTX 40 hardware confirmation remains required; no hardware success
is claimed from the mock tests or reported presentation counts.
