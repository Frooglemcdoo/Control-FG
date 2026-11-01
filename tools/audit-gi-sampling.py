"""Check pinned native ray mapping and the observed GI6 sample-count normalization."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from gi_long_sequence import HASHES, ROOT, transform, edits

def require(source, fragments):
    for fragment in fragments:
        if source.count(fragment) != 1:
            raise ValueError('Unreviewed sampling instruction: ' + fragment)

def audit(dxc):
    results = []
    with tempfile.TemporaryDirectory() as tmp:
        for variant in (0, 1):
            data = base64.b64decode((ROOT / f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64').read_bytes())
            if hashlib.sha256(data).hexdigest() != HASHES[variant]:
                raise ValueError('Native ray shader identity changed')
            path = Path(tmp) / 'native.dxbc'
            path.write_bytes(data)
            source = subprocess.check_output([dxc, '-dumpbin', str(path)], text=True)
            if variant == 0:
                # Producer ABI: the texture position is view-space, not a ready
                # world-space candidate. Misses leave the normal texture unwritten.
                require(source, [
                    '%105 = fadd fast float %RayTCurrent, 0xBF847AE140000000',
                    'float %123, float %125, float %127, float %tmp157, i8 15)',
                    'i32 65535, i32 65535, i32 65535, i32 65535, i8 15)',
                    'float %23, float %25, float %27, float 0.000000e+00, i8 15)',
                ])
            angle, inv, cosine, sine, z = ((237, 238, 239, 240, 'Frc26') if variant == 0
                                          else (297, 298, 299, 300, 'Frc85'))
            require(source, [
                f'%{angle} = fmul fast float %Frc, 0x401921FB60000000',
                f'%{inv} = fsub fast float 1.000000e+00, %{z}',
                f'%{cosine} = fmul fast float %Cos, %Sqrt',
                f'%{sine} = fmul fast float %Sin, %Sqrt',
                f'float 0.000000e+00, float %{z})',
                f'float 0.000000e+00, float %{inv})',
                f'float %{angle})  ; Cos(value)',
                f'float %{angle})  ; Sin(value)',
            ])
            old, new, _, _ = edits(variant)
            assert transform(source, variant).replace(new, old) == source
            results.append(dict(variant=variant, native_sha256=HASHES[variant],
                mapping='x=cos(2*pi*u)*sqrt(1-v), y=sin(2*pi*u)*sqrt(1-v), z=sqrt(v)',
                pdf_for_uniform_uv='cos(theta)/pi',
                r264_changes_phase_only=True))
    # The active hit shader is separately pinned to the bytes captured in gameplay.
    raw = base64.b64decode((ROOT / 'research/gi-source-r267/gi-6.dxbc.b64').read_bytes())
    if hashlib.sha256(raw).hexdigest() != '1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de':
        raise ValueError('GI6 identity changed')
    # Decode bytes instead of trusting the checked-in human-readable assembly.
    from gi_spot_pcf import decoder
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / 'native.dxbc'
        path.write_bytes(raw)
        instructions = decoder.decode(path)['instructions']
    checks = [
        ('ige', ['r[7].w', 'r[0].z', 'cb[4][4][0].z']),
        ('mad', ['r[10].xyz', 'r[3].xyzx', 'r[15].xyzx', 'r[10].xyzx']),
        ('iadd', ['r[0].z', 'r[0].z', 'l(1)']),
        ('mul', ['r[0].xyz', 'r[0].xxxx', 'r[10].xyzx']),
        ('itof', ['r[0].w', 'cb[4][4][0].z']),
        ('div', ['r[1].xyz', 'r[0].xyzx', 'r[0].wwww']),
        ('mul', ['r[12].xyz', 'r[11].yyyy', 'cb[0][0][7].xyzx']),
        ('mad', ['r[12].xyz', 'cb[0][0][6].xyzx', 'r[11].xxxx', 'r[12].xyzx']),
        ('mad', ['r[12].xyz', 'cb[0][0][8].xyzx', 'r[11].zzzz', 'r[12].xyzx']),
        ('add', ['r[12].xyz', 'r[12].xyzx', 'cb[0][0][9].xyzx']),
    ]
    for opcode, operands in checks:
        if sum(i['name'] == opcode and i['operands'] == operands for i in instructions) != 1:
            raise ValueError('Unreviewed GI6 normalization: ' + str((opcode, operands)))
    return dict(schema='gi-sampling-audit-v1', ray_variants=results,
        hit_contract=dict(producer_variant=0, material_miss_sentinel=65535,
            position_storage='view-space xyz, texture-coordinate y in w on hits',
            hit_position_bias='world origin + direction * (RayTCurrent - 0.01), then WorldToView',
            normal_on_miss='not written by miss shader; reject as a surface candidate',
            consumer_position_transform='GI6 cb0 registers 6..9, ViewToWorld',
            limitations='Pinned static instructions, not runtime matrix agreement or visibility validation'),
        gi6_normalization='depth_scale * sum(primary_throughput * secondary_shading) / configured_ray_count',
        limits=['Static shader audit, not proof of runtime constant-buffer agreement.',
                'Cosine PDF assumes uniform input coordinates and a valid orthonormal basis.',
                'No inference that six rays adequately sample a small bright indirect source.',
                'No change to lighting, ray distribution or sample count.'])

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dxc', required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    a.output.write_text(json.dumps(audit(a.dxc), indent=2) + '\n')
    print('PASS pinned ray mappings, reversible R264 edit and GI6 count normalization')
