"""Static regression gate for P3-A: no native reflection mutation."""
from pathlib import Path
import re
import subprocess

root=Path(__file__).resolve().parents[2]
source=(root/"src/probe.cpp").read_bytes()
base=subprocess.check_output(
    ["git","show","refs/tags/v2.1.2:src/probe.cpp"],cwd=root)
expected=base

def integrate(find: bytes, new: bytes, label: str) -> None:
    global expected
    if expected.count(find)!=1:
        raise SystemExit(f"Baseline contract ambiguous for {label}")
    expected=expected.replace(find,new,1)

integrate(
    b"static bool HookAA(void* t1, void* t2, void* t3, void* t4, void* t5, void* t6,",
    b'// P3-A does not install any new D3D12 hooks or alter the native renderer.\n#include "pt_p3_native_observer.h"\n\n'
    b"static bool HookAA(void* t1, void* t2, void* t3, void* t4, void* t5, void* t6,",
    "P3 observer include")
integrate(
    b"    const bool result = argumentsReady&&originalAA(t1,t2,t3,t4,t5,t6,t7,t8,t9,flag,d1,d2,f1,f2,appliedF3);\n    MarkControlDLSSReady(call, result);",
    b"    const bool result = argumentsReady&&originalAA(t1,t2,t3,t4,t5,t6,t7,t8,t9,flag,d1,d2,f1,f2,appliedF3);\n"
    b"    // Read-only P3-A observer: sample bound native reflection textures and\n"
    b"    // renderer context at the existing AA boundary. No GPU writes/commands.\n"
    b"    P3AObserveAfterAA(call,currentPresent,engineFrame);\n"
    b"    MarkControlDLSSReady(call, result);",
    "P3 observer post-AA call")
integrate(
    b"        verifiedD3d = d3d;\n        verifiedRenderer = renderer;\n        if (!ResolveSemanticGetters(d3d)) {",
    b"        verifiedD3d = d3d;\n        verifiedRenderer = renderer;\n"
    b"        // P3-A traces no secondary rays and composites nothing.\n"
    b"        // Observation uses the existing AA hook; base FG/RR remain intact.\n"
    b"        P3AObserverConfigure();\n"
    b"        if (!ResolveSemanticGetters(d3d)) {",
    "P3 observer verified-target initialization")

if source != expected:
    raise SystemExit("P3-A probe.cpp differs from tag v2.1.2 in more than the three read-only integration locations.")

header=(root/"src/pt_p3_native_observer.h").read_text(encoding="utf-8")
for marker in (
    "P3_A_CONFIG",
    "P3_A_NATIVE_SNAPSHOT",
    "P3_A_COMMAND_CONTEXT",
    "P3_A_UNIMPLEMENTED_FEATURE_REQUEST_BLOCKED",
    "P3AObserveAfterAA(call,currentPresent,engineFrame)",
    "P3AObserverConfigure()",
):
    target=source.decode("utf-8") if marker.startswith("P3A") else header
    if marker not in target:
        raise SystemExit(f"Missing observer contract marker: {marker}")
for code in (
    r"->Dispatch\s*\(",
    r"->SetPipelineState\s*\(",
    r"->SetDescriptorHeaps\s*\(",
    r"->SetComputeRoot",
    r"->ResourceBarrier\s*\(",
    r"->CopyTextureRegion\s*\(",
    r"->ExecuteCommandLists\s*\(",
    r"->CreateCommittedResource\s*\(",
    r"\bTraceRay\s*\(",
    r"\bVirtualProtect\s*\(",
    r"\bDetourAttach\s*\(",
):
    if re.search(code,header):
        raise SystemExit(f"P3 observer unexpectedly attempts a GPU mutation: {code}")
if "nativeShape" not in header or "DepthOrArraySize==2u" not in header:
    raise SystemExit("Expected native two-layer texture shape validation is missing.")
if "GetNativeTexture" not in header and "getNativeTexture@ShaderTexture@" not in header:
    raise SystemExit("No native texture getter bound.")
print("P3_A_STATIC_PASS")
print("native_base=v2.1.2")
print("native_probe_diff=three_observation_only_additions")
print("new_render_hooks=0")
print("gpu_commands=0")
print("native_raygen_swap=0")
print("P3_trace=off P3_composite=off")
