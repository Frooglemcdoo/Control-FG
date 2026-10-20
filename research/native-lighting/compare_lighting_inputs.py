"""Inspect the verified native HIGH event-418 inputs against event-543 lighting.

Usage: python compare_lighting_inputs.py INPUT_DIRECTORY BUFFER_DIRECTORY OUTPUT
Requires numpy, Pillow and scipy. Outputs are analysis, not game replacements.
Normal packing follows captured PSO3648 instructions 249-422. Material indices
follow instructions 437-459. Local regressions are descriptive, not causal tests.
"""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter


PATCHES = {
    "box_upper": (2080, 800, 70, 40),
    "box_lower": (2100, 860, 45, 20),
    "wall_panel": (2100, 640, 25, 15),
}


def decode_normals(rgba):
    packed = rgba.astype(np.uint32)
    # R holds the high 8 bits of an 11-bit coordinate. G holds the high
    # 8 bits of a 12-bit coordinate. Alpha bit 0 is not part of the normal.
    x_bits = (packed[..., 0] << 3) | ((packed[..., 3] >> 1) & 7)
    y_bits = (packed[..., 1] << 4) | ((packed[..., 3] >> 4) & 15)
    x = (x_bits * (2.0 / 2047.0) - 1.0) * 1.3
    y = (y_bits * (2.0 / 4095.0) - 1.0) * 1.3
    radius_squared = x * x + y * y
    normal = np.stack((2 * x, 2 * y, radius_squared - 1), axis=-1)
    return normal / (radius_squared + 1)[..., None]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", type=Path)
    parser.add_argument("buffers", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    gbuffer = np.array(Image.open(args.inputs / "418-gbuffer1_packed_normals.png"))
    material = np.array(Image.open(args.inputs / "418-gbuffer2_material_data.png"))
    if gbuffer.shape != (1440, 2560, 4) or material.shape != gbuffer.shape:
        raise ValueError("Expected native HIGH 2560x1440 RGBA inputs")
    raw = (args.inputs / "418-gbuffer1_packed_normals.dds").read_bytes()
    if raw[:4] != b"DDS " or len(raw) != 128 + gbuffer.size:
        raise ValueError("Unexpected GBuffer1 DDS layout")
    if not np.array_equal(np.frombuffer(raw[128:], np.uint8).reshape(gbuffer.shape), gbuffer):
        raise ValueError("PNG channels differ from captured DDS bytes")
    raw = (args.buffers / "543-after_deferred_lighting_draw.dds").read_bytes()
    if raw[:4] != b"DDS " or raw[84:88] != b"q\0\0\0" or len(raw) != 128 + gbuffer.size * 2:
        raise ValueError("Expected legacy A16B16G16R16F lighting DDS")
    light = np.frombuffer(raw[128:], "<f2").reshape(gbuffer.shape).astype(float)
    luminance = light[..., :3] @ np.array([0.2126, 0.7152, 0.0722])
    normals = decode_normals(gbuffer)
    if not np.allclose(np.linalg.norm(normals, axis=-1), 1, atol=1e-6):
        raise ValueError("Decoded normal lengths are invalid")
    report = {"scope": "One frame; local linear association does not prove causation", "patches": {}}
    for name, (x, y, width, height) in PATCHES.items():
        region = np.s_[y:y + height, x:x + width]
        normal = normals[region]
        intensity = luminance[region]
        roughness = gbuffer[region][..., 2].astype(float)
        normal_detail = normal - gaussian_filter(normal, (2, 2, 0))
        roughness_detail = roughness - gaussian_filter(roughness, 2)
        light_detail = intensity - gaussian_filter(intensity, 2)
        design = np.column_stack((normal_detail.reshape(-1, 3), roughness_detail.ravel(), np.ones(width * height)))
        fit = design @ np.linalg.lstsq(design, light_detail.ravel(), rcond=None)[0]
        values = material[region]
        identifiers = (values[..., 2].astype(np.uint32) << 8) | values[..., 3]
        report["patches"][name] = {
            "rectangle_xywh": [x, y, width, height],
            "material_ids": np.unique(identifiers).tolist(),
            "gbuffer2_unique_rgba": np.unique(values.reshape(-1, 4), axis=0).tolist(),
            "lighting_detail_std_over_mean": float(light_detail.std() / intensity.mean()),
            "normal_roughness_local_linear_r_squared": float(1 - np.var(light_detail.ravel() - fit) / np.var(light_detail)),
        }
    args.output.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.uint8(np.clip(normals * 0.5 + 0.5, 0, 1) * 255)).save(args.output / "decoded-normals.png")
    (args.output / "comparison.json").write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
