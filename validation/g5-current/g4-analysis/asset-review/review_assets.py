#!/usr/bin/env python3
"""Independent byte/numeric review of the supplied G4 diagnostic guide capture."""
import hashlib
import json
import struct
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
CAPTURE = ROOT / "g4-logs/rr-guides/rr-guide-g3-20260913-144531-841-48084"
m = json.loads((CAPTURE / "metadata.json").read_text())
c = json.loads((ROOT / "g4-logs/collection-report.json").read_text("utf-8-sig"))["Captures"][0]
old = json.loads((ROOT / "g4-release/validation/g2-live/shader-equations-summary.json").read_text())
w, h = m["width"], m["height"]
n = w * h
report = {"schema": "ControlFG.G4.AssetReview.v1", "capture": CAPTURE.name,
          "dimensions": [w,h], "pixels": n, "files": []}
for f in c["Files"]:
    p = CAPTURE / f["Name"]
    digest = hashlib.sha256(p.read_bytes()).hexdigest()
    ok = p.stat().st_size == f["Bytes"] and digest == f["SHA256"].lower()
    assert ok, p
    report["files"].append({"name": p.name, "bytes": p.stat().st_size,
                           "sha256": digest, "collector_match": ok})
assert sum(x["bytes"] for x in report["files"]) == c["Bytes"]
assert {p.name for p in CAPTURE.iterdir()} == {x["name"] for x in report["files"]}
assert (CAPTURE / "gbuffer1.rgba8").stat().st_size == n*4
assert (CAPTURE / "gbuffer2.rgba8").stat().st_size == n*4
assert (CAPTURE / "normal-roughness.rgba32f").stat().st_size == n*16
gb1 = np.fromfile(CAPTURE / "gbuffer1.rgba8", dtype=np.uint8).reshape(n,4)
gb2 = np.fromfile(CAPTURE / "gbuffer2.rgba8", dtype=np.uint8).reshape(n,4)
gpu = np.fromfile(CAPTURE / "normal-roughness.rgba32f", dtype="<f4").reshape(n,4)
assert np.isfinite(gpu).all()
assert np.all((gpu[:,3]>=0) & (gpu[:,3]<=1))

# CPU float64 equations operate directly on packed input bytes. No embedded
# shader bytecode, GPU export function, or previous numeric report is executed.
ints = gb1.astype(np.uint32)
u = ((ints[:,0]<<3) | ((ints[:,3]>>1)&7)).astype(np.float64)/2047
v = ((ints[:,1]<<4) | ((ints[:,3]>>4)&15)).astype(np.float64)/4095
qx, qy = 1.3*(2*u-1), 1.3*(2*v-1)
d = qx*qx+qy*qy
view = np.stack((2*qx/(1+d),2*qy/(1+d),(d-1)/(1+d)),axis=1)
view /= np.linalg.norm(view,axis=1)[:,None]
basis = np.array(m["camera"]["view_to_world"][:9]).reshape(3,3)
world = view @ basis
world /= np.linalg.norm(world,axis=1)[:,None]
normalerr = np.abs(gpu[:,:3].astype(np.float64)-world)
roughness = 1-gb1[:,2].astype(np.float64)/255
rougherr = np.abs(gpu[:,3].astype(np.float64)-roughness)
lengths = np.linalg.norm(gpu[:,:3].astype(np.float64),axis=1)
report["normal"] = {
    "nonfinite_pixels": int(np.count_nonzero(~np.isfinite(gpu[:,:3]).all(axis=1))),
    "component_min": gpu[:,:3].min(axis=0).tolist(), "component_max": gpu[:,:3].max(axis=0).tolist(),
    "length_min": float(lengths.min()), "length_max": float(lengths.max()),
    "max_unit_length_error": float(np.abs(lengths-1).max()),
    "outside_metadata_length_tolerance": int(np.count_nonzero(np.abs(lengths-1)>m["statistics"]["normal_unit_length_tolerance"])),
    "reference_max_component_absolute_error": float(normalerr.max()),
    "reference_mean_component_absolute_error": float(normalerr.mean()),
    "reference_pixels_above_1e_6": int(np.count_nonzero(normalerr.max(axis=1)>1e-6)),
}
report["roughness"] = {
    "nonfinite_pixels": int(np.count_nonzero(~np.isfinite(gpu[:,3]))),
    "outside_range_pixels": int(np.count_nonzero((gpu[:,3]<0)|(gpu[:,3]>1))),
    "min": float(gpu[:,3].min()), "max": float(gpu[:,3].max()),
    "mean": float(gpu[:,3].astype(np.float64).mean()),
    "reference_max_absolute_error": float(rougherr.max()),
    "byte_equation_mismatches": int(np.count_nonzero(np.rint(gpu[:,3]*255).astype(np.int16) != 255-gb1[:,2].astype(np.int16))),
    "unique_values": len(np.unique(gpu[:,3]))}
assert report["normal"]["reference_pixels_above_1e_6"] == 0
assert report["roughness"]["byte_equation_mismatches"] == 0
stats = m["statistics"]
assert stats["pixels"] == n
assert stats["finite_normal_pixels"] == n
assert stats["finite_roughness_pixels"] == n
assert stats["nonfinite_normal_pixels"] == report["normal"]["nonfinite_pixels"]
assert stats["nonfinite_roughness_pixels"] == report["roughness"]["nonfinite_pixels"]
assert stats["normal_length_outside_tolerance"] == report["normal"]["outside_metadata_length_tolerance"]
assert stats["roughness_outside_range"] == report["roughness"]["outside_range_pixels"]
for key1,key2 in [("normal_length_min","length_min"),("normal_length_max","length_max"),("max_normal_unit_length_error","max_unit_length_error")]:
    assert abs(stats[key1]-report["normal"][key2]) < 1e-14
