"""Audit captured material variants against manually reviewed surface boundaries.

No shaders are patched and no runtime variant is enabled. A successful audit
means the exact captured bytecode matches the recorded offline boundary only.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve()
DECODER = HERE.parents[2] / 'native-integration-audit/tools/decode_dxbc.py'
spec = importlib.util.spec_from_file_location('decode_dxbc', DECODER)
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)


def audit(inventory, shader_dir, boundaries):
    selected = {}
    for view, data in inventory['views'].items():
        for pso, pipeline in data['pipelines'].items():
            for shader in pipeline['shaders']:
                names = {b['name'] for b in shader['bindings'] or []}
                if shader.get('stage') == 'pixel' and {'g_tLightBufferDiffuse', 'g_tLightBufferSpecular'} <= names:
                    selected.setdefault(shader['sha256'], []).append(dict(view=view, pipeline=pso))
    results = {}
    for sha, uses in sorted(selected.items()):
        path = shader_dir / (sha + '.dxbc')
        if hashlib.sha256(path.read_bytes()).hexdigest() != sha:
            raise ValueError(f'Shader content/hash mismatch: {sha}')
        decoded = decoder.decode(path)
        names = {b['name'] for b in decoded['bindings']}
        group = ('character' if 'g_bCharacterLightRigs' in names else
                 'decal' if 'g_sDecalColorMap' in names else 'general_surface')
        instructions = decoded['instructions']
        boundary = boundaries['variants'].get(sha)
        evidence = None
        if boundary:
            combine = next(i for i in instructions if i['dword'] == boundary['combine_dword'])
            fog = next(i for i in instructions if i['dword'] == boundary['fog_start_dword'])
            if combine['name'] != 'mad' or combine['operands'] != boundary['combine_operands']:
                raise ValueError(f'Reviewed combine mismatch: {sha}')
            if fog['name'] not in ('dp3', 'add') or fog['dword'] <= combine['dword']:
                raise ValueError(f'Reviewed fog boundary mismatch: {sha}')
            evidence = dict(combine=combine, fog_start=fog,
                            between=[i for i in instructions if combine['dword'] < i['dword'] < fog['dword']],
                            contract=boundary)
        results[sha] = dict(group=group, uses=uses, instruction_count=len(instructions),
            status='reviewed_offline_candidate' if boundary else 'separate_semantic_review_required',
            custom_data_blocks=sum(i['name'] == 'customdata' for i in instructions),
            discard_offsets=[i['dword'] for i in instructions if i['name'].startswith('discard')],
            output_writes=[i for i in instructions if i['operands'] and i['operands'][0].startswith('o[')],
            evidence=evidence)
    return dict(schema=1, runtime_enabled=False,
                scope='Captured pixel shaders declaring both native light buffers; not all game materials.',
                variants=results)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('inventory', type=Path)
    p.add_argument('shader_dir', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--boundaries', type=Path, default=HERE.parents[1] / 'entrance-surface-boundaries.json')
    args = p.parse_args()
    result = audit(json.loads(args.inventory.read_text()), args.shader_dir,
                   json.loads(args.boundaries.read_text()))
    args.output.write_text(json.dumps(result, indent=2) + '\n')
