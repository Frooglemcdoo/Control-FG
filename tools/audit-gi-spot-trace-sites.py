"""Resolve per-light trace insertion sites from pinned native GI bytecode.

This creates an instrumentation contract, not a shader patch or runtime probe.
Registers are variant-local and must be sampled at the recorded instruction.
"""
import argparse
import base64
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'research/gi-source-r267'
spec = importlib.util.spec_from_file_location(
    'decoder', ROOT / 'research/native-integration-audit/tools/decode_dxbc.py')
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)


def one(items, label):
    if len(items) != 1:
        raise ValueError(f'{label}: expected one match, got {len(items)}')
    return items[0]


def audit(decoded, identity):
    instructions = decoded['instructions']
    binding = one([b for b in decoded['bindings']
                   if b['name'] == 'g_sbDeferredLightSpotData'], 'spot table')
    prefix = f"t[{binding['identifier']}]["
    coefficient = one([i for i in instructions if i['name'] == 'ld_structured'
                       and i['operands'][2] == 'l(68)'
                       and i['operands'][3].startswith(prefix)], 'coefficient load')
    # Resolve the enclosing native loop, not another light family's similar math.
    stack = []
    body = None
    for i in instructions:
        if i['name'] == 'loop':
            stack.append([])
        for loop in stack:
            loop.append(i)
        if i is coefficient:
            body = stack[-1]
        if i['name'] == 'endloop':
            stack.pop()
    if not body:
        raise ValueError('Coefficient load is outside a loop')
    sites = {}

    def record(name, instruction, **fields):
        sites[name] = dict(instruction_shex_dword=instruction['dword'],
                           sample_after_instruction=True,
                           opcode=instruction['name'],
                           operands=instruction['operands'], **fields)

    record('light_coefficients', coefficient,
           light_index=coefficient['operands'][1],
           coefficient_bytes=[68, 72, 76])
    color = one([i for i in body if i['name'] == 'ld_structured'
                 and i['operands'][2] == 'l(48)'
                 and i['operands'][3].startswith(prefix)], 'light color load')
    record('light_color_range', color)
    visibility = one([i for n, i in enumerate(body[:-3])
                      if i['name'] == 'movc'
                      and [x['name'] for x in body[n+1:n+4]] == ['ne', 'if', 'ld_structured']
                      and body[n+1]['operands'][1] == i['operands'][0]
                      and body[n+1]['operands'][2] == 'l(0)'], 'visibility')
    record('shadow_visibility_before_early_out', visibility)
    floor = one([i for i in body if i['name'] == 'max'
                 and i['operands'][-1] == 'l(0.25)'], 'denominator floor')
    k = body.index(floor)
    if [i['name'] for i in body[k-3:k+6]] != [
            'dp3', 'mul', 'mad', 'max', 'mul', 'rcp', 'add', 'max', 'mul']:
        raise ValueError('Unreviewed attenuation instruction chain')
    record('distance_squared', body[k-3])
    record('denominator_before_floor', body[k-1])
    record('attenuation', body[k+4])
    record('range_times_attenuation', body[k+5])
    cookie_binding = one([b for b in decoded['bindings']
                          if b['name'] == 'g_tSpotLightProjectionAtlas'], 'cookie binding')
    cookie = one([i for i in body if i['name'] == 'sample_l'
                  and i['operands'][2].startswith(f"t[{cookie_binding['identifier']}][")], 'cookie sample')
    record('projection_color', cookie, conditional=True)
    contributions = [i for a, i in zip(body, body[1:])
                     if a['name'] == 'mul' and a['operands'][-1] == 'l(0.318309873)'
                     and i['name'] == 'mad'
                     and i['operands'][-1] == i['operands'][0] + 'x']
    contribution = one(contributions, 'spot contribution')
    record('spot_diffuse_accumulation', contribution,
           sample_before_instruction=True,
           diffuse_factor=contribution['operands'][1],
           gated_light_rgb=contribution['operands'][2],
           includes_secondary_albedo=False,
           includes_primary_throughput=False,
           includes_ray_count_normalization=False)
    sites['spot_diffuse_accumulation']['sample_after_instruction'] = False
    if not visibility['dword'] < color['dword'] < floor['dword'] < contribution['dword']:
        raise ValueError('Unreviewed control-flow order')
    return dict(native_sha256=identity['sha256'],
                spotlight_binding=binding, sites=sites,
                native_lighting_modified=False,
                runtime_binding_validated=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = dict(schema='gi-spot-trace-sites-v1', runtime_ready=False, variants=[])
    identities = json.loads((EVIDENCE / 'manifest.json').read_text())[:8]
    with tempfile.TemporaryDirectory() as tmp:
        for variant, identity in enumerate(identities):
            raw = base64.b64decode((EVIDENCE / f'gi-{variant}.dxbc.b64').read_bytes())
            if len(raw) != identity['bytes'] or hashlib.sha256(raw).hexdigest() != identity['sha256']:
                raise ValueError('Unreviewed native shader')
            source = Path(tmp) / 'native.dxbc'
            source.write_bytes(raw)
            decoded = decoder.decode(source)
            contract = audit(decoded, identity)
            contract['variant'] = variant
            result['variants'].append(contract)
            # A missing or duplicated coefficient load must fail closed in the
            # real audit, not merely in a stand-alone cardinality helper.
            offset = contract['sites']['light_coefficients']['instruction_shex_dword']
            for duplicate in (False, True):
                bad = copy.deepcopy(decoded)
                selected = next(i for i in bad['instructions'] if i['dword'] == offset)
                if duplicate:
                    bad['instructions'].append(copy.deepcopy(selected))
                else:
                    bad['instructions'].remove(selected)
                try:
                    audit(bad, identity)
                except ValueError:
                    pass
                else:
                    raise AssertionError('Missing/ambiguous native coefficient site accepted')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: eight pinned GI variants; per-light trace sites resolved; runtime remains disabled')


if __name__ == '__main__':
    main()
