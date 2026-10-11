#!/usr/bin/env python3
"""Lossless P2 secondary-material sideband test for FP16 textures.

Control's floating-point reflection hit guides may be stored as FP16. The old
asfloat(uintMaterialID) bitcasts generated subnormal floats (~1e-41) which can
be flushed to zero. V8 stores the material index in two numeric components:
normal.w = low8 + 1; position.w = high8.

Neither channel exceeds 256, and both have EXACT IEEE 754 binary16
representations. This offline test exercises every 16-bit native material ID.
"""
from pathlib import Path
import struct


def f16_roundtrip(value: float) -> float:
    return struct.unpack("<e", struct.pack("<e", value))[0]


def main():
    count = 0
    for material_id in range(65534):
        low = f16_roundtrip(float((material_id & 255) + 1))
        high = f16_roundtrip(float((material_id >> 8) & 255))
        assert 1.0 <= low <= 256.0, (material_id, low)
        assert 0.0 <= high <= 255.0, (material_id, high)
        decoded = (round(high) << 8) | (round(low) - 1)
        assert decoded == material_id, (material_id, decoded, low, high)
        count += 1

    assert f16_roundtrip(0.0) == 0.0  # no hit / miss
    # The legacy failure mechanism on binary16:
    raw_bits = struct.unpack("<f", struct.pack("<I", 65534))[0]
    assert f16_roundtrip(raw_bits) == 0.0

    shader = Path("src/shaders/pt_reflection_p2.hlsl").read_text(encoding="utf-8")
    composite = Path("src/shaders/pt_reflection_p2_composite.hlsl").read_text(encoding="utf-8")
    assert "asfloat(uMaterialID)" not in shader, "legacy bitcast ID is back"
    assert "asfloat(65534u)" not in shader, "legacy numeric miss sentinel is back"
    assert "PTDecodeSecondaryMaterial" in composite
    assert "PTDecodeSecondaryMaterial(secondNormalPacked,secondPosPacked)" in composite
    assert "PTDecodeSecondaryMaterial(n,secondaryPosPacked)" in composite
    print(f"PASS: {count} native material IDs survive two-channel FP16 numeric encoding; miss=0; old bitcast underflows")


if __name__ == "__main__":
    main()