assert stats["roughness_min"] == report["roughness"]["min"]
assert stats["roughness_max"] == report["roughness"]["max"]
report["all_metadata_statistics_match"] = True

mid = (gb2[:,2].astype(np.uint32)<<8)|gb2[:,3]
ids, counts = np.unique(mid,return_counts=True)
top = np.argsort(counts)[-10:][::-1]
report["material_ids"] = {"unique": len(ids), "minimum": int(ids.min()), "maximum":int(ids.max()),
    "top": [{"id":int(ids[k]),"pixels":int(counts[k])} for k in top],
    "is_albedo": False}
report["raw_inputs"] = {"gbuffer1_zero_pixels": int(np.count_nonzero(~gb1.any(axis=1))),
    "gbuffer2_zero_pixels": int(np.count_nonzero(~gb2.any(axis=1))),
    "both_zero_pixels":int(np.count_nonzero(~gb1.any(axis=1)&~gb2.any(axis=1))),
    "gbuffer1_alpha_bit0_pixels":int(np.count_nonzero(gb1[:,3]&1)),
    "background_validity_proven":False}
report["camera"] = {"basis_gram_matrix":(basis@basis.T).tolist(), "determinant":float(np.linalg.det(basis)),
    "max_orthonormal_error":float(np.abs(basis@basis.T-np.eye(3)).max()),
    "engine_frame":m["engine_frame"], "camera_engine_frame":m["camera_engine_frame"],
    "present_token":m["present_token"], "temporal_alignment_independently_proven":False}

expect = {
    # Preview export is explicitly float32, unlike the independent float64
    # scientific decode above. Preserve those half-integer rounding boundaries.
    "world-normal.bmp": np.floor(np.clip(gpu[:,:3]*np.float32(.5)+np.float32(.5),0,1)*np.float32(255)+np.float32(.5)).astype(np.uint8),
    "roughness.bmp": np.repeat(np.floor(gpu[:,3]*np.float32(255)+np.float32(.5)).astype(np.uint8)[:,None],3,axis=1),
    "material-id.bmp":np.stack(((mid&255)^0x5a,(mid>>8)^0xa5,(mid*73+(mid>>8)*151+31)&255),axis=1).astype(np.uint8)}
report["bmp"] = {}
for name, rgb in expect.items():
    raw = (CAPTURE/name).read_bytes()
    sig,size,off = raw[:2],struct.unpack_from("<I",raw,2)[0],struct.unpack_from("<I",raw,10)[0]
    bw,bh,planes,bits,compression = struct.unpack_from("<iiHHI",raw,18)
    assert sig==b"BM" and size==len(raw) and (bw,bh,planes,bits,compression)==(w,-h,1,24,0)
    stride=(3*w+3)&~3
    actual=np.frombuffer(raw,dtype=np.uint8,offset=off).reshape(h,stride)[:,:3*w].reshape(n,3)[:,::-1]
    mismatch=int(np.count_nonzero(actual!=rgb))
    assert mismatch == 0, (name,mismatch)
    report["bmp"][name]={"bytes":len(raw),"top_down":True,"channel_mismatches":mismatch}

oldbyname={s["file"]:s for s in old["shaders"]}
report["shader_identity"]=[]
for sh in m["native_albedo_shaders"]:
    data=(CAPTURE/sh["file"]).read_bytes()
    digest=hashlib.sha256(data).hexdigest()
    fnv=0xcbf29ce484222325
    for byte in data: fnv=((fnv^byte)*0x100000001b3)&0xffffffffffffffff
    assert f"{fnv:016x}" == sh["fnv1a64"]
    assert digest==oldbyname[sh["file"]]["sha256"]
    assert data[:4]==b"DXBC" and struct.unpack_from("<I",data,24)[0]==len(data)
    report["shader_identity"].append({"file":sh["file"],"sha256":digest,"bytes":len(data),
        "fnv1a64_matches_metadata":True,"identical_to_G2_equation_review":True,
        "source_frame":sh["source_frame"],"declared_render_target_count":sh["declared_render_target_count"]})
assert not m["has_native_albedo_outputs"] and m["native_albedo_output_count"] == 0
assert not m["native_albedo_images"]
assert all(v==0 for v in m["native_albedo_capture_counts"].values())
report["native_material_output"]={"has_outputs":False,"count":0,"counts":m["native_albedo_capture_counts"],
    "new_shader_semantics_evidence":False,"new_material_coverage_evidence":False}
report["limitations"]=[
    "Arithmetic and byte consistency do not independently re-prove game shader semantics.",
    "Applying the recorded camera rotation correctly does not prove its coordinate convention or exact temporal alignment.",
    "This is one unmasked diagnostic frame with no depth/visibility evidence; material coverage and background correctness remain unverified.",
    "No native material target images, diffuse/specular albedo, hit-distance, or RR evaluation are present.",
    "Pixel-shader copies came from earlier preflight frame 1754; base export and camera metadata refer to frame 1755.",
    "All pixel-shader bytes match the previous G2 equation-reviewed identities, so decoding them again would add no new semantics evidence."]
(OUT/"numeric-review.json").write_text(json.dumps(report,indent=2)+"\n")
print(json.dumps({"status":"PASS_NUMERIC_AND_BYTE_CONSISTENCY","pixels":n,
    "normal_error":report["normal"]["reference_max_component_absolute_error"],
    "roughness_error":report["roughness"]["reference_max_absolute_error"],
    "unique_material_ids":len(ids),"material_images":0,"identical_prior_shaders":3},indent=2))
