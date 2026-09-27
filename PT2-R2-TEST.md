# PT2 R2 — Proxy-Aware Late Composite

The PT2 runtime log proved the custom RayQuery pass was executing at 2560x1440, but every presentation composite fail-closed with:

`PT2_COMPOSITE_SKIP ... reason=device_mismatch`

The queue returns a Streamline-wrapped device while the PT ray owner holds the native Control device. PT2 R1 compared their COM identities directly and therefore rejected two addresses that represent the same underlying D3D12 device.

PT2 R2 uses the same Streamline native-interface canonicalization already proven by the FG UI private-work path before comparing devices.

## Test
- Open F10.
- Confirm PT OFF looks exactly like native Control.
- Turn PT ON with HIT/MISS selected.
- The final presentation should become an unmistakable black/white diagnostic.
- Try DISTANCE and INSTANCE.
- Toggle PT OFF again and confirm immediate return to native Control.

Expected:
- PT2_DISPATCH_OK
- PT2_COMPOSITE_READY
- PT2_COMPOSITE_OK
- PT2_COUNTERS
- no repeating device_mismatch skips
