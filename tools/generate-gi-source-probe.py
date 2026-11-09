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
        # Independent spotlight-lighting probes, all built from native bytes.
        cookie_binding = next(b for b in decoded['bindings'] if b['name']=='g_tSpotLightProjectionAtlas')
        cookie_samples = [j for j,x in enumerate(instructions) if x['name']=='sample_l'
                          and x['operands'][2].startswith(f"t[{cookie_binding['identifier']}][")]
        assert len(cookie_samples)==1
        j = cookie_samples[0]
        cookie = instructions[j+1]
        assert cookie in loops[sites[1]] and cookie['name']=='mul'
        assert cookie['operands'][1]==cookie['operands'][0]+'x'
        assert cookie['operands'][2]==instructions[j]['operands'][0]+'x'
        falloffs = [x for x in instructions[j-25:j] if x['name']=='max' and x['operands'][-1]=='l(0)']
        assert len(falloffs)==1 and falloffs[0] in loops[sites[1]]
        k = instructions.index(falloffs[0])
        assert [x['name'] for x in instructions[k-4:k+3]]==['max','mul','rcp','add','max','mul','mul']
        assert instructions[k-4]['operands'][-1]=='l(0.25)'
        assert instructions[k-3]['operands'][-1]=='l(3.14159274)'
        assert instructions[k+1]['operands'][-1]==falloffs[0]['operands'][0]
        fresnels = [x for x in instructions[j:] if x['dword']<sites[1] and x['name']=='add_sat']
        assert len(fresnels)==1 and fresnels[0] in loops[sites[1]]
        k = instructions.index(fresnels[0])
        assert [x['name'] for x in instructions[k-1:k+4]]==['mad','add_sat','mul','mul','mad']
        assert instructions[k+1]['operands'][-1]==fresnels[0]['operands'][0]
        for mode, selected in enumerate(((sites[0],), (sites[1],), tuple(sites), tuple(visibility),
                                         (cookie['dword'],), (falloffs[0]['dword'],), (fresnels[0]['dword'],)), 1):
            edits = []
            for offset in selected:
                length = (native_words[offset] >> 24) & 127
                expected = native_words[offset:offset+length]
                if mode == 5:
                    assert expected[0] == 0x07000038  # RGB cookie MUL
                    replacement = [0x05000036, *expected[1:3], *expected[3:5]] + [0x0100003a]*2
                elif mode in (6,7):
                    # Flatten distance attenuation / bypass secondary Fresnel reduction.
                    assert (expected[0]&0x7ff)==(52 if mode==6 else 0)
                    constant = 0x3ea2f983 if mode==6 else 0x3f800000  # 1/pi or 1
                    replacement = [0x05000036, *expected[1:3], 0x4001, constant] + [0x0100003a]*(length-5)
                elif mode == 4:
                    assert expected[0] == 0x09000037  # MOVC, four simple temps
                    replacement = [0x05000036, *expected[1:3], 0x4001, 0x3f800000] + [0x0100003a]*4
                else:
                    assert expected[0] == 0x09000032
                    assert expected[2] == expected[8]
                    replacement = [0x05000036, *expected[1:3], *expected[7:9]] + [0x0100003a]*4
                edits.append((offset, expected, replacement))
            def transform(words):
                for offset, expected, replacement in edits:
                    assert words[offset:offset+len(expected)] == expected
                    words[offset:offset+len(expected)] = replacement
            changed = transform_shex(raw, transform)
            target = out / f'gi-source-{variant}-{mode}.dxbc'
            target.write_bytes(changed)
            assert len(changed) == len(raw)
            permitted = set(range(4, 20))
            for offset, expected, _ in edits:
                permitted.update(range(start+offset*4, start+(offset+len(expected))*4))
            assert all(a == b for n, (a, b) in enumerate(zip(raw, changed)) if n not in permitted)
            assert decoder.decode(target)['bindings'] == decoded['bindings']
            def undo(words):
                for offset, expected, _ in edits: words[offset:offset+len(expected)] = expected
            assert transform_shex(changed, undo) == raw
            symbol = f'kGISource{variant}Mode{mode}'
            for number, (_, expected, replacement) in enumerate(edits):
                for suffix, words in (('Expected', expected), ('Replacement', replacement)):
                    header.append(f'inline constexpr uint32_t {symbol}{suffix}{number}[]={{'+','.join(hex(v)+'u' for v in words)+'};')
            header.append(f'inline constexpr InstructionEdit {symbol}Edits[]={{'+','.join('{'+f'{start+offset*4},{symbol}Expected{n},{symbol}Replacement{n},{len(expected)}'+'}' for n, (offset, expected, _) in enumerate(edits))+'};')
            header.append(f'inline constexpr unsigned char {symbol}Checksum[]={{'+','.join(map(str, changed[4:20]))+'};')
            header.append(f'inline constexpr ShaderPatch {symbol}={{"GI source {variant} mode {mode}",{len(raw)},"{identity["sha256"]}","{hashlib.sha256(changed).hexdigest()}",{start+edits[0][0]*4},{symbol}Expected0,{len(edits[0][1])},nullptr,0,{symbol}Checksum,{symbol}Edits,{len(edits)}}};')
            symbols.append('&'+symbol)
        print(f'PASS GI variant {variant}: point MAD {sites[0]}, spot MAD {sites[1]}, visibility {visibility[0]}, cookie {cookie["dword"]}, falloff {falloffs[0]["dword"]}, Fresnel {fresnels[0]["dword"]}; all unrelated bytes preserved')
    header.append('inline constexpr const ShaderPatch* kGISourcePatches[8][7]={'+','.join('{'+','.join(symbols[i:i+7])+'}' for i in range(0,56,7))+'};')
    header.append('}')
    text = '\n'.join(header)+'\n'
    path = ROOT / 'src/gi_source_patches.generated.h'
    if '--check' in sys.argv:
        assert path.read_text() == text
    else:
        path.write_text(text)

if __name__ == '__main__':
    main()
