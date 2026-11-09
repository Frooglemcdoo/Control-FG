#!/usr/bin/env python3
"""Static contract audit for Control's native rt_reflection DXIL family.

This is intentionally independent of the runtime hook. It proves the source
facts that the experimental PT reflection path relies on before any custom
library is eligible for runtime use.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LL_ROOT = ROOT / "research" / "hit-distance" / "evidence" / "native-writer"
DXIL_ROOT = ROOT / "research" / "hit-distance" / "evidence" / "original-shaders"
HASHES = ROOT / "research" / "hit-distance" / "tests" / "native-shader-hashes.json"
RUNTIME = ROOT / "validation" / "resolution" / "r20j-runtime.json"

FUNCTIONS = (
    "reflectionRayGeneration",
    "reflectionClosestHit",
    "reflectionAlphaTestAnyHit",
    "reflectionMiss",
    "shadowClosestHit",
    "shadowAlphaTestAnyHit",
    "shadowMiss",
)


def function_body(text: str, name: str) -> str:
    marker = f'define void @"\\01?{name}@@'
    start = text.find(marker)
    if start < 0:
        raise AssertionError(f"missing function {name}")
    nxt = text.find("\ndefine ", start + len(marker))
    return text[start:] if nxt < 0 else text[start:nxt]


def main() -> int:
    expected_hashes = json.loads(HASHES.read_text(encoding="utf-8"))
    assert len(expected_hashes) == 32

    variants = []
    for index in range(32):
        stem = f"rt_reflection-{index:03d}"
        ll_path = LL_ROOT / f"{stem}.ll"
        dxil_path = DXIL_ROOT / f"{stem}.dxbc"
        assert ll_path.is_file(), ll_path
        assert dxil_path.is_file(), dxil_path

        raw = dxil_path.read_bytes()
        actual_sha = hashlib.sha256(raw).hexdigest()
        expected_sha = expected_hashes[dxil_path.name]
        assert actual_sha == expected_sha, (dxil_path.name, actual_sha, expected_sha)

        text = ll_path.read_text(encoding="utf-8")
        hit_type = re.search(r"%struct\.HitData\s*=\s*type\s*\{([^\n]+)\}", text)
        assert hit_type and hit_type.group(1).strip() == "i32", stem

        bodies = {name: function_body(text, name) for name in FUNCTIONS}
        raygen = bodies["reflectionRayGeneration"]
        closest = bodies["reflectionClosestHit"]
        miss = bodies["reflectionMiss"]
        any_hit = bodies["reflectionAlphaTestAnyHit"]

        assert "dx.op.traceRay.struct.HitData" in raygen, stem
        assert "g_uRTReflectionRayCount" in text, stem
        assert "g_rtScene" in text, stem

        full_writer = index % 2 == 0
        if full_writer:
            assert "g_rwtMaterialId" in text, stem
            assert "g_rwtNormal_TexcoordX" in text, stem
            assert "g_rwtPosition_TexcoordY" in text, stem
            assert "dx.op.textureStore.i32" in closest, stem
            assert "dx.op.textureStore.f32" in closest, stem
            assert "1.000000e+04" in closest, stem
            assert "store i32 %131" in closest or "fptoui" in closest, stem
            assert "i32 65535" in miss, stem
            assert re.search(r"store i32 0, i32\* %\d+", miss), stem
            assert "dx.op.ignoreHit" in any_hit, stem
        else:
            assert "dx.op.textureStore" not in closest, stem
            assert "dx.op.textureStore" not in miss, stem
            assert "dx.op.ignoreHit" not in any_hit, stem
            assert "ret void" in closest and "ret void" in miss and "ret void" in any_hit, stem

        variants.append(
            {
                "variant": index,
                "sha256": actual_sha,
                "bytes": len(raw),
                "full_hit_writer": full_writer,
                "hit_payload": "uint32",
                "raygen_trace": True,
                "hit_distance_encoding": "RayTCurrent*10000" if full_writer else None,
                "miss_payload": 0 if full_writer else None,
                "miss_material_id": 65535 if full_writer else None,
            }
        )

    runtime_text = RUNTIME.read_text(encoding="utf-8")
    layer_matches = [int(x) for x in re.findall(r"RR_REFLECTION_PREPARED[^\n]*layers=(\d+)", runtime_text)]
    assert layer_matches, "no validated reflection layer count"
    assert set(layer_matches) == {4}, layer_matches

    result = {
        "schema": "control-pt-reflection-native-audit-v1",
        "status": "PASS",
        "variants": variants,
        "variant_count": 32,
        "full_writer_variants": [v["variant"] for v in variants if v["full_hit_writer"]],
        "stub_variants": [v["variant"] for v in variants if not v["full_hit_writer"]],
        "payload_contract": {
            "type": "uint32",
            "input": "reflection output array layer",
            "hit": "RayTCurrent*10000",
            "miss": 0,
        },
        "hit_uavs": {
            "material": "u0 R16_UINT array",
            "normal_texcoord_x": "u1 RGBA16_FLOAT array",
            "position_texcoord_y": "u2 RGBA16_FLOAT array",
        },
        "validated_runtime_layers": 4,
        "p2_layer_plan": {
            "layer0": "path0_bounce1",
            "layer1": "path0_bounce2",
            "layer2": "path1_bounce1",
            "layer3": "path1_bounce2",
        },
        "runtime_custom_library_admitted": False,
        "reason": "P2 hit/alpha material reconstruction remains intentionally blocked",
    }

    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "pt-reflection-native-audit.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {out}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        import traceback
        print("PT_REFLECTION_NATIVE_AUDIT_FAILURE:", repr(exc), file=sys.stderr)
        traceback.print_exc()
        failure = ROOT / "build" / "pt-reflection-native-audit-failure.txt"
        failure.parent.mkdir(parents=True, exist_ok=True)
        failure.write_text(traceback.format_exc(), encoding="utf-8")
        raise
