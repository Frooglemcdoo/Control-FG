"""Exact native GI source and spotlight visibility isolation diagnostics."""
import base64
import hashlib
import importlib.util
import json
import struct
import sys
from pathlib import Path
from dxbc_transform import transform_shex

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'research/gi-source-r267'
spec = importlib.util.spec_from_file_location('decoder', ROOT / 'research/native-integration-audit/tools/decode_dxbc.py')
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)

def main():
    out = ROOT / 'build'
    out.mkdir(exist_ok=True)
    header = ['#pragma once', '#include "native_lighting_isolation.h"', 'namespace native_lighting {']
    symbols = []
    for variant, identity in enumerate(json.loads((EVIDENCE / 'manifest.json').read_text())[:8]):
        raw = base64.b64decode((EVIDENCE / f'gi-{variant}.dxbc.b64').read_bytes(), validate=False)
        assert len(raw) == identity['bytes'] and hashlib.sha256(raw).hexdigest() == identity['sha256']
        source = out / f'gi-source-{variant}.dxbc'
        source.write_bytes(raw)
        decoded = decoder.decode(source)
        instructions = decoded['instructions']
        # Each light loop ends with diffuse BRDF / pi followed by its RGB MAD.
        sites = []
        for previous, ins in zip(instructions, instructions[1:]):
            if previous['name'] != 'mul' or previous['operands'][-1] != 'l(0.318309873)' or ins['name'] != 'mad':
                continue
            args = ins['operands']
            if args[0].endswith('.xyz') and args[-1] == args[0] + 'x':
                sites.append(ins['dword'])
        assert len(sites) == 2, (variant, sites)
        # Prove the two sites belong to the point and spot light loops, rather
        # than relying on arithmetic-pattern ordering alone.
        stack = []
        loops = {}
        for ins in instructions:
            if ins['name'] == 'loop':
                stack.append([])
            for body in stack:
                body.append(ins)
            if ins['dword'] in sites:
                assert stack
                loops[ins['dword']] = stack[-1]
            if ins['name'] == 'endloop':
                stack.pop()
        for site, name in zip(sites, ('g_sbDeferredLightPointData', 'g_sbDeferredLightSpotData')):
            binding = next(b for b in decoded['bindings'] if b['name'] == name)
            assert any(ins['name'] == 'ld_structured' and
                       any(arg.startswith(f"t[{binding['identifier']}][") for arg in ins['operands'])
                       for ins in loops[site]), (variant, site, name)
        count = struct.unpack_from('<I', raw, 28)[0]
        start = next(o + 8 for o in struct.unpack_from('<'+'I'*count, raw, 32) if raw[o:o+4] == b'SHEX')
        native_words = list(struct.unpack_from('<'+'I'*((len(raw)-start)//4), raw, start))
        # Final static/dynamic visibility blend immediately controls the spotlight
        # zero-visibility early-out. Replace it before that branch, so fully
        # shadowed samples are tested too. Near-plane and cookie gates stay native.
        visibility = []
        for j, ins in enumerate(instructions[:-3]):
            if ins['name'] != 'movc': continue
            following = instructions[j+1:j+4]
            if [x['name'] for x in following] != ['ne','if','ld_structured']: continue
            if following[0]['operands'][1] != ins['operands'][0]: continue
            if following[0]['operands'][2] != 'l(0)': continue
            if ins not in loops[sites[1]]: continue
            visibility.append(ins['dword'])
        assert len(visibility) == 1, (variant, visibility)
        for mode, selected in enumerate(((sites[0],), (sites[1],), tuple(sites), tuple(visibility)), 1):
            edits = []
            for offset in selected:
                expected = native_words[offset:offset+9]
                if mode == 4:
                    assert expected[0] == 0x09000037  # MOVC, four simple temps
                    replacement = [0x05000036, *expected[1:3], 0x4001, 0x3f800000] + [0x0100003a]*4
                else:
                    assert expected[0] == 0x09000032
                    assert expected[2] == expected[8]
                    replacement = [0x05000036, *expected[1:3], *expected[7:9]] + [0x0100003a]*4
                edits.append((offset, expected, replacement))
            def transform(words):
                for offset, expected, replacement in edits:
                    assert words[offset:offset+9] == expected
                    words[offset:offset+9] = replacement
            changed = transform_shex(raw, transform)
            target = out / f'gi-source-{variant}-{mode}.dxbc'
            target.write_bytes(changed)
            assert len(changed) == len(raw)
            permitted = set(range(4, 20))
            for offset, _, _ in edits:
                permitted.update(range(start+offset*4, start+(offset+9)*4))
            assert all(a == b for n, (a, b) in enumerate(zip(raw, changed)) if n not in permitted)
            assert decoder.decode(target)['bindings'] == decoded['bindings']
            def undo(words):
                for offset, expected, _ in edits: words[offset:offset+9] = expected
            assert transform_shex(changed, undo) == raw
            symbol = f'kGISource{variant}Mode{mode}'
            for number, (_, expected, replacement) in enumerate(edits):
                for suffix, words in (('Expected', expected), ('Replacement', replacement)):
                    header.append(f'inline constexpr uint32_t {symbol}{suffix}{number}[]={{'+','.join(hex(v)+'u' for v in words)+'};')
            header.append(f'inline constexpr InstructionEdit {symbol}Edits[]={{'+','.join('{'+f'{start+offset*4},{symbol}Expected{n},{symbol}Replacement{n},9'+'}' for n, (offset, _, _) in enumerate(edits))+'};')
            header.append(f'inline constexpr unsigned char {symbol}Checksum[]={{'+','.join(map(str, changed[4:20]))+'};')
            header.append(f'inline constexpr ShaderPatch {symbol}={{"GI source {variant} mode {mode}",{len(raw)},"{identity["sha256"]}","{hashlib.sha256(changed).hexdigest()}",{start+edits[0][0]*4},{symbol}Expected0,9,nullptr,0,{symbol}Checksum,{symbol}Edits,{len(edits)}}};')
            symbols.append('&'+symbol)
        print(f'PASS GI variant {variant}: point MAD {sites[0]}, spot MAD {sites[1]}, visibility {visibility[0]}; all unrelated bytes preserved')
    header.append('inline constexpr const ShaderPatch* kGISourcePatches[8][4]={'+','.join('{'+','.join(symbols[i:i+4])+'}' for i in range(0,32,4))+'};')
    header.append('}')
    text = '\n'.join(header)+'\n'
    path = ROOT / 'src/gi_source_patches.generated.h'
    if '--check' in sys.argv:
        assert path.read_text() == text
    else:
        path.write_text(text)

if __name__ == '__main__':
    main()
