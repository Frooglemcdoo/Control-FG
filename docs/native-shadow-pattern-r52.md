# R52: isolate spotlight shadow sampling noise

Diagnostic, based on R49. R50/R51 rendering changes are not included.

R51 user result: the visible pattern remains. Compact log
`probe-20261020-173530-070-8628.log` confirms enabled=1, matched=1, applied=1,
HRESULT 0 and the expected patched SHA. Removing output dither was therefore
insufficient; this does not prove it makes no contribution.

The same captured non-RT lighting shader samples `g_tBlueNoiseShadowJitter`
at t5, then maps RGBA to [-1,1] at SHEX dword 392. At dword 6127 that value
scales the spotlight PCF sampling basis, followed by sample coordinates formed
from pairs of kernel constants and reads of `g_tSpotLightPCFShadowAtlas` at t12.
The jitter register is overwritten with material data at dword 10404. This
experiment changes only its initial basis, not the later material loads.

R52 changes the MAD's multiplier to zero and bias to (1,0,0,1). This supplies
fixed unit axes instead of the random basis. It retains the PCF tap count,
weights, comparisons and the ordinary output dither. It does not collapse
all shadow samples onto the center. Filter shape/softness and aliasing can
change, so this is an isolation test rather than a confirmed quality fix.

Native SHA-256 (69,704 bytes):
`bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff`
Patched SHA-256:
`ce87eef29ac053359c9c4ac5ce916174120169c36c07aa1dddddf05eea553eb6`

The complete shader hash and instruction must match. The patched checksum and
SHA are verified. Driver rejection falls back to native creation. Unsupported
variants pass through, including RT variants. The existing compute PSO hook is
reused; no new GPU pass or resource is added.

Local validation: C++ patch equals an independent byte-level patch; disassembly
has exactly one instruction difference; resource bindings/constant metadata
are unchanged; checksum and the 132-file baseline audit pass. Windows CI builds
and runs existing validation separately. No local NVIDIA/visual validation.

Install with Control closed. The included `ControlFG-Fixed-Shadow-Pattern.txt`
enables the diagnostic. Rename it to .disabled and restart for native behavior.
Old R51 markers are ignored. Start with RT/DLSS/RR/FG/glass OFF at the same pose.
Compare stationary speckles, shadow edges and slow camera movement. Logs should
show `NATIVE_SHADOW_PATTERN_R52 matched=1 applied=1`. Send comparison images and
compact logs; no new PIX capture is required for this test.
